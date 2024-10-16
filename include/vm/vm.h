#ifndef VM_VM_H
#define VM_VM_H
#include <stdbool.h>
#include "threads/palloc.h"
#include "hash.h"
#include "list.h"

enum vm_type {
	/* page not initialized 페이지가 초기화 되지 않음*/
	VM_UNINIT = 0,
	/* page not related to the file, aka anonymous page 파일과 관련이 없는 페이지, 익명 페이지*/
	VM_ANON = 1,
	/* page that realated to the file 파일과 관련된 페이지*/
	VM_FILE = 2,
	/* page that hold the page cache, for project 4 페이지 캐시를 보유하는 페이지*/
	VM_PAGE_CACHE = 3,

	/* Bit flags to store state 상태를 저장하는 플래그 비트*/

	/* Auxillary bit flag marker for store information. You can add more
	 * markers, until the value is fit in the int. 
	 * 추가적인 정보를 저장하기 위한 비트 플래그. 원하는 만큼 추가 가능*/
	VM_MARKER_0 = (1 << 3),
	VM_MARKER_1 = (1 << 4),

	/* DO NOT EXCEED THIS VALUE. 더 이상 추가하지 말아야 할 값*/
	VM_MARKER_END = (1 << 31),
};

#include "vm/uninit.h"
#include "vm/anon.h"
#include "vm/file.h"
#include "vm/page.h"
#ifdef EFILESYS
#include "filesys/page_cache.h"
#endif

struct page_operations;
struct thread;

#define VM_TYPE(type) ((type) & 7)

/* The representation of "page".
 * This is kind of "parent class", which has four "child class"es, which are
 * uninit_page, file_page, anon_page, and page cache (project4).
 * DO NOT REMOVE/MODIFY PREDEFINED MEMBER OF THIS STRUCTURE. */
struct page {
	const struct page_operations *operations;	//파악못함
	void *va;              /* Address in terms of user space */
	struct frame *frame;   /* Back reference for frame */

	/* Your implementation */
	bool writable;
	// struct file *file;
	enum vm_type type;
	size_t swap_slot;	//스왑 할 때 필요
	struct file *file;	//파일 자체에 대한 포인터 추가
	struct hash_elem elem;	//hash_table elem
	bool is_loaded; //물리 메모리의 탑재 여부, 아직 역할 모름.
	struct list_elem mmap_elem;
	
	/* Per-type data are binded into the union.
	 * Each function automatically detects the current union */
	union {		//스왑할 때 필요
		struct uninit_page uninit;
		struct anon_page anon;
		struct file_page file_page;	//file backed page. page 구조체의 file 포인터와 구분하기 위해 file_page로 변경
#ifdef EFILESYS
		struct page_cache page_cache;
#endif
	};
};

// struct vme{
// 	enum vm_type type;			/*VM_BIN, VM_FILE, VM_ANON의 타입*/
// 	void *va;			/*vm_entry가 관리하는 가상페이지 번호*/
// 	bool writable;			/*True일 경우 해당 주소에 write 가능
// 							False일 경우 해당 주소에 write 불가능*/

// 	bool is_loaded;			/*물리메모리의 탑재 여부를 알려주는 플래그*/
// 	struct file *file;		/*가상주소와 맵핑된 파일*/

// 	/*Memory Mapped File 에서 다룰 예정*/
// 	struct list_elem mmap_elem;		/*mmap 리스트 element*/

// 	size_t offset;			/*읽어야 할 파일 오프셋*/
// 	size_t read_bytes;		/*가상페이지에 쓰여져 있는 데이터 크기*/
// 	size_t zero_bytes;		/*0으로 채울 남은 페이지의 바이트*/

// 	/*Swapping 과제에서 다룰 예정*/
// 	size_t swap_slot;		/*스왑 슬롯*/
// 	struct page *page;

// 	/*'vm_entry들을 위한 자료구조' 부분에서 다룰 예정*/
// 	struct hash_elem elem;		/*해시 테이블 Element*/
// };

/* The representation of "frame" */
struct frame {
	void *kva;	//커널  가상 주소
	struct page *page;	//페이지 구조체를 담기 위한 멤버
};

/* The function table for page operations.
 * This is one way of implementing "interface" in C.
 * Put the table of "method" into the struct's member, and
 * call it whenever you needed. */
struct page_operations {
	bool (*swap_in) (struct page *, void *);
	bool (*swap_out) (struct page *);
	void (*destroy) (struct page *);
	enum vm_type type;
};

#define swap_in(page, v) (page)->operations->swap_in ((page), v)
#define swap_out(page) (page)->operations->swap_out (page)
#define destroy(page) \
	if ((page)->operations->destroy) (page)->operations->destroy (page)

/* Representation of current process's memory space.
 * We don't want to force you to obey any specific design for this struct.
 * All designs up to you for this. */
struct supplemental_page_table {
	struct hash spt;
};

#include "threads/thread.h"
void supplemental_page_table_init (struct supplemental_page_table *spt);
bool supplemental_page_table_copy (struct supplemental_page_table *dst,
		struct supplemental_page_table *src);
void supplemental_page_table_kill (struct supplemental_page_table *spt);
struct page *spt_find_page (struct supplemental_page_table *spt,
		void *va);
bool spt_insert_page (struct supplemental_page_table *spt, struct page *page);
void spt_remove_page (struct supplemental_page_table *spt, struct page *page);

void vm_init (void);
bool vm_try_handle_fault (struct page *p);

#define vm_alloc_page(type, upage, writable) \
	vm_alloc_page_with_initializer ((type), (upage), (writable), NULL, NULL)
bool vm_alloc_page_with_initializer (enum vm_type type, void *upage,
		bool writable, vm_initializer *init, void *aux);
void vm_dealloc_page (struct page *page);
bool vm_claim_page (void *va);

enum vm_type page_get_type (struct page *page);
static bool page_init(struct page *page);

struct page *page_create(enum vm_type type, void *va, bool writable, struct file *file, 
						off_t offset, size_t read_bytes, size_t zero_bytes);
void page_destructor(struct hash_elem *e, void *aux);
struct page *page_copy(struct page *original_page);


#endif  /* VM_VM_H */
