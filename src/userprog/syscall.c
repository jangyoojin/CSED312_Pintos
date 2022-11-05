#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

#define STACK_END 0x8048000
#define STACK_BASE 0xc0000000

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  lock_init(&filesys_lock);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  int argv[3];
  uint32_t *sp= f->esp;
  check_user_addr((void*)sp);
  int syscall_number=*sp;
  
  switch(syscall_number)
  {
    case SYS_HALT:
      halt();
      break;
    case SYS_EXIT:
      get_arg(sp,argv,1);
      exit(argv[0]);
      break;
    case SYS_EXEC:
      get_arg(sp, argv,1);
      f->eax=exec((const char *) argv[0]);
      break;
    case SYS_WAIT:
      get_arg(sp,argv,1);
      f->eax=wait((pid_t)argv[0]);
      break;
    case SYS_CREATE:
      get_arg(sp,argv,2);
      f->eax=create_file(argv[0],argv[1]);
      break;
    case SYS_REMOVE:
      get_arg(sp,argv,1);
      f->eax=remove_file(argv[0]);
      break;
    case SYS_OPEN:
      get_arg(sp,argv,1);
      f->eax=open(argv[0]);
      break;
    case SYS_FILESIZE:
      get_arg(sp,argv,1);
      f->eax=filesize(argv[0]);
      break;
    case SYS_READ :
      get_arg(sp,argv,3);
      f->eax=read(argv[0],argv[1],argv[2]);
      break;
    case SYS_WRITE:
      get_arg(sp,argv,3);
      f->eax=write(argv[0],argv[1],argv[2]);
      break;
    case SYS_SEEK:
      get_arg(sp,argv,2);
      seek(argv[0],argv[1]);
      break;
    case SYS_TELL:
      get_arg(sp,argv,1);
      f->eax=tell(argv[0]);
      break;
    case SYS_CLOSE:
      get_arg(sp,argv,1);
      close(argv[0]);
      break;
    
  }

  }


void halt()
{
  shutdown_power_off();
}

void exit(int status)
{
  struct thread * t=thread_current();
  t->pcb->exit_status = status;
  printf("%s: exit(%d)\n",t->name, status);
  thread_exit();
}

bool create_file(const char * file , unsigned initial_size)
{
  check_user_addr(file);
  return filesys_create(file,initial_size);
}

bool remove_file(const char * file)
{
  check_user_addr(file);
  return filesys_remove(file);
}

pid_t exec(const char * cmdline)
{
  check_user_addr(cmdline);
  struct pcb * child_pcb;
  pid_t pid=process_execute(cmdline);
  if (pid == -1) return -1;
  child_pcb=get_child(pid);
  sema_down(&(child_pcb->sema_load));

  if(child_pcb->is_load==false) return -1;
  else return pid;
}

int wait(pid_t pid)
{
  return process_wait(pid);
}

int open(const char *file){
  struct file *f;
  check_user_addr((void*) file);
  if(file == NULL) exit(-1);
  f = filesys_open(file);

  if(f==NULL) return -1;

  int fd = process_file_add(f);
  return fd;
}

int filesize(int fd) {
  struct file* file = process_file_get(fd);
  if(file==NULL) return -1;

  return file_length(file);
}

int read (int fd, void *buffer, unsigned size){
  check_user_addr(buffer);
  int read_byte;
  int i;

  lock_acquire(&filesys_lock);
  if(fd == 0) {
    for (i = 0; i<size; i++) {
      ((char*)buffer)[i] = input_getc();
      if (((char*)buffer)[i]=='\0') break;
    }
    read_byte = i + 1;
  }
  else {
    struct file *file = process_file_get(fd);
    if(file == NULL) {
      lock_release(&filesys_lock);
      return -1;
    }
    read_byte = file_read(file, buffer, size);
  }
  
  lock_release(&filesys_lock);
  return read_byte;
}

int write(int fd, void* buffer, unsigned size) {
  check_user_addr(buffer);
  int write_byte;

  if(fd == 1) {
    lock_acquire(&filesys_lock);
    putbuf(buffer,size);
    lock_release(&filesys_lock);
    return size;
  }
  else {
    lock_acquire(&filesys_lock);
    struct file *file = process_file_get(fd);
    if(file == NULL) {
      lock_release(&filesys_lock);
      return -1;
    }
    write_byte = file_write(file,buffer,size);
    lock_release(&filesys_lock);
    return write_byte;
  }
}

void seek (int fd, unsigned position) {
  struct file* file = process_file_get(fd);
  if(file == NULL) return;
  else file_seek(file,position);
}

unsigned tell(int fd) {
  struct file* file = process_file_get(fd);
  if(file == NULL) return -1;
  else return file_tell(file);
}

void close(int fd){
  process_file_close(fd);
}


//파일 만들면서 주소 체크하는 거 잊기 ㄴ ㄴ









void check_user_addr(void *addr)
{
  if(addr<STACK_END && addr>=STACK_BASE) exit(-1);
}

void get_arg(int *esp, int * argv, int argc)
{
  int i;
  for(i=0;i<argc;i++)
  {
    check_user_addr(esp+1);
    esp += 1;
    argv[i]=*esp;
  }
}