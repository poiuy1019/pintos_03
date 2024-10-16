/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "threads/vaddr.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "hash.h"

static uint64_t spt_hash_func(const struct hash_elem *e, void *aux);
static bool spt_less_func(const struct hash_elem *a, const struct hash_elem *b, void *aux);

#include "userprog/syscall.h" 
#include "vm/uninit.h"

/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes. */
void
vm_init (void) {
	vm_anon_init ();
	vm_file_init ();
#ifdef EFILESYS  /* For project 4 */
	pagecache_init ();
#endif
	register_inspect_intr ();
	/* DO NOT MODIFY UPPER LINES. */
	/* TODO: Your code goes here. */
}

/* Get the type of the page. This function is useful if you want to know the
 * type of the page after it will be initialized.
 * This function is fully implemented now. */
enum vm_type
page_get_type (struct page *page) {
	int ty = VM_TYPE (page->operations->type);
	switch (ty) {
		case VM_UNINIT:
			return VM_TYPE (page->uninit.type);
		default:
			return ty;
	}
}

static bool page_init(struct page *page) {

}

/* Helpers */
static struct frame *vm_get_victim (void);
static bool vm_do_claim_page (struct page *page);
static struct frame *vm_evict_frame (void);

// /* Create the pending page object with initializer. If you want to create a
//  * page, do not create it directly and make it through this function or
//  * `vm_alloc_page`. */
// bool
// vm_alloc_page_with_initializer (enum vm_type type, void *upage, bool writable,
// 		vm_initializer *init, void *aux) {

// 	ASSERT (VM_TYPE(type) != VM_UNINIT)

// 	struct supplemental_page_table *spt = &thread_current ()->spt;

// 	/* Check wheter the upage is already occupied or not. */
// 	if (spt_find_page (spt, upage) == NULL) {
// 		/* TODO: Create the page, fetch the initialier according to the VM type,
// 		 * TODO: and then create "uninit" page struct by calling uninit_new. You
// 		 * TODO: should modify the field after calling the uninit_new. */

// 		/* TODO: Insert the page into the spt. */
// 	}
// err:
// 	return false;
// }

bool vm_alloc_page_with_initializer (enum vm_type type, void *upage, bool writable,
        vm_initializer *init, void *aux) {

    ASSERT (VM_TYPE(type) != VM_UNINIT); //original code

    struct supplemental_page_table *spt = &thread_current ()->spt; //original code

    /* Check whether the upage is already occupied or not. */
    if (spt_find_page (spt, upage) == NULL) {  //original code
        struct page *page = malloc(sizeof(struct page)); 
        if (!page) goto err; //습관적 예외처리

        bool (*page_initializer)(struct page *, enum vm_type, void *) = NULL; //page_initializer 선언

        //page_initializer 를 type에 맞는 initializer로 설정
        if (VM_TYPE(type) == VM_ANON) {
            page_initializer = anon_initializer; 
        } else if (VM_TYPE(type) == VM_FILE) {
            page_initializer = file_backed_initializer; //아직 미지의 영역
        } else {
            goto err; // type이 anon도 file도 아닌 경우
        }

        uninit_new(page, upage, init, type, aux, page_initializer); //lazy loading 하는 사람이 잘 이해할듯

        page->writable = writable; // ??? read & write write default값으로 설정해주는 게 맞다.

        return spt_insert_page(spt, page); //생성된 page를 spt에 insert
    }

err: //original code
    return false; //original code
}

/* Find VA from spt and return page. On error, return NULL. */
struct page *
spt_find_page (struct supplemental_page_table *spt, void *va) {
	struct page *page = NULL;
	/* TODO: Fill this function. */
	
	struct page want_page;
	want_page.va = va;
	struct hash_elem *found_elem = hash_find(&spt->spt, &want_page.elem);
	if (found_elem == NULL) {
		return NULL;
	}

	page = hash_entry(found_elem, struct page, elem);
	
	return page;
}

/* Insert PAGE into spt with validation. */
bool
spt_insert_page (struct supplemental_page_table *spt ,
		struct page *page) {
	int succ = false;
	/* TODO: Fill this function. */

	ASSERT(spt != NULL);
	ASSERT(page != NULL);

	struct hash_elem *result = hash_insert(&spt->spt, &page->elem);

	if(result != NULL) {
		return succ;
	} else {
		succ = true;
	}

	return succ;
}

void
spt_remove_page (struct supplemental_page_table *spt, struct page *page) {
	hash_delete(&spt->spt, &page->elem);
	vm_dealloc_page (page);

	return true;
}

/* Get the struct frame, that will be evicted. */
static struct frame *
vm_get_victim (void) {
	struct frame *victim = NULL;
	 /* TODO: The policy for eviction is up to you. */

	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame (void) {
	struct frame *victim UNUSED = vm_get_victim ();
	/* TODO: swap out the victim and return the evicted frame. */

	return NULL;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/
static struct frame *vm_get_frame(void) {
    struct frame *frame = malloc(sizeof(struct frame));
    if (frame == NULL) {
        return NULL;  // 메모리 할당 실패 시 NULL 반환
    }

    // 물리 메모리 페이지 할당
    frame->kva = palloc_get_page(PAL_USER | PAL_ZERO);
    if (frame->kva == NULL) {
        free(frame);
        return NULL;
    }

    // 페이지 프레임 초기화
    frame->page = NULL;
    return frame;
}

/* Growing the stack. */
static void
vm_stack_growth (void *addr UNUSED) {
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp (struct page *page UNUSED) {
}

/* Return true on success */
bool
vm_try_handle_fault (struct page *p) {

	/* TODO: Your code goes here */
	// 1. page(frame) allocation
	// 2. data load file(disk) -> memory
	// 3. page table set up
	return vm_do_claim_page (p);
}

/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void
vm_dealloc_page (struct page *page) {
	destroy (page);
	free (page);
}

bool vm_claim_page(void *va) {
    struct thread *t = thread_current();
    struct page *page = spt_find_page(&t->spt, va);  // 주어진 가상 주소에 대한 페이지를 찾음

    // 페이지가 존재하지 않으면 에러
    if (page == NULL) {
        return false;
    }

    // 페이지를 클레임하고 페이지 테이블을 설정
    return vm_do_claim_page(page);
}


static bool vm_do_claim_page(struct page *page) {
    // 물리 메모리 프레임을 할당
    struct frame *frame = vm_get_frame();
    if (frame == NULL) {
        return false;  // 프레임 할당 실패 시 false 반환
    }

    // 프레임과 페이지를 연결
    frame->page = page;
    page->frame = frame;

    // 페이지 테이블에 페이지를 맵핑
    if (!pml4_set_page(thread_current()->pml4, page->va, frame->kva, page->writable)) {
        // 페이지 테이블 설정 실패 시 물리 메모리 해제
        vm_dealloc_page(page);
        return false;
    }

    // 성공적으로 맵핑되었으면 스왑 혹은 파일에서 데이터를 로드
    if (page->type == VM_FILE || page->type == VM_ANON) {
        if (!load_file(frame->kva, page)) {
            // 스왑이나 파일 로드 실패 시 메모리 해제
            vm_dealloc_page(page);
            return false;
        }
    }

    return true;
}

/* Initialize new supplemental page table */
void
supplemental_page_table_init (struct supplemental_page_table *spt UNUSED) {
	ASSERT(spt != NULL);

	return hash_init(&spt->spt, spt_hash_func, spt_less_func, NULL);
}

/* Copy supplemental page table from src to dst */
bool
supplemental_page_table_copy (struct supplemental_page_table *dst UNUSED,
		struct supplemental_page_table *src UNUSED) {
	//do_fork에서 사용
	struct hash_iterator page_iterator;	//for문의 i와 같은 역할
	hash_first(&page_iterator, &src->spt);	//page_iterator가 해시 테이블의 첫 번쨰 요소를 가리킴

	while (hash_next(&page_iterator)) {		//모든 버킷에 걸쳐 해시 테이블의 모든 요소를 순차적으로 탐색
		struct page *original_page = hash_entry(hash_cur(&page_iterator), struct page, elem);

		//페이지 복사
		struct page *new_page = page_copy(original_page);
		if (new_page == NULL) {
			return false;
		}

		//자식 프로세스의 SPT에 새로운 페이지 삽입
		if (!hash_insert(&dst->spt, &new_page->elem)) {
			free(new_page);
			return false;
		}
	}
	return true;
}

/* Free the resource hold by the supplemental page table */
void
supplemental_page_table_kill (struct supplemental_page_table *spt) {
	//process_exit에서 사용
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
	if (!hash_empty(&spt->spt)) {
		hash_destroy(&spt->spt, page_destructor);
	}
}

void page_destructor(struct hash_elem *e, void *aux UNUSED) {
	struct page *page = hash_entry(e, struct page, elem);
	vm_dealloc_page(page);
}

/*Hash function for the supplemental page table(SPT)*/
static uint64_t
spt_hash_func(const struct hash_elem *e, void *aux) {
	/*e는 해시 테이블의 요소로, 가상 주소(va)를 해시하여 반환*/
	const struct page *page = hash_entry(e, struct page, elem);
	return hash_int(page->va); 
}


/*Comparison function for the supplemental page table(SPT)*/
static bool
spt_less_func(const struct hash_elem *a, const struct hash_elem *b, void *aux) {
	/*a와 b는 각각 해시 테이블 요소로, 가상 주소를 비교하여 반환*/
	const struct page *page_a = hash_entry(a, struct page, elem);
	const struct page *page_b = hash_entry(b, struct page, elem);
	return page_a->va < page_b->va;
}

struct page *page_create(enum vm_type type, void *va, bool writable, struct file *file, 
						off_t offset, size_t read_bytes, size_t zero_bytes) {

	struct page *new_page = malloc(sizeof(struct page));
	if (new_page == NULL) {
		return NULL;
	}				

	new_page->type = type;
	new_page->va = va;
	new_page->writable = writable;
	new_page->is_loaded = false;

	if (type == VM_FILE) {
		new_page->file = file;
		new_page->file_page.offset = offset;
		new_page->file_page.read_bytes = read_bytes;
		new_page->file_page.zero_bytes = zero_bytes;
	}

	return new_page;
}

struct page *page_copy(struct page *original_page) {
	//페이지 타입 확인
	enum vm_type type = original_page->type;
	bool writable = original_page->writable;
	void *va = original_page->va;

	struct file *file = NULL;
	off_t offset = 0;
	size_t read_bytes = 0;
	size_t zero_bytes = 0;
	
	//file_backed일 경우, 파일 관련 정보 복사
	if (type == VM_FILE) {
		file = original_page->file;
		offset = original_page->file_page.offset;
		read_bytes = original_page->file_page.read_bytes;
		zero_bytes = original_page->file_page.zero_bytes;
	}

	struct page *new_page = page_create(type, va, writable, file, offset, read_bytes, zero_bytes);

	//생성된 페이지 반환
	return new_page;
}
