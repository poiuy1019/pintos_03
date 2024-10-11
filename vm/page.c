#include "filesys/file.h"
#include "vm/vm.h"
#include <string.h>


bool load_file (void *kaddr, struct page *page){
    int bytes_read = file_read_at(page->file, kaddr, page->read_bytes, page->offset);
    // vme의 read_bytes만큼만 read했는지 확인하는 절차
    if (bytes_read != page->read_bytes) {
        return false; // read과정이 옳바르지 않을경우 return false 
    }
    //남는 자리 0으로 채우기
    if (page->zero_bytes > 0) {
    memset((char *)kaddr + bytes_read, 0, page->zero_bytes);
    }
    return true;
}