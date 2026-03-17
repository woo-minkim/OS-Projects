#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "devices/shutdown.h"
#include "userprog/process.h"
#include "threads/vaddr.h"
#include "threads/synch.h"

static void syscall_handler(struct intr_frame *);

void syscall_init(void)
{
  /*phase 2 do it*/
  lock_init(&filesys_lock);
  /*phase 2 do it*/
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

  /*phase 2 do it*/
  for (int i = 3; i < 128; i++)
  {
    if (cur->fd[i])
    {
      close(i);
    }
  }
  /*phase 2 do it*/
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
  // 기본 반환 값 -1(Error)
  int bytes_read = -1;

  // 버퍼 유효성 검사
  if (!is_user_vaddr(buf))
    exit(-1);

  lock_acquire(&filesys_lock);
  if (fd == 0)
  {
    unsigned i;
    uint8_t *buffer = (uint8_t *)buf;
    for (i = 0; i < size; i++)
    {
      char c = input_getc();
      buffer[i] = c;
      if (c == '\0')
        break;
    }
    bytes_read = i;
  }
  else
  {
    struct file *f = thread_current()->fd[fd];
    if (f == NULL)
    {
      lock_release(&filesys_lock);
      exit(-1);
    }
    bytes_read = file_read(f, buf, size);
  }

  lock_release(&filesys_lock);
  return bytes_read;
}

int write(int fd, const void *buf, unsigned size)
{
  int bytes_written = -1;

  lock_acquire(&filesys_lock);

  if (fd == 1)
  {
    // 표준 출력(stdout)에 쓰기
    putbuf(buf, size);
    bytes_written = size;
  }
  else if (fd >= 3)
  {
    struct file *f = thread_current()->fd[fd];

    if (f == NULL)
    {
      lock_release(&filesys_lock);
      exit(-1);
    }
    else
    {
      // 파일에 쓰기
      bytes_written = file_write(f, buf, size);
    }
  }
  else
  {
    // 유효하지 않은 파일 디스크립터
    bytes_written = -1;
  }

  lock_release(&filesys_lock);
  return bytes_written;
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

/*phase 2 do it*/
bool create(const char *file, unsigned init_size)
{
  if (file == NULL)
    exit(-1);
  return filesys_create(file, init_size);
}

bool remove(const char *file)
{
  if (file == NULL)
    exit(-1);
  return filesys_remove(file);
}

int filesize(int fd)
{
  struct file *f = thread_current()->fd[fd];
  if (f == NULL)
    exit(-1);
  return file_length(f);
}

void seek(int fd, unsigned pos)
{
  struct file *f = thread_current()->fd[fd];
  if (f == NULL)
    exit(-1);
  return file_seek(f, pos);
}

unsigned tell(int fd)
{
  struct file *f = thread_current()->fd[fd];
  if (f == NULL)
    exit(-1);
  return file_tell(f);
}

int open(const char *file)
{
  if (file == NULL)
    exit(-1);

  struct file *f;
  struct thread *cur = thread_current();
  int nxt = cur->fd_nxt;
  int assigned_fd = -1;

  lock_acquire(&filesys_lock);
  f = filesys_open(file);
  if (f == NULL || cur->fd_size == 128)
  {
    lock_release(&filesys_lock);
    return -1;
  }
  lock_release(&filesys_lock);

  if (strcmp(cur->name, file) == 0)
    file_deny_write(f);

  // 첫 번째 유효한 fd를 찾기 위한 루프
  for (assigned_fd = 3; assigned_fd < 128; assigned_fd++)
  {
    if (cur->fd[assigned_fd] == NULL)
      break;
  }

  if (assigned_fd == 128) // 유효한 fd가 없는 경우
    return -1;

  cur->fd[assigned_fd] = f;
  cur->fd_size++;

  // 다음에 사용할 fd를 업데이트
  cur->fd_nxt = assigned_fd + 1 < 128 ? assigned_fd + 1 : 128;

  return assigned_fd; // 할당된 fd 반환
}

void close(int fd)
{
  struct thread *cur = thread_current();

  // 파일 디스크립터 유효성 검사
  if (fd < 0 || fd >= 128 || cur->fd[fd] == NULL)
    // 유효하지 않은 파일 디스크립터인 경우 프로세스 종료
    exit(-1);

  // 파일 닫기
  file_close(cur->fd[fd]);
  // 파일 디스크립터 테이블에서 제거
  cur->fd[fd] = NULL;
  // 열린 파일 디스크립터 수 감소
  cur->fd_size--;
  // 다음 사용할 파일 디스크립터 번호 업데이트
  if (cur->fd_nxt > fd)
    cur->fd_nxt = fd;
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
  case SYS_CREATE:
    validate_user_addr(esp + 1);
    validate_user_addr(esp + 2);
    f->eax = create((char *)*(uint32_t *)(esp + 1),
                    (unsigned)*(uint32_t *)(esp + 2));
    break;

  case SYS_REMOVE:
    validate_user_addr(esp + 1);
    f->eax = remove((char *)*(uint32_t *)(esp + 1));
    break;

  case SYS_OPEN:
    validate_user_addr(esp + 1);
    f->eax = open((char *)*(uint32_t *)(esp + 1));
    break;

  case SYS_FILESIZE:
    validate_user_addr(esp + 1);
    f->eax = filesize((int)*(uint32_t *)(esp + 1));
    break;

  case SYS_SEEK:
    validate_user_addr(esp + 1);
    validate_user_addr(esp + 2);
    seek((int)*(uint32_t *)(esp + 1),
         (unsigned)*(uint32_t *)(esp + 2));
    break;

  case SYS_TELL:
    validate_user_addr(esp + 1);
    f->eax = tell((int)*(uint32_t *)(esp + 1));
    break;

  case SYS_CLOSE:
    validate_user_addr(esp + 1);
    close((int)*(uint32_t *)(esp + 1));
    break;
  default:
    exit(-1);
  }
}