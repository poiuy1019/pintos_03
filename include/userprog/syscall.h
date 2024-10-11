#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
#include <stdbool.h>

typedef int pid_t;
extern struct lock syscall_lock;

void syscall_init (void);
void halt (void);
void exit (int status);
// pid_t fork (const char *thread_name, struct intr_frame *f); //compile error 때문에 없앰.
int exec (const char *cmd_line);
int wait (pid_t pid);
bool create (const char *file, unsigned initial_size);
bool remove (const char *file);
int open (const char *file);
int filesize (int fd);
int read (int fd, void *buffer, unsigned size);
int write (int fd, const void *buffer, unsigned size);
void seek (int fd, unsigned position);
unsigned tell (int fd);
void close (int fd);
// page_fault handler 에서 사용하기 위해 추가
void user_memory_valid(void *r);

#endif /* userprog/syscall.h */
