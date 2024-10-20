/* anon.c: Implementation of page for non-disk image (a.k.a. anonymous page). */

#include "vm/vm.h"
#include "devices/disk.h"
#include "bitmap.h"

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
	swap_disk = disk_get(1, 1);	//1번째 디스크의 1번째 파티션에서 스왑 디스크를 가져옴
	swap_table = bitmap_create(disk_size(swap_disk) / DISK_SECTOR_SIZE);
	//disk_size: 스왑 디스크의 총 섹터 수를 페이지당 섹터 수로 나누어 스왑 가능한 페이지의 개수를 계산
	//bitmap_create : 스왑 가능한 페이지의 개수만큼 비트맵을 생성하여 스왑 테이블을 관리
}

/* Initialize the file mapping */
bool
anon_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	page->operations = &anon_ops;

	struct anon_page *anon_page = &page->anon;

	anon_page->sector = NULL;
	page->swap_slot = NULL;
}

/* Swap in the page by read contents from the swap disk. */
static bool
anon_swap_in(struct page *page, void *kva) {
    // 1. 스왑 슬롯에서 데이터 읽기
    struct anon_page *anon_page = &page->anon;
    size_t swap_slot = page->swap_slot;

    if (swap_slot == BITMAP_ERROR) {
        return false;
    }

    disk_read(swap_disk, swap_slot, kva);

    bitmap_set(swap_table, swap_slot, false); // 스왑 슬롯 해제

	page->accessed = 1;

    return vm_do_claim_page(page);
}

/* Swap out the page by writing contents to the swap disk. */
static bool
anon_swap_out(struct page *page) {
    struct anon_page *anon_page = &page->anon;  // 익명 페이지 접근

    size_t swap_slot = bitmap_scan_and_flip(swap_table, 0, 1, false); //swap_table에서 빈 스왑 슬롯 찾고 할당

    if (swap_slot == BITMAP_ERROR) {
		return NULL;
	}

	disk_write(swap_disk, swap_slot, page->va);

	page->swap_slot = swap_slot;

	bitmap_set(swap_table, swap_slot, true);
	
	list_remove(&page->frame->frame_elem);
	page->frame = NULL;
	free(page->frame);

	pml4_clear_page(thread_current()->pml4, page->va);

	page->accessed = 0;


	return false;
}

/* Destroy the anonymous page. PAGE will be freed by the caller. */
static void
anon_destroy (struct page *page) {
	struct anon_page *anon_page = &page->anon;
	

}
