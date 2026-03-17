#include "userprog/process.h"
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "userprog/gdt.h"
#include "userprog/pagedir.h"
#include "userprog/tss.h"
#include "filesys/directory.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/flags.h"
#include "threads/init.h"
#include "threads/interrupt.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
/*phase 4 do it*/
#include "vm/page.h"
#include "vm/frame.h"
#include "vm/swap.h"
/*phase 4 do it*/

/* load() helpers. */
static bool install_page(void *upage, void *kpage, bool writable);

static thread_func start_process NO_RETURN;
static bool load(const char *cmdline, void (**eip)(void), void **esp);

struct virtual_page *vm_entry_create(void *addr)
{
  // 메모리 할당
  struct virtual_page *vm_entry = malloc(sizeof(struct virtual_page));
  if (vm_entry == NULL)
    return NULL;

  // 구조체를 0으로 초기화한 뒤 필요한 값 설정
  memset(vm_entry, 0, sizeof(struct virtual_page));

  vm_entry->base_vaddr = addr;
  vm_entry->is_loaded = true;
  vm_entry->writable = true;
  vm_entry->type = false;
  return vm_entry;
}

struct page *page_create(struct virtual_page *vm_entry)
{
  // vm_entry가 NULL일 경우 로직이 성립하지 않으므로 ASSERT
  ASSERT(vm_entry != NULL);

  // PAL_USER 속성으로 페이지를 할당
  struct page *new_page = alloc_page(PAL_USER);
  bool install_success = false;

  if (new_page == NULL)
    goto cleanup; // 페이지 할당 실패

  new_page->vm_entry = vm_entry;

  // 가상 주소(base_vaddr)에 커널 페이지(kernel_addr)를 설치
  install_success = install_page(vm_entry->base_vaddr, new_page->kernel_addr, vm_entry->writable);
  if (!install_success)
    goto cleanup; // 설치 실패

  // 모든 과정 성공 시 new_page 반환
  return new_page;

cleanup:
  // 할당한 페이지가 있다면 해제
  if (new_page && new_page->kernel_addr)
    release_page(new_page->kernel_addr);

  // new_page 구조체 자체도 free
  if (new_page)
    free(new_page);

  return NULL;
}

/*phase 4 do it*/

/*phase 1. do it*/
// 1. file 이름 처리
// 2. 프로그램 파일 존재 확인
// 3. 스레드 생성
tid_t process_execute(const char *file_name)
{
  char *fn_copy;
  tid_t tid;
  struct thread *t;
  struct list_elem *cur_elem;

  /* Make a copy of FILE_NAME.
     Otherwise there's a race between the caller and load(). */
  fn_copy = palloc_get_page(0);
  if (fn_copy == NULL)
    return TID_ERROR;
  strlcpy(fn_copy, file_name, PGSIZE); // 4KB 할당

  /*do it*/
  int len = strlen(fn_copy);
  // 추출할 파일 이름
  char *parsed_name = (char *)malloc(sizeof(char) * (len + 1));

  if (parsed_name == NULL)
  {
    palloc_free_page(fn_copy);
    return TID_ERROR;
  }

  // 공백 skip
  int i = 0;
  while (fn_copy[i] == ' ')
    i++;

  // 파일 이름만 추출
  int j = 0;
  while (fn_copy[i] != ' ' && fn_copy[i] != '\0')
    parsed_name[j++] = fn_copy[i++];
  parsed_name[j] = '\0'; // NULL 추가

  if (!filesys_open(parsed_name))
    return TID_ERROR;
  else
  {
    /* Create a new thread to execute the file */
    tid = thread_create(parsed_name, PRI_DEFAULT, start_process, fn_copy);
    sema_down(&(thread_current()->load_sema));

    if (tid == TID_ERROR)
      palloc_free_page(fn_copy);
    /*phase 2 do it*/
    for (cur_elem = list_begin(&(thread_current()->child_List)); cur_elem != list_end(&(thread_current()->child_List)); cur_elem = list_next(cur_elem))
    {
      t = list_entry(cur_elem, struct thread, child_List_Elem);
      if (!(t->load_status))
      {
        return process_wait(tid);
      }
    }
    /*phase 2 do it*/
    return tid;
  }
}

/*phase 4 do it*/
static unsigned vm_entry_hash(const struct hash_elem *e, void *aux UNUSED)
{
  // hash_elem로부터 virtual_page 구조체 추출
  struct virtual_page *vpage = hash_entry(e, struct virtual_page, elem);
  // 가상 주소를 정수형으로 변환
  uintptr_t key_addr = (uintptr_t)vpage->base_vaddr;
  // 주소 기반 해시 계산
  unsigned key_hash = hash_bytes(&key_addr, sizeof(key_addr));

  return key_hash;
}

static bool vm_entry_less(const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED)
{
  struct virtual_page *vpage_a = hash_entry(a, struct virtual_page, elem);
  struct virtual_page *vpage_b = hash_entry(b, struct virtual_page, elem);

  uintptr_t addr_a = (uintptr_t)vpage_a->base_vaddr;
  uintptr_t addr_b = (uintptr_t)vpage_b->base_vaddr;

  bool result = (addr_a < addr_b);
  return result;
}
/*phase 4 do it*/

/* A thread function that loads a user process and starts it
   running. */
static void
start_process(void *file_name_)
{
  char *file_name = file_name_;
  struct intr_frame if_;
  bool success;

  /*phase 4 do it*/
  hash_init(&(thread_current()->vm), vm_entry_hash, vm_entry_less, NULL);

  /* Initialize interrupt frame and load executable. */
  memset(&if_, 0, sizeof if_);
  if_.gs = if_.fs = if_.es = if_.ds = if_.ss = SEL_UDSEG;
  if_.cs = SEL_UCSEG;
  if_.eflags = FLAG_IF | FLAG_MBS;
  success = load(file_name, &if_.eip, &if_.esp);

  /* If load failed, quit. */
  sema_up(&(thread_current()->parent->load_sema));
  palloc_free_page(file_name);
  if (!success)
  {
    thread_current()->load_status = false;
    exit(-1);
  }
  /* Start the user process by simulating a return from an
     interrupt, implemented by intr_exit (in
     threads/intr-stubs.S).  Because intr_exit takes all of its
     arguments on the stack in the form of a struct intr_frame',
     we just point the stack pointer (%esp) to our stack frame
     and jump to it. */
  asm volatile("movl %0, %%esp; jmp intr_exit" : : "g"(&if_) : "memory");
  NOT_REACHED();
}

/* Waits for thread TID to die and returns its exit status.  If
   it was terminated by the kernel (i.e. killed due to an
   exception), returns -1.  If TID is invalid or if it was not a
   child of the calling process, or if process_wait() has already
   been successfully called for the given TID, returns -1
   immediately, without waiting.

   This function will be implemented in problem 2-2.  For now, it
   does nothing. */
int process_wait(tid_t child_tid UNUSED)
{
  /*phase1 do it*/
  struct list_elem *child_elem;
  struct thread *child_thread = NULL;
  struct thread *parent_thread = thread_current(); // 현재 스레드(부모)

  // 유효하지 않은 tid인 경우 -1 반환
  if (child_tid == TID_ERROR)
    return -1;

  // 현재 스레드의 자식 리스트에서 child_tid에 해당하는 자식 스레드 찾기
  for (child_elem = list_begin(&(parent_thread->child_List));
       child_elem != list_end(&(parent_thread->child_List));
       child_elem = list_next(child_elem))
  {
    // 자식 스레드 얻기
    child_thread = list_entry(child_elem, struct thread, child_List_Elem);
    if (child_tid == child_thread->tid)
    {
      /* 자식 스레드가 종료될 때까지 기다리기 -> 후에 process_exit에서 풀림 */
      sema_down(&(child_thread->exit_sema));

      /* 부모가 자식 스레드의 종료 상태 처리 후, 자식에게 완료 신호 보내기
        이때 자식 프로세스의 메모리가 남아 있어야 함.*/

      /*자식의 종료 상태 수집*/
      int status = child_thread->exit_status;

      /*자식 리스트에서 제거*/
      list_remove(&(child_thread->child_List_Elem));

      /*자식 프로세스가 종료할 수 있도록 신호*/
      sema_up(&(child_thread->remove_sema));

      /*자식 스레드의 종료 상태 반환*/
      return status;
    }
  }
  // 해당하는 자식 스레드 못 찾은 경우 -1 반환
  return -1;

  /*phase 1 do it*/
}

/*phase 4 do it*/
static void destroy_vm(struct hash_elem *elem, void *aux UNUSED)
{
  struct virtual_page *vpage = hash_entry(elem, struct virtual_page, elem);
  uint32_t *pd = thread_current()->pagedir;

  // 페이지 로드되어 있으면 언매핑 처리
  if (vpage->is_loaded)
  {
    void *kpage = pagedir_get_page(pd, vpage->base_vaddr);
    if (kpage != NULL)
      release_page(kpage);

    pagedir_clear_page(pd, vpage->base_vaddr);
  }

  free(vpage);
}
/*phase 4 do it*/

/* Free the current process's resources. */
void process_exit(void)
{
  struct thread *cur = thread_current();
  uint32_t *pd;

  /*phase 4 do it*/
  hash_destroy(&cur->vm, destroy_vm);
  /*phase 4 do it*/
  /* 현재 프로세스의 페이지 디렉토리를 파괴하고
     커널 전용 페이지 디렉토리로 전환한다. */
  pd = cur->pagedir;
  if (pd != NULL)
  {
    /* Correct ordering here is crucial.  We must set
       cur->pagedir to NULL before switching page directories,
       so that a timer interrupt can't switch back to the
       process page directory.  We must activate the base page
       directory before destroying the process's page
       directory, or our active page directory will be one
       that's been freed (and cleared). */
    cur->pagedir = NULL;
    pagedir_activate(NULL);
    pagedir_destroy(pd);
  }
  /*phase 4 do it*/
  for (int i = 2; i < 128; i++)
  {
    if (cur->fd[i] != NULL)
      file_close(cur->fd[i]);
  }
  /*phase 4 do it*/

  /*phase 1 do it*/
  /*부모에게 종료를 알림*/
  sema_up(&(cur->exit_sema));
  /*부모가 종료 상태를 수집할 때까지 대기 -> 후에 process_wait 함수에서 풀림*/
  sema_down(&(cur->remove_sema));
  /*phase 1 do it*/
}

/* Sets up the CPU for running user code in the current
   thread.
   This function is called on every context switch. */
void process_activate(void)
{
  /* phase 1 do it
  pagedir_activate와 tss_update함수는 사용자 프로그램의 주소 공간 관리와 관련된 함수.
    이 함수들은 USERPROG가 정의되어 있을 때만 존재하고 컴파일 되므로 #ifdef USERPROG를 사용할 필요 없음.
  */
  struct thread *t = thread_current();

  /* Activate thread's page tables. */
  pagedir_activate(t->pagedir);

  /* Set thread's kernel stack for use in processing
     interrupts. */
  tss_update();
}

/* We load ELF binaries.  The following definitions are taken
   from the ELF specification, [ELF1], more-or-less verbatim.  */

/* ELF types.  See [ELF1] 1-2. */
typedef uint32_t Elf32_Word, Elf32_Addr, Elf32_Off;
typedef uint16_t Elf32_Half;

/* For use with ELF types in printf(). */
#define PE32Wx PRIx32 /* Print Elf32_Word in hexadecimal. */
#define PE32Ax PRIx32 /* Print Elf32_Addr in hexadecimal. */
#define PE32Ox PRIx32 /* Print Elf32_Off in hexadecimal. */
#define PE32Hx PRIx16 /* Print Elf32_Half in hexadecimal. */

/* Executable header.  See [ELF1] 1-4 to 1-8.
   This appears at the very beginning of an ELF binary. */
struct Elf32_Ehdr
{
  unsigned char e_ident[16];
  Elf32_Half e_type;
  Elf32_Half e_machine;
  Elf32_Word e_version;
  Elf32_Addr e_entry;
  Elf32_Off e_phoff;
  Elf32_Off e_shoff;
  Elf32_Word e_flags;
  Elf32_Half e_ehsize;
  Elf32_Half e_phentsize;
  Elf32_Half e_phnum;
  Elf32_Half e_shentsize;
  Elf32_Half e_shnum;
  Elf32_Half e_shstrndx;
};

/* Program header.  See [ELF1] 2-2 to 2-4.
   There are e_phnum of these, starting at file offset e_phoff
   (see [ELF1] 1-6). */
struct Elf32_Phdr
{
  Elf32_Word p_type;
  Elf32_Off p_offset;
  Elf32_Addr p_vaddr;
  Elf32_Addr p_paddr;
  Elf32_Word p_filesz;
  Elf32_Word p_memsz;
  Elf32_Word p_flags;
  Elf32_Word p_align;
};

/* Values for p_type.  See [ELF1] 2-3. */
#define PT_NULL 0           /* Ignore. */
#define PT_LOAD 1           /* Loadable segment. */
#define PT_DYNAMIC 2        /* Dynamic linking info. */
#define PT_INTERP 3         /* Name of dynamic loader. */
#define PT_NOTE 4           /* Auxiliary info. */
#define PT_SHLIB 5          /* Reserved. */
#define PT_PHDR 6           /* Program header table. */
#define PT_STACK 0x6474e551 /* Stack segment. */

/* Flags for p_flags.  See [ELF3] 2-3 and 2-4. */
#define PF_X 1 /* Executable. */
#define PF_W 2 /* Writable. */
#define PF_R 4 /* Readable. */

static bool setup_stack(void **esp);
static bool validate_segment(const struct Elf32_Phdr *, struct file *);
static bool load_segment(struct file *file, off_t ofs, uint8_t *upage,
                         uint32_t read_bytes, uint32_t zero_bytes,
                         bool writable);

/* Loads an ELF executable from FILE_NAME into the current thread.
   Stores the executable's entry point into *EIP
   and its initial stack pointer into *ESP.
   Returns true if successful, false otherwise. */
bool load(const char *file_name, void (**eip)(void), void **esp)
{
  /*phase 1 do it
    pagedir_create, process_activate, setup_stack모두 그 자체가 사용자 프로그램의
    로드 및 실행과 관련된 함수 이므로 #ifdef USERPROG 사용 안해도 괜찮음.
  */
  struct thread *t = thread_current();
  struct Elf32_Ehdr ehdr;
  struct file *file = NULL;
  off_t file_ofs;
  bool success = false;
  int i;

  /* Allocate and activate page directory. */
  t->pagedir = pagedir_create();
  if (t->pagedir == NULL)
    goto done;
  process_activate();

  /* Open executable file. */

  /*phase1 do it*/
  const char *fn_copy = file_name;
  int argc = 0;
  char **argv = NULL;
  // 길이 계산
  int len = strlen(fn_copy);

  // 1. 인자 갯수 계산
  int in_word = 0; // 현재 문자가 단어의 일부인지 추적
  for (int i = 0; i <= len; i++)
  {
    if (fn_copy[i] == ' ' || fn_copy[i] == '\0')
    {
      if (in_word) // in_word가 1이면 단어의 끝을 의미
      {
        argc++;
        in_word = 0;
      }
    }
    else
      in_word = 1;
  }
  // argv 배열 할당
  argv = (char **)malloc(sizeof(char *) * argc);

  // 2. 인자 추출 및 저장
  int arg_idx = 0;   // argv 배열에서 현재 인자가 저장될 인덱스
  int start_idx = 0; // 현재 단어의 시작 인덱스

  in_word = 0;                   // 현재 문자가 단어의 일부인지 아닌지를 나타내는 플래그
  for (int i = 0; i <= len; i++) // 문자열 끝인 '\0'까지 포함 순회
  {

    if (fn_copy[i] == ' ' || fn_copy[i] == '\0')
    {
      if (in_word) // 만약 해당 단어의 끝이면
      {
        // 단어의 길이 계산 (i(현재 인덱스)-start_idx(단어의 시작 인덱스))
        int word_len = i - start_idx;
        argv[arg_idx] = (char *)malloc(word_len + 1);

        // fn_copy의 start_idx부터 word_len만큼의 문자를 argv[arg_idx]에 복사
        strlcpy(argv[arg_idx], &fn_copy[start_idx], word_len + 1);
        argv[arg_idx][word_len] = '\0';
        arg_idx++;
        in_word = 0;
      }
    }
    // 현재 문자가 단어의 일부인 경우 (주로 새 단어의 시작)
    else
    {
      if (!in_word)
      {
        start_idx = i;
        in_word = 1;
      }
    }
  }
  /*parsing end*/

  file = filesys_open(argv[0]);
  if (file == NULL)
  {
    printf("load: %s: open failed\n", file_name);
    goto done;
  }

  /* Read and verify executable header. */
  if (file_read(file, &ehdr, sizeof ehdr) != sizeof ehdr || memcmp(ehdr.e_ident, "\177ELF\1\1\1", 7) || ehdr.e_type != 2 || ehdr.e_machine != 3 || ehdr.e_version != 1 || ehdr.e_phentsize != sizeof(struct Elf32_Phdr) || ehdr.e_phnum > 1024)
  {
    printf("load: %s: error loading executable\n", file_name);
    goto done;
  }

  /* Read program headers. */
  file_ofs = ehdr.e_phoff;
  for (i = 0; i < ehdr.e_phnum; i++)
  {
    struct Elf32_Phdr phdr;

    if (file_ofs < 0 || file_ofs > file_length(file))
      goto done;
    file_seek(file, file_ofs);

    if (file_read(file, &phdr, sizeof phdr) != sizeof phdr)
      goto done;
    file_ofs += sizeof phdr;
    switch (phdr.p_type)
    {
    case PT_NULL:
    case PT_NOTE:
    case PT_PHDR:
    case PT_STACK:
    default:
      /* Ignore this segment. */
      break;
    case PT_DYNAMIC:
    case PT_INTERP:
    case PT_SHLIB:
      goto done;
    case PT_LOAD:
      if (validate_segment(&phdr, file))
      {
        bool writable = (phdr.p_flags & PF_W) != 0;
        uint32_t file_page = phdr.p_offset & ~PGMASK;
        uint32_t mem_page = phdr.p_vaddr & ~PGMASK;
        uint32_t page_offset = phdr.p_vaddr & PGMASK;
        uint32_t read_bytes, zero_bytes;
        if (phdr.p_filesz > 0)
        {
          /* Normal segment.
             Read initial part from disk and zero the rest. */
          read_bytes = page_offset + phdr.p_filesz;
          zero_bytes = (ROUND_UP(page_offset + phdr.p_memsz, PGSIZE) - read_bytes);
        }
        else
        {
          /* Entirely zero.
             Don't read anything from disk. */
          read_bytes = 0;
          zero_bytes = ROUND_UP(page_offset + phdr.p_memsz, PGSIZE);
        }
        if (!load_segment(file, file_page, (void *)mem_page,
                          read_bytes, zero_bytes, writable))
          goto done;
      }
      else
        goto done;
      break;
    }
  }
  /*전체 흐름:
    1. 파일열기 -> 2. ELF 헤더 읽기 및 검증 -> 3. 프로그램 헤더 순회 (세그먼트 타입 확인)
    -> 4. 세그먼트 로드
    즉, 이 과정은 운영체제에 의해 수행되며, 프로그램이 메모리에 로드되고 실행되기 위한 준비 단계.
    사용자 프로세스 주소 공간에서 메모리 할당과 초기화가 이루어지는 것이다.
    * 세그먼트: 실행 시 필요한 정보를 담고 있으며, 프로그램 헤더 테이블에 정의
    즉, 세그먼트는 프로그램 실행 시 메모리에 로드되어야 하는 코드와 데이터를 그룹화 한 것.
  */
  /* Set up stack. */
  if (!setup_stack(esp))
    goto done;

  /*phase 1 do it*/
  /*스택 설정 및 argument passsing 과정*/
  /*1. 인자 문자열들을 스택에 역순으로 push*/
  for (i = argc - 1; i >= 0; i--)
  {
    int len = strlen(argv[i]) + 1; // '\0' 포함
    *esp -= len;                   // 스택 포인터 감소
    memcpy(*esp, argv[i], len);    // 스택에 문자열 복사
    argv[i] = *esp;                // argv[i]에 스택 내 문자열 주소 저장
  }

  /*2. 워드 정렬을 위해 패딩 추가*/
  uintptr_t esp_uint = (uintptr_t)(*esp); // 스택 포인터를 정수형으로 변환
  int padding = (4 - (esp_uint % 4)) % 4;
  if (padding != 0)
  {
    *esp -= padding;
    memset(*esp, 0, padding);
  } // 스택 포인터를 4의 배수로 정렬

  /*3. argv[argc]에 대한 NULL포인터 push*/
  *esp -= sizeof(char *);
  *(uint32_t *)(*esp) = NULL;

  /*4. argv[0~argc-1]의 주소들을 스택에 역순으로 push*/
  for (i = argc - 1; i >= 0; i--)
  {
    *esp -= 4;
    *(uint32_t *)(*esp) = (uint32_t)argv[i];
  }

  /*5. argv의 시작 주소를 스택에 push*/
  char **argv_addr = (char **)*esp; // 현재 스택 포인터는 argv[0] 주소를 가리킴
  *esp -= sizeof(char **);
  *(uint32_t *)(*esp) = (uint32_t)argv_addr;

  /*6. argc 값을 스택에 push*/
  *esp -= sizeof(int);
  *(int *)(*esp) = argc;

  /*7. 가짜 return 주소 push*/
  *esp -= sizeof(void *);
  *(uint32_t *)(*esp) = NULL; // 프로그램 시작 시에는 리턴 주소가 의미 없음

  free(argv);
  /*passing done*/

  // printf("hex dump in construct_stack start\n\n");
  // hex_dump(*esp, *esp, 100, true);
  /* Start address. */
  *eip = (void (*)(void))ehdr.e_entry;

  success = true;

done:
  /* We arrive here whether the load is successful or not. */
  file_close(file);
  return success;
}

/*phase 4 do it*/
bool memory_fault_handler(struct virtual_page *vm_entry)
{
  // 이미 메모리에 로드되어 있다면 추가 로드 필요 없음
  if (vm_entry->is_loaded)
    return false;

  // 페이지 할당 시도
  struct page *page = alloc_page(PAL_USER);
  if (!page)
    return false;
  page->vm_entry = vm_entry;

  // 페이지 데이터 로드
  bool load_success = false;
  if (!vm_entry->type)
  {
    // 파일에서 페이지를 로드
    load_success = load_page_data(page->kernel_addr, vm_entry);
  }
  else
  {
    // 스왑에서 페이지 로드
    swap_load(vm_entry->swap_index, page->kernel_addr);
    load_success = true; // swap_load에서 실패에 대한 예외 케이스가 없다고 가정
  }

  // 로드 실패 시 정리 후 반환
  if (!load_success)
    goto error;

  // 페이지 테이블에 매핑 시도
  if (!install_page(vm_entry->base_vaddr, page->kernel_addr, vm_entry->writable))
    goto error;

  // 로드 성공
  vm_entry->is_loaded = true;
  return true;

error:
  release_page(page->kernel_addr);
  return false;
}
/*phase 4 do it*/

/* Checks whether PHDR describes a valid, loadable segment in
   FILE and returns true if so, false otherwise. */
static bool
validate_segment(const struct Elf32_Phdr *phdr, struct file *file)
{
  /* p_offset and p_vaddr must have the same page offset. */
  if ((phdr->p_offset & PGMASK) != (phdr->p_vaddr & PGMASK))
    return false;

  /* p_offset must point within FILE. */
  if (phdr->p_offset > (Elf32_Off)file_length(file))
    return false;

  /* p_memsz must be at least as big as p_filesz. */
  if (phdr->p_memsz < phdr->p_filesz)
    return false;

  /* The segment must not be empty. */
  if (phdr->p_memsz == 0)
    return false;

  /* The virtual memory region must both start and end within the
     user address space range. */
  if (!is_user_vaddr((void *)phdr->p_vaddr))
    return false;
  if (!is_user_vaddr((void *)(phdr->p_vaddr + phdr->p_memsz)))
    return false;

  /* The region cannot "wrap around" across the kernel virtual
     address space. */
  if (phdr->p_vaddr + phdr->p_memsz < phdr->p_vaddr)
    return false;

  /* Disallow mapping page 0.
     Not only is it a bad idea to map page 0, but if we allowed
     it then user code that passed a null pointer to system calls
     could quite likely panic the kernel by way of null pointer
     assertions in memcpy(), etc. */
  if (phdr->p_vaddr < PGSIZE)
    return false;

  /* It's okay. */
  return true;
}

/* Loads a segment starting at offset OFS in FILE at address
   UPAGE.  In total, READ_BYTES + ZERO_BYTES bytes of virtual
   memory are initialized, as follows:

        - READ_BYTES bytes at UPAGE must be read from FILE
          starting at offset OFS.

        - ZERO_BYTES bytes at UPAGE + READ_BYTES must be zeroed.

   The pages initialized by this function must be writable by the
   user process if WRITABLE is true, read-only otherwise.

   Return true if successful, false if a memory allocation error
   or disk read error occurs. */

/*phase 4 do it*/
static bool
load_segment(struct file *file, off_t ofs, uint8_t *upage,
             uint32_t read_bytes, uint32_t zero_bytes, bool writable)
{
  // 초기 조건 확인
  ASSERT((read_bytes + zero_bytes) % PGSIZE == 0);
  ASSERT(pg_ofs(upage) == 0);
  ASSERT(ofs % PGSIZE == 0);

  // 파일 포인터를 지정된 오프셋으로 이동
  file_seek(file, ofs);

  // 파일을 재오픈하여 백업 파일로 사용
  struct file *reopen_file = file_reopen(file);
  if (reopen_file == NULL)
    return false;

  // 현재 스레드의 VM 정보 가져오기
  struct thread *cur = thread_current();

  // 총 페이지 수 계산
  uint32_t total_pages = (read_bytes + zero_bytes) / PGSIZE;

  for (uint32_t page_index = 0; page_index < total_pages; page_index++)
  {
    size_t bytes_to_read;
    size_t bytes_to_zero;

    // 페이지당 읽을 바이트 계산
    if (read_bytes >= PGSIZE)
      bytes_to_read = PGSIZE;

    else
      bytes_to_read = read_bytes;

    // 페이지당 제로 바이트 계산
    bytes_to_zero = PGSIZE - bytes_to_read;

    // virtual_page 구조체 메모리 할당
    struct virtual_page *vpage = malloc(sizeof(struct virtual_page));
    if (vpage == NULL)
      goto error_exit;

    // virtual_page 필드 초기화
    vpage->type = false;
    vpage->is_loaded = false;
    vpage->backing_file = reopen_file;
    vpage->base_vaddr = upage;
    vpage->read_bytes = bytes_to_read;
    vpage->zero_bytes = bytes_to_zero;
    vpage->file_offset = ofs;
    vpage->writable = writable;

    // VM 해시 테이블에 virtual_page 삽입
    bool insert_success = insert_virtual_page(&cur->vm, vpage);
    if (!insert_success)
    {
      free(vpage);
      goto error_exit;
    }
    // 다음 페이지로 이동할 준비
    upage += PGSIZE;
    // 남은 바이트 업데이트
    if (read_bytes >= bytes_to_read)
      read_bytes -= bytes_to_read;
    else
      read_bytes = 0;
    if (zero_bytes >= bytes_to_zero)
      zero_bytes -= bytes_to_zero;
    else
      zero_bytes = 0;
    // 파일 오프셋 업데이트
    ofs += bytes_to_read;
  }
  // 성공적으로 모든 페이지를 로드함
  return true;

error_exit:
  // 오류 발생 시 재오픈한 파일 닫기
  file_close(reopen_file);
  return false;
}

/*phase 4 do it*/
static bool
setup_stack(void **esp)
{
  struct thread *cur = thread_current();

  // 사용자 공간용 페이지 할당
  struct page *pg = alloc_page(PAL_USER | PAL_ZERO);
  if (pg == NULL)
    return false;

  // 스택을 위한 가상 주소 계산
  void *vaddr = (uint8_t *)PHYS_BASE - PGSIZE;

  // 페이지 매핑 시도
  if (!install_page(vaddr, pg->kernel_addr, true))
    goto error_install;

  // 스택 포인터 설정
  *esp = PHYS_BASE;

  // virtual_page 구조체 할당
  struct virtual_page *vm_entry = malloc(sizeof(struct virtual_page));
  if (vm_entry == NULL)
    goto error_vm_entry;

  vm_entry->base_vaddr = pg_round_down(vaddr);
  vm_entry->is_loaded = true;
  vm_entry->writable = true;
  vm_entry->type = false;
  pg->vm_entry = vm_entry;

  // virtual_page를 해시 테이블에 삽입
  if (!insert_virtual_page(&cur->vm, vm_entry))
    goto error_insert_vm;

  // 모든 과정 성공
  return true;

error_insert_vm:
  free(vm_entry);
error_vm_entry:
  release_page(pg->kernel_addr);
error_install:
  return false;
}

/*phase 4 do it*/

/* Adds a mapping from user virtual address UPAGE to kernel
   virtual address KPAGE to the page table.
   If WRITABLE is true, the user process may modify the page;
   otherwise, it is read-only.
   UPAGE must not already be mapped.
   KPAGE should probably be a page obtained from the user pool
   with palloc_get_page().
   Returns true on success, false if UPAGE is already mapped or
   if memory allocation fails. */
static bool
install_page(void *upage, void *kpage, bool writable)
{
  struct thread *t = thread_current();

  /* Verify that there's not already a page at that virtual
     address, then map our page there. */
  return (pagedir_get_page(t->pagedir, upage) == NULL && pagedir_set_page(t->pagedir, upage, kpage, writable));
}
