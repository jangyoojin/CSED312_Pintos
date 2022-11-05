#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);
struct thread* get_child(int pid);
void argument_stack(char **argv, int argc, void **esp);
void remove_child(struct thread* child);
int process_file_add(struct file* f);
struct file * process_file(int fd);
void process_file_close(int fd);

#endif /* userprog/process.h */
