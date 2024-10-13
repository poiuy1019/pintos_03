/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "threads/vaddr.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "hash.h"
#include "threads/mmu.h"

static uint64_t spt_hash_func(const struct hash_elem *e, void *aux);
static bool spt_less_func(const struct hash_elem *a, const struct hash_elem *b, void *aux);

#include "userprog/syscall.h" 
#include "userprog/process.h" 
static struct list frame_list;  
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
	list_init(&frame_list);
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
struct page *spt_find_page (struct supplemental_page_table *spt, void *va) {
	struct page *page = NULL;
	/* TODO: Fill this function. */
	
	struct page *want_page;
	want_page->va = pg_round_down(va); 
	struct hash_elem *found_elem = hash_find(&spt->spt_hash, &want_page->hash_elem);
	if (found_elem == NULL) {
		return NULL;
	}

	page = hash_entry(found_elem, struct page, hash_elem);
	
	return page;
}

/* Insert PAGE into spt with validation. */
bool
spt_insert_page (struct supplemental_page_table *spt, struct page *page) {
	if(hash_insert(&spt->spt_hash, &page->hash_elem))
	return false;
	else
	return true;
}
void
spt_remove_page (struct supplemental_page_table *spt, struct page *page) {
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
static struct frame *
vm_get_frame (void) {
	struct frame *frame = NULL;
	// TODO: victim page
	// frame malloc 으로 할당 후 page, kva 할당
	frame = malloc(sizeof(struct frame));
	ASSERT(frame != NULL);

	frame->kva = palloc_get_page(PAL_USER | PAL_ZERO); 
	if (frame->kva == NULL)
        frame = vm_evict_frame();  
    else
        list_push_back(&frame_list, &frame->frame_elem);

    frame->page = NULL;

	ASSERT (frame->page == NULL);
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
vm_try_handle_fault (struct intr_frame *f UNUSED, void *addr,
		bool user UNUSED, bool write UNUSED, bool not_present UNUSED) {
	struct supplemental_page_table *spt = &thread_current ()->spt;
	struct page *page = NULL;
		
	/* TODO: Your code goes here */
	// 1. page(frame) allocation
	// 2. data load file(disk) -> memory
	// 3. page table set up
	return vm_do_claim_page (page);
}

bool check_validation_of_fault_addr (void *fault_addr, struct supplemental_page_table *spt) {
	 // 주소가 사용자 영역에 있는지 확인고 아니면 exit || 주소가 프로세스의 주소 공간 내에 있는지 확인 || 주소가 이미 매핑되어 있지 않은지 확인 || 주소가 유효한 vm_entry에 해당하는지 확인 (SPT에서 검색)
	user_memory_valid(fault_addr);
	struct page *find_page = spt_find_page(spt, fault_addr);
    if (fault_addr == NULL || !is_user_vaddr(fault_addr) || fault_addr >= KERN_BASE || find_page == NULL) {
		exit(-1);
        return false;
    }
}


/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void
vm_dealloc_page (struct page *page) {
	destroy (page);
	free (page);
}

/* Claim the page that allocate on VA. */
bool
vm_claim_page (void *va UNUSED) {
	// struct page *page = NULL;
	/* TODO: Fill this function */
	struct page *page = spt_find_page(&thread_current()->spt, va);

    if (page == NULL)
        return false;
	return vm_do_claim_page (page);
}

/* Claim the PAGE and set up the mmu. */
static bool
vm_do_claim_page (struct page *page) {
	struct frame *frame = vm_get_frame ();

	/* Set links */
	frame->page = page;
	page->frame = frame;

	/* TODO: Insert page table entry to map page's VA to frame's PA. */
	if (!pml4_set_page(thread_current()->pml4, page->va, frame->kva, page->writable))
        return false;

	return swap_in (page, frame->kva);
}

/* Initialize new supplemental page table */
void
supplemental_page_table_init (struct supplemental_page_table *spt UNUSED) {
	hash_init(spt, spt_hash_func, spt_less_func, NULL);
}

/* Copy supplemental page table from src to dst */
bool
supplemental_page_table_copy (struct supplemental_page_table *dst UNUSED,
		struct supplemental_page_table *src UNUSED) {
}

/* Free the resource hold by the supplemental page table */
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
}

/*Hash function for the supplemental page table(SPT)*/
static uint64_t
spt_hash_func(const struct hash_elem *e, void *aux) {
	/*e는 해시 테이블의 요소로, 가상 주소(va)를 해시하여 반환*/
	const struct page *page = hash_entry(e, struct page, hash_elem);
	return hash_int(page->va); 
}


/*Comparison function for the supplemental page table(SPT)*/
static bool
spt_less_func(const struct hash_elem *a, const struct hash_elem *b, void *aux) {
	/*a와 b는 각각 해시 테이블 요소로, 가상 주소를 비교하여 반환*/
	const struct page *page_a = hash_entry(a, struct page, hash_elem);
	const struct page *page_b = hash_entry(b, struct page, hash_elem);
	return page_a->va < page_b->va;
}

