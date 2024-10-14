/* anon.c: Implementation of page for non-disk image (a.k.a. anonymous page). */

#include "vm/vm.h"
#include "devices/disk.h"

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
void
vm_anon_init (void) {
	/* TODO: Set up the swap_disk. */
	swap_disk = NULL;
}

/* Initialize the file mapping */
bool
anon_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	page->operations = &anon_ops;

	struct anon_page *anon_page = &page->anon;
}

/* Swap in the page by read contents from the swap disk. */
static bool
anon_swap_in (struct page *page, void *kva) {
	struct anon_page *anon_page = &page->anon;
}

/* Swap out the page by writing contents to the swap disk. */
static bool
anon_swap_out (struct page *page) {
	struct anon_page *anon_page = &page->anon;
}

/* Destroy the anonymous page. PAGE will be freed by the caller. */
static void
anon_destroy (struct page *page) {
	struct anon_page *anon_page = &page->anon;

    // if (page->frame != NULL) { 
    //     lock_acquire(&frame_table_lock);
    //     list_remove(&page->frame->frame_elem);
	// 	lock_release(&frame_table_lock);
	// 	frame_free(page->frame);
    // }
}

// void frame_free(struct frame *frame) { //고민해봐야하긴함
//     if (frame == NULL) {
//         return;
//         }
//     list_remove(&frame->frame_elem); //frame_list에서 해당 frame 삭제
//     pml4_clear_page(thread_current()->pml4, frame->page->va); //pml4를 통한 va와 물리주소 연결 삭제
//     palloc_free_page(frame->kva); //아니면 frame->kva???
// 	free(frame->page);
//     free(frame);                  //frame structure 자체를 제거, metadata들을 삭제 malloc으로 frame을 선언했다.
//     return;
// }