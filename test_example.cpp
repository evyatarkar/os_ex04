// This program run the example given in source files.
// Notice that you should include the MemoryConstants.h that provided with the test
// Good luck!

#include "VirtualMemory.h"
#include "PhysicalMemory.h"
#include "stdio.h"
#include "MemoryConstantsTest.h"




void print_physical() {
    word_t val;
    for(int i=0; i<NUM_FRAMES; i++) {
        for(int row =0; row<PAGE_SIZE; row++) {
            PMread(i*PAGE_SIZE + row, &val);
            printf("frame: %d | row: %d | val: %d\n", i, row, val);
        }
    }
}

int main() {
    printf("---- VMinitialize ----\n");
    VMinitialize();
    print_physical();

    printf("---- VMwrite(13, 3) ----\n");
    printf("virtual add: 13 (01101), page: 6 (0110), offset: 1 (1) \n");
    VMwrite(13, 3);
    word_t val;
    VMread(13, &val);
    printf("compare to slide 14\n");
    print_physical(); // compare to slide 14

    printf("---- VMread(6, &val) ----\n");
    printf("virtual add: 6 (00110), page: 3 (0011), offset: 0 (0) \n");
    VMread(6, &val);
    printf("compare to slide 16\n");
    print_physical(); // compare to slide 16

    printf("---- VMread(31, &val);----\n");
    printf("virtual add: 31 (11111), page: 15 (1111), offset: 1 (1) \n");
    VMread(31, &val);
    printf("compare to slide 27\n");
    print_physical(); // compare to slide 27, notice that second row in frame 2 should be 7
    printf("DONE\n");
}