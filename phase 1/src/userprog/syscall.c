#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "devices/shutdown.h"
#include "userprog/process.h"
#include "threads/vaddr.h"

static void syscall_handler(struct intr_frame *);

void syscall_init(void)
{
  intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
}

/*phase 1 do it!*/
static void validate_user_addr(const void *vaddr)
{
  if (vaddr == NULL)
    exit(-1);

  if (!is_user_vaddr(vaddr))
    exit(-1); // 잘못된 접근이므로 프로세스 종료

  // 페이지 디렉토리에서 유효한 주소인지 확인 (페이지가 매핑되지 않으면 종료)
  void *ptr = pagedir_get_page(thread_current()->pagedir, vaddr);
  if (ptr == NULL)
    exit(-1);
}

void halt(void)
{
  shutdown_power_off();
}

void exit(int status)
{
  struct thread *cur = thread_current();
  printf("%s: exit(%d)\n", cur->name, status);
  cur->exit_status = status;
  thread_exit(); // 현재 스레드 종료
}

pid_t exec(const char *cmd)
{
  return process_execute(cmd);
}

int wait(pid_t pid)
{
  return process_wait(pid);
}

int read(int fd, void *buf, unsigned size)
{
  if (fd == 0) // 표준 입력에서 읽기
  {
    unsigned int cnt;
    uint8_t *buffer = (uint8_t *)buf;

    for (cnt = 0; cnt < size; cnt++)
    {
      buffer[cnt] = input_getc();
    }
    return cnt;
  }
  else
    return -1; // 지원되지 않는 파일 디스크립터
  
}

int write(int fd, const void *buf, unsigned size)
{
  if (fd == 1)
  {
    putbuf(buf, size);
    return size;
  }
  return -1;
}

int fibonacci(int x)
{
  // fibonacchi(47) 부터는 값이 32비트 정수형으로 표현할 수 있는 최대값을 초과함.
  if (x < 0 || x >= 47)
    return -1;

  if (x == 0)
    return 0;
  if (x == 1)
    return 1;

  int a = 0, b = 1, sum;
  for (int i = 2; i <= x; i++)
  {
    sum = a + b;
    a = b;
    b = sum;
  }
  return b;
}

int max_of_four_int(int a, int b, int c, int d)
{
  int max_ab = (a > b) ? a : b;
  int max_cd = (c > d) ? c : d;
  return (max_ab > max_cd) ? max_ab : max_cd;
}

static void
syscall_handler(struct intr_frame *f UNUSED)
{
  uint32_t *esp = f->esp;
  validate_user_addr(esp);
  uint32_t syscall_num = *(uint32_t *)esp;

  switch (syscall_num)
  {
  case SYS_HALT:
    halt();
    break;

  case SYS_EXIT:
    validate_user_addr(esp + 1);
    exit((int)*(uint32_t *)(esp + 1));
    break;

  case SYS_EXEC:
    validate_user_addr(esp + 1);
    f->eax = exec((char *)*(uint32_t *)(esp + 1));
    break;

  case SYS_WAIT:
    validate_user_addr(esp + 1);
    f->eax = wait((pid_t) * (uint32_t *)(esp + 1));
    break;

  case SYS_READ:
    validate_user_addr(esp + 1);
    validate_user_addr(esp + 2);
    validate_user_addr(esp + 3);
    f->eax = read((int)*(uint32_t *)(esp + 1), (void *)*(uint32_t *)(esp + 2), (unsigned)*(uint32_t *)(esp + 3));
    break;

  case SYS_WRITE:
    validate_user_addr(esp + 1);
    validate_user_addr(esp + 2);
    validate_user_addr(esp + 3);
    f->eax = write((int)*(uint32_t *)(esp + 1), (void *)*(uint32_t *)(esp + 2), (unsigned)*(uint32_t *)(esp + 3));
    break;

  case SYS_FIBONACCI:
    validate_user_addr(esp + 1);
    f->eax = fibonacci((int)*(uint32_t *)(esp + 1));
    break;

  case SYS_MAX_OF_FOUR_INT:
    validate_user_addr(esp + 1);
    validate_user_addr(esp + 2);
    validate_user_addr(esp + 3);
    validate_user_addr(esp + 4);
    f->eax = max_of_four_int((int)*(uint32_t *)(esp + 1), (int)*(uint32_t *)(esp + 2), (int)*(uint32_t *)(esp + 3), (int)*(uint32_t *)(esp + 4));
    break;

  default:
    exit(-1);
  }
}
