/*
#include "frame.h"


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

    void * faddr=palloc_get_page(flags);
    
    while(faddr==NULL)
        faddr= frame_evict(flags);

    struct frame * f= malloc(sizeof(struct frame));
    if(f==NULL) {
        palloc_free_page(faddr);
        return f;
    }

    f->faddr=faddr;
    f->thread=thread_current();



    lock_acquire(&frame_lock);
    list_push_back(&frame_table, &(f->elem));
    lock_release(&frame_lock);
    return f;

}


void frame_dealloc(void * faddr)
{
    struct list_elem * e;
    struct frame *f;
    for(e=list_begin(&frame_table);e!=list_end(&frame_table);e=list_next(e))
    {
        f=list_entry(e,struct frame, elem);
        if(f->faddr==faddr) 
        {
            palloc_free_page(f->faddr);
            //만약 deallocate 
            lock_acquire(&frame_lock);
            list_remove(&(f->elem));
            lock_release(&frame_lock);
            free(f);
            break;
        }
    }
}


void frame_evict()
{


}




*/