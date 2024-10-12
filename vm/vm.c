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

/* Helpers */
static struct frame *vm_get_victim (void);
static bool vm_do_claim_page (struct page *page);
static struct frame *vm_evict_frame (void);

/* Create the pending page object with initializer. If you want to create a
 * page, do not create it directly and make it through this function or
 * `vm_alloc_page`. */
bool
vm_alloc_page_with_initializer (enum vm_type type, void *upage, bool writable,
		vm_initializer *init, void *aux) {

	ASSERT (VM_TYPE(type) != VM_UNINIT)

	struct supplemental_page_table *spt = &thread_current ()->spt;

	/* Check wheter the upage is already occupied or not. */
	if (spt_find_page (spt, upage) == NULL) {
		/* TODO: Create the page, fetch the initialier according to the VM type,
		 * TODO: and then create "uninit" page struct by calling uninit_new. You
		 * TODO: should modify the field after calling the uninit_new. */

		/* TODO: Insert the page into the spt. */
	}
err:
	return false;
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



	
}

/* Free the resource hold by the supplemental page table */
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	//process_exit에서 사용
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
	
	return hash_destroy(&spt->spt, page_destructor);	
}

void page_destructor(struct hash_elem *e, void *aux) {
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

