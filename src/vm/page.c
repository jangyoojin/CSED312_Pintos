#include "page.h"
#include "threads/synch.h"


void vm_init (struct hash *vm) {
    hash_init(vm, vm_hash_func, vm_less_func, NULL);
}

//do hashing. It return a hash value
static unsigned vm_hash_func (const struct hash_elem *e, void *aux) {
    struct vm_entry* vme = hash_entry(e, struct vm_entry, elem);
    return hash_int((int)vme->vaddr);
}

//For sorting hash elems?
static bool vm_less_func (const struct hash_elem *a,const struct hash_elem *b) {
    struct vm_entry *v1 = hash_entry(a, struct vm_entry, elem);
    struct vm_entry *v2 = hash_entry(b, struct vm_entry, elem);
    return v1->vaddr < v2->vaddr;
}

bool vm_insert_vme (struct hash *vm, struct vm_entry *vme) {
    struct hash_elem *v = hash_insert(vm, &vme->elem);
    if(v == NULL) return true;
    return false;
}

bool vm_delete_vme (struct hash *vm, struct vm_entry *vme) {
    struct hash_elem * v = hash_delete(vm, &vme->elem);
    free(vme);
    if(v == NULL) return false;
    return true;
}

struct vm_entry *vm_find_vme(void *vaddr) {
    struct vm_entry obj;
    obj.vaddr = pg_round_down(vaddr);
    struct hash_elem * v = hash_find(&thread_current()->vm, &obj.elem);
    if(v == NULL) return v;
    return hash_entry(v, struct vm_entry, elem);
}

void vm_destroy (struct hash * vm) {
    hash_destroy(vm, vm_destroy_func);
}

void vm_destroy_func (struct hash_elem * v, void*aux UNUSED) {
  struct vm_entry * vme = hash_entry(v, struct vm_entry, elem);
  
  if(vme != NULL) 
  {
    if(vme->is_loaded) {
        //maybe we should deallcate frame here......
        pagedir_clear_page(thread_current()->pagedir, vme->vaddr);
        frame_dealloc(pagedir_get_page(thread_current()->pagedir, vme->vaddr));
    }
    free(vme);
  }
}

bool load_file (void * kaddr, struct vm_entry *vme) {

 size_t bytes;
    
    if(lock_held_by_current_thread)
        bytes = file_read_at(vme->file, kaddr, vme->read_bytes, vme->offset);
    else {
    lock_acquire(&filesys_lock);
    bytes = file_read_at(vme->file, kaddr, vme->read_bytes, vme->offset);
    lock_release(&filesys_lock);

    }
    if (bytes == vme->read_bytes) {
        memset(kaddr + bytes, 0, vme->zero_bytes);
        return true;
    }
    return false;

}

/*------------------swapping----------------------*/
void swap_init()
{
	swap_block = block_get_role(BLOCK_SWAP);
	swap_bitmap = bitmap_create(block_size(swap_block)*BLOCK_SECTOR_SIZE / PGSIZE);	
	lock_init(&swap_lock);
}

bool swap_in(size_t used_index, void* kaddr)
{
	lock_acquire(&swap_lock);
	int i;
    int sector_num = PGSIZE / BLOCK_SECTOR_SIZE;
	int target_sector = used_index * sector_num;

    if (bitmap_test(swap_bitmap, used_index) == 0) {
        lock_release(&swap_lock);
        return false;
    }
    lock_acquire(&filesys_lock);
	for (i = 0; i < sector_num; i++) {
		block_read(swap_block, target_sector+i, kaddr+i*BLOCK_SECTOR_SIZE);
	}
    lock_release(&filesys_lock);
    bitmap_flip(swap_bitmap, used_index);
	lock_release(&swap_lock);
    return true;
}

size_t swap_out(void* kaddr) {
    size_t i = 0;
    
    lock_acquire(&swap_lock);
    size_t swap_slot = bitmap_scan_and_flip(swap_bitmap, 0, 1, 0);
    if (swap_slot == BITMAP_ERROR) {
        lock_release(&swap_lock);
        return BITMAP_ERROR;
    }

    int sector_num = BLOCK_SECTOR_SIZE / PGSIZE;
    for (i = 0; i < sector_num; i++) {
        block_write(swap_block, swap_slot * i, kaddr + i * BLOCK_SECTOR_SIZE);
    }
    lock_release(&swap_lock);
    return swap_slot;    
}