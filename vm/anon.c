/* anon.c: Implementation of page for non-disk image (a.k.a. anonymous page). */

#include "vm/vm.h"
#include "vm/page.h"
#include "devices/disk.h"
#include "threads/malloc.h" //free()함수 쓰기 위해
#include "threads/mmu.h" //pml4_clear_page()함수 쓰기 위해

/* DO NOT MODIFY BELOW LINE */
static struct disk *swap_disk;
static bool anon_swap_in (struct page *page, void *kva);
static bool anon_swap_out (struct page *page);
static void anon_destroy (struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations anon_ops = {
	.swap_in = anon_swap_in,
	.swap_out = anon_swap_out,
	.destroy = anon_destroy,
	.type = VM_ANON,
};

/* Initialize the data for anonymous pages */
void vm_anon_init (void) {
	/* TODO: Set up the swap_disk. */
	swap_disk = disk_get(1, 1); //disk.c에 있는 disk_get 함수로 swap은 1,1로 선언하라는 가이드가 있음
	if (swap_disk == NULL) { //오류 처리
        PANIC("No swap disk found!");
    }
    swap_bitmap = bitmap_create(disk_size(swap_disk) / SECTORS_PER_PAGE); //swap_disk size에 맞는 bitmap 선언
	if (swap_bitmap == NULL) { //오류 처리
        PANIC("Failed to initialize swap table!");
    }
}

/* Initialize the file mapping */
bool anon_initializer(struct page *page, enum vm_type type, void *kva) {
    ASSERT(page != NULL);
    ASSERT(VM_TYPE(type) == VM_ANON);

	page->operations = &anon_ops; //original code
    struct anon_page *anon_page = &page->anon; //original code

    anon_page->swap_slot = BITMAP_ERROR;  // 초기값 설정, 이것만 해주면 되는지 의문

    return true;
}

/* Swap in the page by read contents from the swap disk. */
static bool
anon_swap_in (struct page *page, void *kva) {
    struct anon_page *anon_page = &page->anon; //original code
	lock_acquire(&swap_lock); //swap in 과 swap out은 시작과 끝에 lock을 해줘야함
    if (anon_page->swap_slot == BITMAP_ERROR) { 
		lock_release(&swap_lock);
        return false;  // No valid swap slot
    }
    // Read the contents from the swap disk into the allocated frame
    for (size_t i = 0; i < SECTORS_PER_PAGE; i++) {
        disk_read(swap_disk, anon_page->swap_slot * SECTORS_PER_PAGE + i, kva + i * DISK_SECTOR_SIZE);
    }
    // Mark the swap slot as free
    bitmap_reset(swap_bitmap, anon_page->swap_slot);
	lock_release(&swap_lock);
    anon_page->swap_slot = BITMAP_ERROR;  // swap slot reset을 어떻게 해야할지 고민해봐야함. 

    return true;
}

/* Swap out the page by writing contents to the swap disk. */
static bool
anon_swap_out (struct page *page) {
	struct anon_page *anon_page = &page->anon; //original code
    lock_acquire(&swap_lock);
    //bitmap을 통해 비어있는 swap_slot을 찾아준다. (처음 발견되는 빈 공간)
    size_t swap_slot = bitmap_scan_and_flip(swap_bitmap, 0, 1, false);
    if (swap_slot == BITMAP_ERROR) { //비어있는 swap_slot을 찾지 못한 경우
        lock_release(&swap_lock);
        return false; 
    }
    // 물리메모리에 있는 page의 컨텐츠를 swap disk에 write (복사)
    void *frame = page->frame->kva;
    for (size_t i = 0; i < SECTORS_PER_PAGE; i++) {
        disk_write(swap_disk, swap_slot * SECTORS_PER_PAGE + i, frame + i * DISK_SECTOR_SIZE);
    }
    anon_page->swap_slot = swap_slot;

    frame_free(page->frame); 
    lock_release(&swap_lock);
    return true;
}

/* Destroy the anonymous page. PAGE will be freed by the caller. */
static void anon_destroy (struct page *page) {
    struct anon_page *anon_page = &page->anon; //original code

    if (anon_page->swap_slot != BITMAP_ERROR) { //
        bitmap_reset(swap_bitmap, anon_page->swap_slot);
        anon_page->swap_slot = BITMAP_ERROR;
    }
    if (page->frame != NULL) { //page가 frame 갖고 있으면 frame_free
        lock_acquire(&swap_lock);
        frame_free(page->frame);
		lock_release(&swap_lock);
    }
}

void frame_free(struct frame *frame) { //고민해봐야하긴함
    // if (frame == NULL) {
    //     return;
    //     }
    // list_remove(&frame->frame_elem); //frame_list에서 해당 frame 삭제
    // pml4_clear_page(thread_current()->pml4, frame->page->va); //pml4를 통한 va와 물리주소 연결 삭제
    // palloc_free_page(frame->page); //아니면 frame->kva???
    // free(frame);                  //frame structure 자체를 제거, metadata들을 삭제 malloc으로 frame을 선언했다.
    return;
}