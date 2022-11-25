#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H


#include <stdbool.h>
#include "vm/page.h"
#include "vm/frame.h"
#define STACK_END 0x8048000
#define STACK_BASE 0xc0000000
typedef int pid_t;
struct lock filesys_lock;



void syscall_init (void);
struct vm_entry * check_user_addr(void *addr,void * fesp);

void check_valid_buffer(void * buffer, unsigned size, bool to_write,void *fesp);
void check_valid_string(const void * str, void * fesp);

void get_arg(int * esp,int *argv, int argc, void * fesp);
void halt();
void exit(int status);
bool create_file(const char *file, unsigned initial_size,void * fesp);
bool remove_file(const char *file,void * fesp);
pid_t exec(const char * cmdline,void * fesp);
int wait(pid_t tid);
int open(const char *file,void * fesp);
int filesize(int fd);
int read (int fd, void *buffer, unsigned size,void * fesp);
int write(int fd, void *buffer, unsigned size,void * fesp);
void seek (int fd, unsigned position);
unsigned tell (int fd);
void close (int fd);


int mmap(int fd, void * addr);
void munmap(int mapping);
void do_munmap(struct mmap_file * mmap_file);



#endif /* userprog/syscall.h */
