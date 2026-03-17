#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "lib/user/syscall.h"
#include <stdbool.h>
struct lock filesys_lock;
typedef int pid_t;

void syscall_init(void);
void halt(void);
void exit(int status);
pid_t exec(const char *cmd);
int wait(pid_t pid);
int read(int fd, void *buf, unsigned size);
int write(int fd, const void *buf, unsigned size);
int fibonacci(int x);
int max_of_four_int(int a, int b, int c, int d);
bool create(const char *file, unsigned init_size);
bool remove(const char *file);
int filesize(int fd);
void seek(int fd, unsigned pos);
unsigned tell(int fd);
int open(const char *file);
void close(int fd);
#endif /* userprog/syscall.h */
