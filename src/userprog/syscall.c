#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "process.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "vm/page.h"
#include <string.h>
#include "threads/vaddr.h"
#include "vm/frame.h"

#define STACK_END 0x8048000
#define STACK_BASE 0xc0000000
#define MAX_STACK_SIZE (1 << 23)

static void syscall_handler(struct intr_frame *);

void syscall_init(void)
{
  lock_init(&filesys_lock);
  intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler(struct intr_frame *f UNUSED)
{
  int argv[3];
  uint32_t *sp = f->esp;
  check_user_addr((void *)sp);
  int syscall_number = *sp;


  switch (syscall_number)
  {
  case SYS_HALT:
    halt();
    break;
  case SYS_EXIT:
    get_arg(sp, argv, 1);
    exit(argv[0]);
    break;
  case SYS_EXEC:
    get_arg(sp, argv, 1);
    f->eax = exec((const char *)argv[0]);
    break;
  case SYS_WAIT:
    get_arg(sp, argv, 1);
    f->eax = wait((pid_t)argv[0]);
    break;
  case SYS_CREATE:
    get_arg(sp, argv, 2);
    f->eax = create_file(argv[0], argv[1]);
    break;
  case SYS_REMOVE:
    get_arg(sp, argv, 1);
    f->eax = remove_file(argv[0]);
    break;
  case SYS_OPEN:
    get_arg(sp, argv, 1);
    f->eax = open(argv[0]);
    break;
  case SYS_FILESIZE:
    get_arg(sp, argv, 1);
    f->eax = filesize(argv[0]);
    break;
  case SYS_READ:
    get_arg(sp, argv, 3);
    f->eax = read(argv[0], argv[1], argv[2]);
    break;
  case SYS_WRITE:
    get_arg(sp, argv, 3);
    f->eax = write(argv[0], argv[1], argv[2]);
    break;
  case SYS_SEEK:
    get_arg(sp, argv, 2);
    seek(argv[0], argv[1]);
    break;
  case SYS_TELL:
    get_arg(sp, argv, 1);
    f->eax = tell(argv[0]);
    break;
  case SYS_CLOSE:
    get_arg(sp, argv, 1);
    close(argv[0]);
    break;
  case SYS_MMAP:
    get_arg(sp,argv,2);
    f->eax=mmap(argv[0],argv[1]);
    break;

  case SYS_MUNMAP:
    get_arg(sp,argv,1);
    munmap(argv[0]);
    break;

  }
}

void halt()
{
  shutdown_power_off();
}

void exit(int status)
{
  struct thread *t = thread_current();
  t->exit_status = status;
  printf("%s: exit(%d)\n", thread_name(), status);
  thread_exit();
}

bool create_file(const char *file, unsigned initial_size)
{
  //check_user_addr(file);
  check_valid_string(file);
  return filesys_create(file, initial_size);
}

bool remove_file(const char *file)
{
  //check_user_addr(file);
  check_valid_string(file);
  return filesys_remove(file);
}

pid_t exec(const char *cmdline)
{
  check_valid_string(cmdline);
  struct thread *child;
  pid_t pid = process_execute(cmdline);
  if (pid == -1)
    return -1;
  child = get_child(pid);
  sema_down(&(child->sema_load));

  if (child->is_load == false)
    return -1;
  else
    return pid;
}

int wait(pid_t pid)
{
  return process_wait(pid);
}

int open(const char *file)
{
  struct file *f;
  check_valid_string(file);
  if (file == NULL)
    exit(-1);
  int fd_cnt = thread_current()->fd_max;
  lock_acquire(&filesys_lock);
  f = filesys_open(file);
  lock_release(&filesys_lock);

  if (f == NULL) {
    return -1;}

  if (strcmp(thread_name(), file) == 0)
    file_deny_write(f);

  int fd = process_file_add(f);
  return fd_cnt;
}

int filesize(int fd)
{
  struct file *file = process_file_get(fd);
  if (file == NULL)
    return -1;

  return file_length(file);
}

int read(int fd, void *buffer, unsigned size)
{
  check_valid_buffer(buffer, size, true);
  int read_byte;
  int i;

  lock_acquire(&filesys_lock);
  if (fd == 0)
  {
    for (i = 0; i < size; i++)
    {
      ((char *)buffer)[i] = input_getc();
      if (((char *)buffer)[i] == '\0')
        break;
    }
    read_byte = i + 1;
  }
  else
  {
    struct file *file = process_file_get(fd);
    if (file == NULL)
    {
      lock_release(&filesys_lock);
      return -1;
    }
    read_byte = file_read(file, buffer, size);
  }

  lock_release(&filesys_lock);
  return read_byte;
}

int write(int fd, void *buffer, unsigned size)
{
  check_valid_buffer(buffer, size, false);
  int write_byte;

  if (fd == 1)
  {
    lock_acquire(&filesys_lock);
    putbuf(buffer, size);
    lock_release(&filesys_lock);
    return size;
  }
  else
  {
    lock_acquire(&filesys_lock);
    struct file *file = process_file_get(fd);
    if (file == NULL)
    {
      lock_release(&filesys_lock);
      return -1;
    }
    write_byte = file_write(file, buffer, size);
    lock_release(&filesys_lock);
    return write_byte;
  }
}

void seek(int fd, unsigned position)
{
  struct file *file = process_file_get(fd);
  if (file == NULL)
    return;
  else
    file_seek(file, position);
}

unsigned tell(int fd)
{
  struct file *file = process_file_get(fd);
  if (file == NULL)
    return -1;
  else
    return file_tell(file);
}

void close(int fd)
{
  process_file_close(fd);
}


struct vm_entry * check_user_addr(void *addr)
{
  if (addr < STACK_END || addr >= STACK_BASE)
    exit(-1);

  struct vm_entry * vme=vm_find_vme(addr);

  
  if(vme==NULL) 
    {
      if(vm)
      
      exit(-1);}

  return vme;
  
}
void check_valid_buffer (void * buffer, unsigned size, bool to_write)
{
  check_user_addr(buffer);
  check_user_addr(buffer+size);
  unsigned int i=0;
  for (i = 0; i <= size; i++)
  { 
    struct vm_entry * vme=check_user_addr(buffer+i);
    if(to_write && vme->writable==false)  
      exit(-1);
  }
}

void check_valid_string(const void * str)
{
  unsigned int i=0;
  while(1)
  {
    check_user_addr(str+i);
    if(*(char *)(str+i)=='\0') break;
    i++;
  }
  return;
}

void get_arg(int *esp, int *argv, int argc)
{
  int i;
  for (i = 0; i < argc; i++)
  {
    check_user_addr(esp + 1);
    esp += 1;
    argv[i] = *esp;
  }
}

int mmap(int fd, void * addr)
{

  int size = filesize(fd);
  struct file * file = file_reopen(process_file_get(fd));
  if (size==0||file==NULL ||fd<2) {
    return -1; }
  if((uint32_t)addr%PGSIZE!=0||addr==NULL || pg_ofs(addr)!=0)
  {
    return -1;
  }
  if(addr < STACK_END || addr >= STACK_BASE)
  exit(-1);
  
  void * temp;
  for(temp=addr;temp<addr+size;temp+=PGSIZE)
  {
    if(vm_find_vme(temp)!=NULL) return -1;
  }


  struct mmap_file * mapfile = malloc(sizeof(struct mmap_file));
  if(mapfile==NULL) return -1;

  thread_current()->mapid++;
  mapfile->mapid=thread_current()->mapid;
  mapfile->file=file;

  list_init(&(mapfile->vme_list));
  list_push_back(&(thread_current()->mmap_list), &(mapfile->elem));

  struct vm_entry * vme;
  int offset=0;
  while(size>0)
  {
    vme=malloc(sizeof(struct vm_entry));
    //if(vme==NULL) return -1;
    vme->file=file;
    vme->type=VM_FILE;
    vme->vaddr= addr;
    vme->offset= offset;
    vme-> writable= true;
    vme-> is_loaded = false;
    if (size<PGSIZE) vme->read_bytes= size;
    else vme->read_bytes=PGSIZE;

    vme->zero_bytes= PGSIZE - vme->read_bytes;

    list_push_back(&mapfile->vme_list , &vme->mmap_elem);
    vm_insert_vme(&(thread_current()-> vm), vme);

    addr += PGSIZE;
    offset += PGSIZE;
    size -= PGSIZE;
    

  }

return mapfile->mapid;
  

}


void munmap(int mapping)
{
  struct list_elem * e;
  struct mmap_file * temp;

  for (e=list_begin(&(thread_current()->mmap_list)); e!= list_end(&thread_current()->mmap_list);)
  {
    temp = list_entry (e, struct mmap_file, elem);
    if (mapping==CLOSE_ALL) 
    {     
      do_munmap(temp);
      e=list_remove(e);
      free(temp);
    }
    else if (temp->mapid == mapping) 
    {
      do_munmap(temp);
      e=list_remove(e);
      free(temp);
      break;
    }
  }

}

void do_munmap(struct mmap_file * mmap_file)
{
  struct list_elem * e;
  for (e=list_begin(&(mmap_file->vme_list)); e!= list_end(&mmap_file->vme_list);)
  {
    struct vm_entry * vme = list_entry (e, struct vm_entry, mmap_elem);
    if (vme->is_loaded)
    {
      if (pagedir_is_dirty(thread_current()->pagedir, vme->vaddr)){
      lock_acquire(&filesys_lock);
      file_write_at(vme->file, vme->vaddr, vme->read_bytes,vme->offset);
      lock_release(&filesys_lock);

      }
      
      frame_dealloc(pagedir_get_page(thread_current()->pagedir,vme->vaddr));
      pagedir_clear_page(thread_current()->pagedir,vme->vaddr);
    }       
    e = list_remove(e);//mmpfile의 vme_list 에서 삭제

    //pagedir_clear_page(thread_current()->pagedir, vme->vaddr);
    //palloc_free_page(pagedir_get_page(thread_current()->pagedir,vme->vaddr));
    vm_delete_vme(&thread_current()->vm, vme);
  }
  file_close(mmap_file->file);

}


// /*-----------------Stack growth--------------------*/
// bool expand_stack(void *addr) {
//   if(addr >= (PHYS_BASE - MAX_STACK_SIZE))
//   {
//     struct frame *f = frame_alloc(PAL_USER);
//     if(f == NULL) return false;

//     struct vm_entry * v = malloc(sizeof(struct vm_entry));
//     v->type = VM_ANON;
//     v->vaddr = pg_round_down(addr);
//     v->writable = true;
//     v->is_loaded = true;
//     v->is_stack = true;
//     f->vme = v;

//     if(!install_page(v->vaddr, f->faddr, v->writable)) {
//       frame_dealloc(f);
//       free(v);
//       return false;
//     }
//     else if(!vm_insert_vme(&thread_current()->vm, v)) {
//       frame_dealloc(f);
//       free(v);
//       return false;
//     }
  
//     return true;
//   }
//   else return false;
// }