#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/gdt.h"
#include "threads/flags.h"
#include "intrinsic.h"

#include "userprog/process.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "threads/palloc.h"

#include "threads/synch.h"
#include <string.h>

typedef uint32_t disk_sector_t;

struct file {
	struct inode *inode;        /* File's inode. */
	off_t pos;                  /* Current position. */
	bool deny_write;            /* Has file_deny_write() been called? */
};

struct inode_disk {
	disk_sector_t start;                /* First data sector. */
	off_t length;                       /* File size in bytes. */
	unsigned magic;                     /* Magic number. */
	uint32_t unused[125];               /* Not used. */
};

struct inode {
	struct list_elem elem;              /* Element in inode list. */
	disk_sector_t sector;               /* Sector number of disk location. */
	int open_cnt;                       /* Number of openers. */
	bool removed;                       /* True if deleted, false otherwise. */
	int deny_write_cnt;                 /* 0: writes ok, >0: deny writes. */
	struct inode_disk data;             /* Inode content. */
};

void syscall_entry (void);
void syscall_handler (struct intr_frame *);
bool user_memory_valid(void *r);
struct file *get_file_by_descriptor(int fd);
struct lock syscall_lock;

/* System call.
 *
 * Previously system call services was handled by the interrupt handler
 * (e.g. int 0x80 in linux). However, in x86-64, the manufacturer supplies
 * efficient path for requesting the system call, the `syscall` instruction.
 *
 * The syscall instruction works by reading the values from the the Model
 * Specific Register (MSR). For the details, see the manual. */

#define MSR_STAR 0xc0000081         /* Segment selector msr */
#define MSR_LSTAR 0xc0000082        /* Long mode SYSCALL target */
#define MSR_SYSCALL_MASK 0xc0000084 /* Mask for the eflags */

void
syscall_init (void) {
	write_msr(MSR_STAR, ((uint64_t)SEL_UCSEG - 0x10) << 48  |
			((uint64_t)SEL_KCSEG) << 32);
	write_msr(MSR_LSTAR, (uint64_t) syscall_entry);

	/* The interrupt service rountine should not serve any interrupts
	 * until the syscall_entry swaps the userland stack to the kernel
	 * mode stack. Therefore, we masked the FLAG_FL. */
	write_msr(MSR_SYSCALL_MASK,
			FLAG_IF | FLAG_TF | FLAG_DF | FLAG_IOPL | FLAG_AC | FLAG_NT);

	lock_init(&syscall_lock);
}

/* The main system call interface */
void
syscall_handler (struct intr_frame *f UNUSED) {
	// TODO: Your implementation goes here.
	// printf("\n------- syscall handler -------\n");
	uint64_t arg1 = f->R.rdi;
	uint64_t arg2 = f->R.rsi;
	uint64_t arg3 = f->R.rdx;
	uint64_t arg4 = f->R.r10;
	uint64_t arg5 = f->R.r8;
	uint64_t arg6 = f->R.r9;
	switch (f->R.rax)
	{
		case SYS_HALT:							//  0 운영체제 종료
			// RPL(Requested Privilege Level) : cs의 하위 2비트
			if ((f->cs & 0x3) != 0){}
				// 권한 없음
			// printf("SYS_HALT\n");
			halt();
		case SYS_EXIT:							//  1 프로세스 종료
			// printf("SYS_EXIT\n");
			exit(arg1);
			break;
		case SYS_FORK:							//  2 프로세스 복제
			// printf("SYS_FORK\n");
			// f->R.rax=fork(arg1);
			f->R.rax = fork(arg1, f);		//(oom_update)
			break;
		case SYS_EXEC:							//  3 새로운 프로그램 실행
			// printf("SYS_EXEC\n");
			check_valid_string((const char *)arg1);
			f->R.rax=exec(arg1);
			break;
		case SYS_WAIT:							//  4 자식 프로세스 대기
			// printf("SYS_WAIT\n");
			f->R.rax=wait(arg1);
			break;
		case SYS_CREATE:						//  5 파일 생성
			// printf("SYS_CREATE\n");
			check_valid_string((const char *)arg1);
			f->R.rax=create(arg1,arg2);
			break;
		case SYS_REMOVE:						//  6 파일 삭제
			// printf("SYS_REMOVE\n");
			check_valid_string((const char *)arg1);
			f->R.rax=remove(arg1);
			break;
		case SYS_OPEN:							//  7 파일 열기
			// printf("SYS_OPEN\n");
			check_valid_string((const char *)arg1);
			f->R.rax=open(arg1);
			break;
		case SYS_FILESIZE:						//  8 파일 크기 조회
			// printf("SYS_FILESIZE\n");
			f->R.rax=filesize(arg1);
			break;
		case SYS_READ:							//  9 파일에서 읽기
			// printf("SYS_READ\n");
			check_valid_buffer(f->R.rsi, f->R.rdx, f->rsp, 1);
			// check_address((void *)arg2);
			f->R.rax = read(f->R.rdi, f->R.rsi, f->R.rdx);
			break;
		case SYS_WRITE:							//  10 파일에 쓰기
			// printf("SYS_WRITE\n");
			check_valid_buffer(f->R.rsi, f->R.rdx, f->rsp, 0);
			f->R.rax=write((int)arg1,(void *)arg2,(unsigned)arg3);
			break;
		case SYS_SEEK:							//  11 파일 내 위치 변경
			// printf("SYS_SEEK\n");
			seek(arg1,arg2);
			break;
		case SYS_TELL:							//  12 파일의 현재 위치 반환
			// printf("SYS_TELL\n");
			f->R.rax=tell(arg1);
			break;
		case SYS_CLOSE:							//  13 파일 닫기
			// printf("SYS_CLOSE\n");
			close(arg1);
			break;
		default:
			// printf("default;\n");
			break;
	}
	// printf("-------------------------------\n\n");
}

void halt (void){
	power_off();
}

void exit (int status){
	struct thread *curr = thread_current();
	curr->process_status = status;
	printf("%s: exit(%d)\n", curr->name, status);
	thread_exit();
}

pid_t fork (const char *thread_name, struct intr_frame *f) {	//(oom_update)
	return process_fork(thread_name, f);
}

int exec (const char *cmd_line){
	char *copy = palloc_get_page(PAL_ZERO);
	if (copy == NULL) {
		exit(-1);
	}
	strlcpy(copy, cmd_line, strlen(cmd_line) + 1);
	if (process_exec (copy) < 0) {
		exit(-1);
	}
	NOT_REACHED();
}

int wait (pid_t pid){
	return process_wait(pid);
}

bool create (const char *file, unsigned initial_size){
	lock_acquire(&syscall_lock);		//minjae's
	bool create_return = filesys_create(file, initial_size);
	lock_release(&syscall_lock);		//minjae's
	return create_return;
}

bool remove (const char *file){
	return filesys_remove(file);
}

int open (const char *file) {	//(oom_update)
	lock_acquire(&syscall_lock);
	struct file *f = filesys_open(file);
	if (f == NULL){
		lock_release(&syscall_lock);
		return -1;
	}
	struct thread *curr = thread_current();
	struct file **fdt = curr->fd_table;

	while (curr->next_fd < FD_MAX && fdt[curr->next_fd])
		curr->next_fd++;

	if (curr->next_fd >= FD_MAX) {
		file_close (f);
		lock_release(&syscall_lock);
		return -1;
	}

	fdt[curr->next_fd] = f;
	lock_release(&syscall_lock);
	return curr->next_fd;
}

int filesize (int fd){
	struct file *file = get_file_by_descriptor(fd);
	return file_length(file);
}

int read (int fd, void *buffer, unsigned size){
	if (fd == STD_IN) {                // keyboard로 직접 입력
		int i;  // 쓰레기 값 return 방지
		char c;
		unsigned char *buf = buffer;

		for (i = 0; i < size; i++) {
			c = input_getc();
			*buf++ = c;
			if (c == '\0')
				break;
		}
		return i;
	}
    struct file *file = get_file_by_descriptor(fd);
	if (file == NULL || fd == STD_OUT || fd == STD_ERR)  // 빈 파일, stdout, stderr를 읽으려고 할 경우
		return -1;

	off_t bytes = -1;
	lock_acquire(&syscall_lock);
	bytes = file_read(file, buffer, size);
	lock_release(&syscall_lock);

	return bytes;
}


int write (int fd, const void *buffer, unsigned size){
	if (fd == STD_IN || fd == STD_ERR){
		return -1;
	}

	if (fd == STD_OUT){
		putbuf(buffer, size);
		return size;
	}

	struct file *file = get_file_by_descriptor(fd);
	if (file == NULL){
		return -1;
	}

	lock_acquire(&syscall_lock);
	int written = file_write(file, buffer, size);
	lock_release(&syscall_lock);

	return written;
}

void seek (int fd, unsigned position){
	if (fd < 3)
		return;

	struct file *file = get_file_by_descriptor(fd);
	if (file == NULL){
		return;
	}

	file_seek(file, position);
}

unsigned tell (int fd){
	if (fd < 3)
		return -1;

	struct file *file = get_file_by_descriptor(fd);
	if (file == NULL){
		return -1;
	}

	return file_tell(file);
}

void close (int fd){	//(oom_update)
	if (fd <= 2)
		return;
	struct thread *curr = thread_current ();
	struct file *f = curr->fd_table[fd];

	if (f == NULL){
		return;
	}
	curr->fd_table[fd] = NULL;
	curr->next_fd = 3;

	file_close(f);
}


bool user_memory_valid(void *r) {
    return r != NULL && is_user_vaddr(r);
}


struct page * check_address(void *addr) {
    if (is_kernel_vaddr(addr))
    {
        exit(-1);
    }
    return spt_find_page(&thread_current()->spt, addr);
}

void check_valid_buffer(void* buffer, unsigned size, void* rsp, bool to_write) {
    for (int i = 0; i < size; i++) {
        struct page* page = check_address(buffer + i);    // 인자로 받은 buffer부터 buffer + size까지의 크기가 한 페이지의 크기를 넘을수도 있음
        if(page == NULL)
            exit(-1);
        if(to_write == true && page->writable == false)
            exit(-1);
    }
}

void check_valid_string(const char *str) {
    while (true) {
        check_address((const void *)str);
        if (*str == '\0') {
            break;
        }
        str++;
    }
}

struct file *get_file_by_descriptor(int fd)
{
	if (fd < 3 || fd > 128)
		return NULL;
	struct thread *t = thread_current();
	return t->fd_table[fd];
}