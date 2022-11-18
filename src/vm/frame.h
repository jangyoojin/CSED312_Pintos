/*
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

#endif

*/