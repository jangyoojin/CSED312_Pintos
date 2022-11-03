#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H


#include <stdbool.h>
typedef int pid_t;


void syscall_init (void);
void check_user_addr(void *addr);
void get_arg(int * esp,int *argv, int argc);
void halt();
void exit(int status);
bool create_file(const char *file, unsigned initial_size);
bool remove_file(const char *file);
pid_t exec(const char * cmdline);
int wait(pid_t tid);
int open(const char *file);
int filesize(int fd);
int read (int fd, void *buffer, unsigned size);
int write(int fd, void *buffer, unsigned size);
void seek (int fd, unsigned position);
unsigned tell (int fd);
void close (int fd);



#endif /* userprog/syscall.h */
