#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include "myHeap.h"
 
/*
 * This structure serves as the header for each allocated and free block.
 * It also serves as the footer for each free block but only containing size.
 */
typedef struct blockHeader {

    int size_status;
} blockHeader;

/* Global variable - DO NOT CHANGE. It should always point to the first block,
 * i.e., the block at the lowest address.
 */
blockHeader *heapStart = NULL;

/* Size of heap allocation padded to round to nearest page size.
 */
int allocsize;

/*
 * Additional global variables may be added as needed below
 */

 
/*
 * Function for allocating 'size' bytes of heap memory.
 * Argument size: requested size for the payload
 * Returns address of allocated block (payload) on success.
 * Returns NULL on failure.
 *
 * This function must:
 * - Check size - Return NULL if not positive or if larger than heap space.
 * - Determine block size rounding up to a multiple of 8
 *   and possibly adding padding as a result.
 *
 * - Use BEST-FIT PLACEMENT POLICY to chose a free block
 *
 * - If the BEST-FIT block that is found is exact size match
 *   - 1. Update all heap blocks as needed for any affected blocks
 *   - 2. Return the address of the allocated block payload
 *
 * - If the BEST-FIT block that is found is large enough to split
 *   - 1. SPLIT the free block into two valid heap blocks:
 *         1. an allocated block
 *         2. a free block
 *         NOTE: both blocks must meet heap block requirements
 *       - Update all heap block header(s) and footer(s)
 *              as needed for any affected blocks.
 *   - 2. Return the address of the allocated block payload
 *
 * - If a BEST-FIT block found is NOT found, return NULL
 *   Return NULL unable to find and allocate block for desired size
 *
 * Note: payload address that is returned is NOT the address of the
 *       block header.  It is the address of the start of the
 *       available memory for the requesterr.
 *
 * Tips: Be careful with pointer arithmetic and scale factors.
 */
void* myAlloc(int size) {
    //TODO: Your code goes in here.
    if(size <= 0 || size > allocsize || heapStart == NULL){
        return NULL;
    }
    int padding = 0;
    int fullsize = size + 4; // 4 bytes for header
    if((size + 4) % 8 == 0){
        padding = 0;
    } else{
        padding = (8 - ((size + 4) % 8));
        // padding if the full size if not a multiple of 8
    }
    fullsize = fullsize + padding; // updating fullsize with padding if required
    blockHeader *fptr = heapStart;
    blockHeader *fit = NULL; // created for best fit
    int size2;
    
    while (fptr -> size_status != 1){ //not the end
        if((fptr -> size_status/8) * 8 < fullsize){ // if block is not large enough
            fptr = (blockHeader*) ((char*) fptr + (fptr -> size_status/8) * 8);
            continue; // we want to keep looking
        }
        if(fptr -> size_status % 2 != 0 ){ //not available
            fptr = (blockHeader*) ((char*) fptr + (fptr -> size_status/8) * 8);
            continue; // we want to keep looking
            }
        else if ((fptr -> size_status/8) * 8 == fullsize){ // fit matches
            fptr -> size_status = fptr -> size_status + 1; // now allocated
            fptr = (blockHeader*) ((char*) fptr + fullsize); // adding the size of the block to go to the next block
            fptr = (blockHeader*) ((char*) fptr - fullsize + 4);
            return fptr;
        }
        else if((fptr -> size_status/8) * 8 == fullsize){
            if(fptr != NULL){ // if not null
                if(fptr -> size_status == 0) {
                    fptr -> size_status = fptr -> size_status + 2;
                }
            }
        }
        else {
            fit = fptr;
            size2 = (fptr -> size_status/8) * 8;
        }
        fptr = (blockHeader*)((char*)fptr + (fptr -> size_status/8) * 8 );
    }
    if(fit == NULL){ // if best fit doesnt exist, then return null
        return NULL;
    }
    if((fptr -> size_status/8) * 8 != fullsize){
        int p = fit->size_status & 2;
        fit->size_status = fullsize + 1 + p;
        blockHeader *head = (blockHeader*)((char*)fit + fullsize); // header
        head->size_status = (size2 - fullsize) + 2;
        blockHeader *footer = (blockHeader*)((char*)head + size2 - fullsize - 4); // footer
        footer -> size_status = (fptr -> size_status/8) * 8 - fullsize;
        fit = (blockHeader*)((char*)footer - size2 + 2 * 4);
        return fit;
    }
    return NULL;
}
 
/*
 * Function for freeing up a previously allocated block.
 * Argument ptr: address of the block to be freed up.
 * Returns 0 on success.
 * Returns -1 on failure.
 * This function should:
 * - Return -1 if ptr is NULL.
 * - Return -1 if ptr is not a multiple of 8.
 * - Return -1 if ptr is outside of the heap space.
 * - Return -1 if ptr block is already freed.
 * - Update header(s) and footer as needed.
 */
int myFree(void *ptr) {
    int header = sizeof(blockHeader); // 4
    if (ptr == NULL){ // ptr is null
        return -1;
    }
    blockHeader *point = ptr - header;
    if((point -> size_status) % 2 != 1){ // already freed
        return -1;
    }
    int sz = (point -> size_status /8) * 8;
    
    if((unsigned int) ptr % 8 != 0){ // if not a multiple of 8, then return -1
        return -1;
    }
    if((unsigned int) ptr < (unsigned int) heapStart){ // if ptr is less than the heap space
        return -1;
    }
    point = (blockHeader*) ((char*) point + sz - header);
    blockHeader *al = (blockHeader*)((char*) ptr - header);
    al->size_status -= 1; // alloc bit cleared
    point = (blockHeader*)((char*) point + sz - header);
    blockHeader *footer = (blockHeader*) ((char*) al + sz - header);
    footer->size_status = sz;
    point = (blockHeader*)((char*) point + header);
    
    if(point -> size_status == 1){ // end reached
        return 0;
    }
    point -> size_status = point -> size_status -2;
    return 0;
}

/*
 * Function for traversing heap block list and coalescing all adjacent
 * free blocks.
 *
 * This function is used for delayed coalescing.
 * Updated header size_status and footer size_status as needed.
 */
int coalesce() {
    //TODO: Your code goes in here.
    blockHeader *curr = heapStart; // current initialization
    blockHeader *nextBlock = NULL;
    int sizeval;
    
    while(1){
        if (curr -> size_status != 1) { // not the end
            sizeval = (curr -> size_status/8)*8;
            nextBlock = (blockHeader*) (char*) curr + sizeval; // setting up next block
        }
        if(curr -> size_status != 1){
            if(nextBlock -> size_status == 1){ // we have to reach the end
                return 0;
            }
            if(curr -> size_status % 2 != 0 || nextBlock -> size_status % 2 != 0){
                curr = nextBlock;
                continue;
            }
        }
        break;
    }
    return 0;
}

/*
 * Function used to initialize the memory allocator.
 * Intended to be called ONLY once by a program.
 * Argument sizeOfRegion: the size of the heap space to be allocated.
 * Returns 0 on success.
 * Returns -1 on failure.
 */
int myInit(int sizeOfRegion) {
 
    static int allocated_once = 0; //prevent multiple myInit calls
 
    int pagesize;   // page size
    int padsize;    // size of padding when heap size not a multiple of page size
    void* mmap_ptr; // pointer to memory mapped area
    int fd;

    blockHeader* endMark;
  
    if (0 != allocated_once) {
        fprintf(stderr,
        "Error:mem.c: InitHeap has allocated space during a previous call\n");
        return -1;
    }

    if (sizeOfRegion <= 0) {
        fprintf(stderr, "Error:mem.c: Requested block size is not positive\n");
        return -1;
    }

    // Get the pagesize
    pagesize = getpagesize();

    // Calculate padsize as the padding required to round up sizeOfRegion
    // to a multiple of pagesize
    padsize = sizeOfRegion % pagesize;
    padsize = (pagesize - padsize) % pagesize;

    allocsize = sizeOfRegion + padsize;

    // Using mmap to allocate memory
    fd = open("/dev/zero", O_RDWR);
    if (-1 == fd) {
        fprintf(stderr, "Error:mem.c: Cannot open /dev/zero\n");
        return -1;
    }
    mmap_ptr = mmap(NULL, allocsize, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (MAP_FAILED == mmap_ptr) {
        fprintf(stderr, "Error:mem.c: mmap cannot allocate space\n");
        allocated_once = 0;
        return -1;
    }
  
    allocated_once = 1;

    // for double word alignment and end mark
    allocsize -= 8;

    // Initially there is only one big free block in the heap.
    // Skip first 4 bytes for double word alignment requirement.
    heapStart = (blockHeader*) mmap_ptr + 1;

    // Set the end mark
    endMark = (blockHeader*)((void*)heapStart + allocsize);
    endMark->size_status = 1;

    // Set size in header
    heapStart->size_status = allocsize;

    // Set p-bit as allocated in header
    // note a-bit left at 0 for free
    heapStart->size_status += 2;

    // Set the footer
    blockHeader *footer = (blockHeader*) ((void*)heapStart + allocsize - 4);
    footer->size_status = allocsize;
  
    return 0;
}
                  
/*
 * Function to be used for DEBUGGING to help you visualize your heap structure.
 * Prints out a list of all the blocks including this information:
 * No.      : serial number of the block
 * Status   : free/used (allocated)
 * Prev     : status of previous block free/used (allocated)
 * t_Begin  : address of the first byte in the block (where the header starts)
 * t_End    : address of the last byte in the block
 * t_Size   : size of the block as stored in the block header
 */
void dispMem() {
 
    int counter;
    char status[6];
    char p_status[6];
    char *t_begin = NULL;
    char *t_end   = NULL;
    int t_size;

    blockHeader *current = heapStart;
    counter = 1;

    int used_size = 0;
    int free_size = 0;
    int is_used   = -1;

    fprintf(stdout,
    "*********************************** Block List **********************************\n");
    fprintf(stdout, "No.\tStatus\tPrev\tt_Begin\t\tt_End\t\tt_Size\n");
    fprintf(stdout,
    "---------------------------------------------------------------------------------\n");
  
    while (current->size_status != 1) {
        t_begin = (char*)current;
        t_size = current->size_status;
    
        if (t_size & 1) {
            // LSB = 1 => used block
            strcpy(status, "alloc");
            is_used = 1;
            t_size = t_size - 1;
        } else {
            strcpy(status, "FREE ");
            is_used = 0;
        }

        if (t_size & 2) {
            strcpy(p_status, "alloc");
            t_size = t_size - 2;
        } else {
            strcpy(p_status, "FREE ");
        }

        if (is_used)
            used_size += t_size;
        else
            free_size += t_size;

        t_end = t_begin + t_size - 1;
    
        fprintf(stdout, "%d\t%s\t%s\t0x%08lx\t0x%08lx\t%4i\n", counter, status,
        p_status, (unsigned long int)t_begin, (unsigned long int)t_end, t_size);
    
        current = (blockHeader*)((char*)current + t_size);
        counter = counter + 1;
    }

    fprintf(stdout,
    "---------------------------------------------------------------------------------\n");
    fprintf(stdout,
    "*********************************************************************************\n");
    fprintf(stdout, "Total used size = %4d\n", used_size);
    fprintf(stdout, "Total free size = %4d\n", free_size);
    fprintf(stdout, "Total size      = %4d\n", used_size + free_size);
    fprintf(stdout,
    "*********************************************************************************\n");
    fflush(stdout);

    return;
}
