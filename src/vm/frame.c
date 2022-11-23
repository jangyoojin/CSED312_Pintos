#include "frame.h"
#include "vm/page.h"


void frame_table_init(void)
{
  list_init(&frame_table);
  lock_init(&frame_lock);
  frame_clock_head=NULL;
}


struct frame * frame_alloc(enum palloc_flags flags)
{
	if(flags & PAL_USER ==0)
	  return NULL;

  void * faddr = palloc_get_page(flags);
  
	//free physical memory가 없으면 evict하고 할당
  while(faddr==NULL) {
    frame_evict(flags);
		faddr = palloc_get_page(flags);
  }
	
	//위에서 할당받은 physical memory에 대한 정보를 담는 frame 선언
  struct frame * f = malloc(sizeof(struct frame));
  if(f==NULL) {
    palloc_free_page(faddr);
    return f;
  }
  f->faddr=faddr;
  f->thread=thread_current();
	
	//frame_table에 추가
	lock_acquire(&frame_lock);
  if(list_empty(&frame_table))
    frame_clock_head = &(f->elem);
  list_push_back(&frame_table, &(f->elem));
  lock_release(&frame_lock);
  return f;

}

//faddr인 frame 할당 해제하기
void frame_dealloc(void * faddr)
{
  
  struct list_elem * e;
  struct frame *f;
  for(e = list_begin(&frame_table); e != list_end(&frame_table); e = list_next(e))
  {
    f = list_entry(e, struct frame, elem);
    if(f->faddr==faddr) 
    {
      //pagedir_clear_page(f->thread->pagedir, f->vme->vaddr);
      palloc_free_page(f->faddr);
      lock_acquire(&frame_lock);
      list_remove(&(f->elem));
      lock_release(&frame_lock);
      free(f);
      break;
    }
  }

}

static struct list_elem* next_frame() {
	struct list_elem* e;
	struct frame* f;

	for(e = frame_clock_head; e != list_end(&frame_table); e = list_next(e)) {
		f = list_entry(e, struct frame, elem);
		if(pagedir_is_accessed(f->thread->pagedir, f->vme->vaddr)) 
			pagedir_set_accessed(f->thread->pagedir, f->vme->vaddr, false);
		else {
			//frame_clock_head = f;
			return e;
		}
	}

	for(e = list_begin(&frame_table); e != frame_clock_head; e = list_next(e)) {
		f = list_entry(e, struct frame, elem);
		if(pagedir_is_accessed(f->thread->pagedir, f->vme->vaddr)) 
			pagedir_set_accessed(f->thread->pagedir, f->vme->vaddr, false);
		else {
			//frame_clock_head = f;
			return e;
		}
	}
	return NULL;
	
}

void frame_evict(enum palloc_flags flags)
{
  lock_acquire(&frame_lock);
  //printf("frame_evict\n");
  struct list_elem * e = next_frame();
  if(e == NULL) {
    lock_release(&frame_lock);
    return;
  }
  struct frame * f = list_entry(e, struct frame, elem);

  switch (f->vme->type)
  {
    case VM_BIN:
      if(pagedir_is_dirty(f->thread->pagedir, f->vme->vaddr)) {
        f->vme->type = VM_ANON;
        f->vme->swap_slot = swap_out(f->faddr);
      }
      break;
    case VM_FILE:
      if(pagedir_is_dirty(f->thread->pagedir, f->vme->vaddr)) {
        lock_acquire(&filesys_lock);
        file_write_at(f->vme->file, f->faddr, f->vme->read_bytes, f->vme->offset);
        lock_release(&filesys_lock);
      }
      break;
    case VM_ANON:
      f->vme->swap_slot = swap_out(f->faddr);
      break;
  }
  f->vme->is_loaded = false;
  frame_clock_head = list_remove(e);
  pagedir_clear_page(f->thread->pagedir, f->vme->vaddr);
  palloc_free_page(f->faddr);
  list_remove(&(f->elem));
  free(f);
  lock_release(&frame_lock);
}

