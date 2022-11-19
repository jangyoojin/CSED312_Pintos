#ifndef PAGE_H
#define PAGE_H

#include <ctype.h>
#include <stdint.h>
#include <list.h>
#include <hash.h>
#include "threads/palloc.h"
#include "threads/thread.h"
#include "userprog/pagedir.h"
#include "filesys/file.h"
#include "threads/vaddr.h"

#define VM_BIN 0
#define VM_FILE 1
#define VM_ANON 2

struct vm_entry {
    uint8_t type;
    void *vaddr;
    bool writable;

    bool is_loaded;
    struct file *file;

    /*----Memory mapped file ------*/
    struct list_elem mmap_elem;

    size_t offset;
    size_t read_bytes;
    size_t zero_bytes;

    /*-------Swapping---------*/
    size_t swap_slot;

    struct hash_elem elem;
};


struct mmap_file{
    int mapid;    
    struct list vme_list;
    struct file * file;
    struct list_elem elem;
};


void vm_init (struct hash *vm);
static unsigned vm_hash_func (const struct hash_elem *e, void *aux);
static bool vm_less_func (const struct hash_elem *a,const struct hash_elem *b);
bool vm_insert_vme (struct hash *vm, struct vm_entry *vme);
bool vm_delete_vme (struct hash *vm, struct vm_entry *vme);
struct vm_entry* vm_find_vme(void *vaddr);
void vm_destroy (struct hash * vm);
void vm_destroy_func (struct hash_elem * v, void*aux UNUSED);
bool load_file (void * kaddr, struct vm_entry *vme);

#endif