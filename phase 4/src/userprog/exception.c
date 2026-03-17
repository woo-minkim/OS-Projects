#include "userprog/exception.h"
#include <inttypes.h>
#include <stdio.h>
#include "userprog/gdt.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"

/*phase 4 do it*/
#include "userprog/process.h"
#include "vm/page.h"
#include "vm/frame.h"
#include "vm/swap.h"
/*phase 4 do it*/

/* Number of page faults processed. */
static long long page_fault_cnt;

static void kill(struct intr_frame *);
static void page_fault(struct intr_frame *);

void exception_init(void)
{
   /* These exceptions can be raised explicitly by a user program,
      e.g. via the INT, INT3, INTO, and BOUND instructions.  Thus,
      we set DPL==3, meaning that user programs are allowed to
      invoke them via these instructions. */
   intr_register_int(3, 3, INTR_ON, kill, "#BP Breakpoint Exception");
   intr_register_int(4, 3, INTR_ON, kill, "#OF Overflow Exception");
   intr_register_int(5, 3, INTR_ON, kill,
                     "#BR BOUND Range Exceeded Exception");

   /* These exceptions have DPL==0, preventing user processes from
      invoking them via the INT instruction.  They can still be
      caused indirectly, e.g. #DE can be caused by dividing by
      0.  */
   intr_register_int(0, 0, INTR_ON, kill, "#DE Divide Error");
   intr_register_int(1, 0, INTR_ON, kill, "#DB Debug Exception");
   intr_register_int(6, 0, INTR_ON, kill, "#UD Invalid Opcode Exception");
   intr_register_int(7, 0, INTR_ON, kill,
                     "#NM Device Not Available Exception");
   intr_register_int(11, 0, INTR_ON, kill, "#NP Segment Not Present");
   intr_register_int(12, 0, INTR_ON, kill, "#SS Stack Fault Exception");
   intr_register_int(13, 0, INTR_ON, kill, "#GP General Protection Exception");
   intr_register_int(16, 0, INTR_ON, kill, "#MF x87 FPU Floating-Point Error");
   intr_register_int(19, 0, INTR_ON, kill,
                     "#XF SIMD Floating-Point Exception");

   /* Most exceptions can be handled with interrupts turned on.
      We need to disable interrupts for page faults because the
      fault address is stored in CR2 and needs to be preserved. */
   intr_register_int(14, 0, INTR_OFF, page_fault, "#PF Page-Fault Exception");
}

/* Prints exception statistics. */
void exception_print_stats(void)
{
   printf("Exception: %lld page faults\n", page_fault_cnt);
}

/* Handler for an exception (probably) caused by a user process. */
static void
kill(struct intr_frame *f)
{
   /* This interrupt is one (probably) caused by a user process.
      For example, the process might have tried to access unmapped
      virtual memory (a page fault).  For now, we simply kill the
      user process.  Later, we'll want to handle page faults in
      the kernel.  Real Unix-like operating systems pass most
      exceptions back to the process via signals, but we don't
      implement them. */

   /* The interrupt frame's code segment value tells us where the
      exception originated. */
   switch (f->cs)
   {
   case SEL_UCSEG:
      /* User's code segment, so it's a user exception, as we
         expected.  Kill the user process.  */
      printf("%s: dying due to interrupt %#04x (%s).\n",
             thread_name(), f->vec_no, intr_name(f->vec_no));
      intr_dump_frame(f);
      thread_exit();

   case SEL_KCSEG:
      /* Kernel's code segment, which indicates a kernel bug.
         Kernel code shouldn't throw exceptions.  (Page faults
         may cause kernel exceptions--but they shouldn't arrive
         here.)  Panic the kernel to make the point.  */
      intr_dump_frame(f);
      PANIC("Kernel bug - unexpected interrupt in kernel");

   default:
      /* Some other code segment?  Shouldn't happen.  Panic the
         kernel. */
      printf("Interrupt %#04x (%s) in unknown segment %04x\n",
             f->vec_no, intr_name(f->vec_no), f->cs);
      thread_exit();
   }
}

/* Page fault handler.  This is a skeleton that must be filled in
   to implement virtual memory.  Some solutions to project 2 may
   also require modifying this code.

   At entry, the address that faulted is in CR2 (Control Register
   2) and information about the fault, formatted as described in
   the PF_* macros in exception.h, is in F's error_code member.  The
   example code here shows how to parse that information.  You
   can find more information about both of these in the
   description of "Interrupt 14--Page Fault Exception (#PF)" in
   [IA32-v3a] section 5.15 "Exception and Interrupt Reference". */
static void
page_fault(struct intr_frame *f)
{
   bool not_present = (f->error_code & PF_P) == 0; // 페이지가 메모리에 없는 경우(true)
   bool write = (f->error_code & PF_W) != 0;       // 쓰기 접근인지
   bool user = (f->error_code & PF_U) != 0;        // 사용자 모드에서 발생한 페이지 폴트인지
   void *fault_addr;

   // CR2 레지스터에서 fault_addr(페이지 폴트 발생 주소)을 가져옴
   asm("movl %%cr2, %0" : "=r"(fault_addr));

   // 인터럽트 다시 활성화하고, 페이지 폴트 횟수 증가
   intr_enable();
   page_fault_cnt++;

   /*phase 4 do it*/
   // 기본 검증 1: fault_addr는 사용자 영역이어야 하며,
   // 기본 검증 2: 페이지 폴트 원인이 '페이지 없음(not_present)'이어야 한다.
   if (!is_user_vaddr(fault_addr) || !not_present)
      exit(-1);

   // 현재 fault_addr에 대응하는 virtual_page를 찾는다.
   struct virtual_page *fault_vpage = find_virtual_page(fault_addr);

   if (fault_vpage != NULL)
   {
      // 일반적인 페이지 폴트 처리 경로:
      // 메모리 로드 핸들러 시도 → 성공하면 return, 실패하면 exit(-1)
      if (memory_fault_handler(fault_vpage) || fault_vpage->is_loaded)
         return; // 로드 성공 → 함수 종료
      exit(-1);  // 로드 실패 → 프로세스 종료
   }

   // 여기까지 왔다면 fault_vpage를 찾지 못한 상황.
   // 즉, 스택 영역 확장을 고려해야 하는 케이스이다.

   // 스택 확장 조건 확인:
   // 1. fault_addr가 스택 포인터보다 일정 범위(32바이트) 내에 있어야 한다.
   if (fault_addr < f->esp - 32)
      exit(-1);

   // 2. fault_addr를 페이지 경계로 정렬하고,
   //    최대 스택 크기를 초과하지 않는지 확인한다.
   void *rounded_addr = pg_round_down(fault_addr);
   if ((size_t)(PHYS_BASE - rounded_addr) > MAX_STACK_SIZE)
      exit(-1);

   // 새로운 vm_entry 생성
   struct virtual_page *new_vpage = vm_entry_create(rounded_addr);
   if (new_vpage == NULL)
      exit(-1);

   // 해당 vpage를 위한 실제 물리 페이지 할당 및 매핑
   struct page *stack_page = page_create(new_vpage);
   if (stack_page == NULL)
   {
      free(new_vpage);
      exit(-1);
   }

   // vm 해시 테이블에 삽입 실패 시 모든 자원 해제 후 종료
   if (!insert_virtual_page(&thread_current()->vm, new_vpage))
   {
      release_page(stack_page->kernel_addr);
      free(stack_page);
      free(new_vpage);
      exit(-1);
   }

   // 스택 확장 성공 시 추가 로직 없이 정상 종료
   // 여기서 반환하지 않아도 함수 끝에 도달하면 return
}
/*phase 4 do it*/