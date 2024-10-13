#include "filesys/file.h"
#include "vm/vm.h"
#include <string.h>


// bool load_file (void *kaddr, struct page *page) {
//     int bytes_read = file_read_at(page->file, kaddr, page->file_page.read_bytes, page->file_page.offset);
//     // vme의 read_bytes만큼만 read했는지 확인하는 절차
//     if (bytes_read != page->file_page.read_bytes) {
//         return false; // read과정이 옳바르지 않을경우 return false 
//     }
//     //남는 자리 0으로 채우기
//     if (page->file_page.zero_bytes > 0) {
//     memset((char *)kaddr + bytes_read, 0, page->file_page.zero_bytes);
//     }
//     return true;
//     page->
// }

bool load_file (void *kaddr, struct page *page) {
    struct file_page *file_page = &page->file;
    int bytes_read = file_read_at(file_page, kaddr, file_page->read_bytes, file_page->offset);
    if (bytes_read != (int)file_page->read_bytes) {
        return false;
    }
    if (file_page->zero_bytes > 0) {
        memset((char *)kaddr + bytes_read, 0, file_page->zero_bytes);
    }
    return true;
}