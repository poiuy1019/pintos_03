#include "tests/main.h"
#include "tests/vm/parallel-merge.h"
#include "tests/lib.h"
void
test_main (void) 
{
  parallel_merge ("child-sort", 123);
}
