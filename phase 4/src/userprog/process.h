#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"
#define MAX_STACK_SIZE (1 << 23) // 8MB

tid_t process_execute(const char *file_name); // Execute a process
int process_wait(tid_t);                      // Wait for a process to finish
void process_exit(void);                      // Exit the current process
void process_activate(void);                  // Activate a new process

/*phase 4 do it*/
struct virtual_page *vm_entry_create(void *addr);
struct page *page_create(struct virtual_page *vm_entry);
bool memory_fault_handler(struct virtual_page *vm_entry); // Handle a memory fault
/*phase 4 do it*/

#endif /* userprog/process.h */
