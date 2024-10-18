/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"
#include "vm/file.h"
#include "threads/mmu.h"
#include "userprog/syscall.h"

static bool file_backed_swap_in (struct page *page, void *kva);
static bool file_backed_swap_out (struct page *page);
static void file_backed_destroy (struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations file_ops = {
	.swap_in = file_backed_swap_in,
	.swap_out = file_backed_swap_out,
	.destroy = file_backed_destroy,
	.type = VM_FILE,
};

/* The initializer of file vm */
void
vm_file_init (void) {
}

/* Initialize the file backed page */
bool file_backed_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	page->operations = &file_ops; //original code
	struct file_page *file_page = &page->file; //original code
	/* NOTE: The beginning where custom code is added */
    struct lazy_load_info *info = (struct lazy_load_info *)page->uninit.aux;
    file_page->file = info->file;
    file_page->offset = info->offset;
    file_page->page_read_bytes = info->page_read_bytes;
	file_page->page_zero_bytes = info->page_zero_bytes;
    return true;
	/* NOTE: The end where custom code is added */
}

/* Swap in the page by read contents from the file. */
static bool
file_backed_swap_in (struct page *page, void *kva) {
	struct file_page *file_page = &page->file; //original code
	
	/* NOTE: The beginning where custom code is added */

	/* NOTE: The end where custom code is added */

}

/* Swap out the page by writeback contents to the file. */
static bool
file_backed_swap_out (struct page *page) {
	struct file_page *file_page = &page->file; //original code

	/* NOTE: The beginning where custom code is added */

	/* NOTE: The end where custom code is added */
}

/* Destory the file backed page. PAGE will be freed by the caller. */
static void
file_backed_destroy (struct page *page) {
	struct file_page *file_page = &page->file; //original code
	/* NOTE: The beginning where custom code is added */
	if (pml4_is_dirty(thread_current()->pml4, page->va)) {
		file_seek(file_page->file, file_page->offset);
    	file_write(file_page->file, page->va, file_page->page_read_bytes);
        pml4_set_dirty(thread_current()->pml4, page->va, false);
    }
    pml4_clear_page(thread_current()->pml4, page->va);
	
	/* NOTE: The end where custom code is added */
	return 0;
}

static bool lazy_load_segment_mmap (struct page *page, void *aux) {
	/* TODO: Load the segment from the file */
	/* TODO: This called when the first page fault occurs on address VA. */
	/* TODO: VA is available when calling this function. */

	/* NOTE: The beginning where custom code is added */
	struct lazy_load_info *info = (struct lazy_load_info *)aux;
    struct file *file = info->file;
    off_t offset = info->offset;
    size_t page_read_bytes = info->page_read_bytes;
    size_t page_zero_bytes = info->page_zero_bytes;

	/* Allocate a physical frame */
    uint8_t *kva = page->frame->kva;

	/* Read from file */
    file_seek(file, offset);
    if (file_read(file, kva, page_read_bytes) != (int)page_read_bytes) {
        /* Handle read error */
        palloc_free_page(kva);
        return false;
    }

	/* Zero the remaining bytes */
    memset(kva + page_read_bytes, 0, page_zero_bytes);
    return true;
	/* NOTE: The end where custom code is added */
}

/* Do the mmap */
void *
do_mmap (void *addr, size_t length, int writable, struct file *file, off_t offset) {
	/* NOTE: The beginning where custom code is added */
	lock_acquire(&syscall_lock);
	struct file *mmap_file = file_reopen(file);
  	void * first_addr = addr;
  	size_t read_bytes = length > file_length(file) ? file_length(file) : length;
  	size_t zero_bytes = PGSIZE - (read_bytes % PGSIZE);
	
	while (read_bytes > 0 || zero_bytes > 0) {
		size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
		size_t page_zero_bytes = PGSIZE - page_read_bytes;

		struct lazy_load_info *info = (struct lazy_load_info*)malloc(sizeof(struct lazy_load_info));
		info->file = mmap_file;
		info->offset = offset;
		info->page_read_bytes = page_read_bytes;
		info->page_zero_bytes = page_zero_bytes;
		if (!vm_alloc_page_with_initializer (VM_FILE, addr, writable, lazy_load_segment_mmap, info)) {
			lock_release(&syscall_lock);
			return NULL;
    }
		read_bytes -= page_read_bytes;
		zero_bytes -= page_zero_bytes;
		addr       += PGSIZE;
		offset     += page_read_bytes;
	}
	lock_release(&syscall_lock);
	return first_addr;
	/* NOTE: The end where custom code is added */
}

void do_munmap (void *addr){
	/* NOTE: The beginning where custom code is added */
	struct thread *curr = thread_current();
    struct page *page;
	lock_acquire(&syscall_lock);
    while ((page = spt_find_page(&curr->spt, addr))) {
		destroy(page);
        list_remove(&page->frame->frame_elem);
        free(page->frame);
		addr += PGSIZE;
    }
	lock_release(&syscall_lock);
	/* NOTE: The end where custom code is added */
}
