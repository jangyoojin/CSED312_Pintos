#ifndef FRAME_H
#define FRAME_H

#include "threads/thread.h"
#include "page.h"
#include "threads/synch.h"
#include "threads/palloc.h"
#include <list.h>


struct frame
{
    void * faddr;
    struct vm_entry * vme;
    struct thread *thread;
    struct list_elem elem;
};

struct list frame_table;
static struct frame * frame_clock_head;
struct lock frame_lock;


void frame_table_init(void);
struct frame * frame_alloc(enum palloc_flags flags);
void frame_dealloc(void * faddr);
static struct list_elem* next_frame();
void frame_evict(enum palloc_flags flags);

#endif
