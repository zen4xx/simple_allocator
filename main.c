#include <stddef.h>
#include <string.h>
#include <sys/mman.h>
#include <stdio.h>

#define ALLOCATOR_PAGE_SIZE 4096

typedef struct Block{
    int is_free;
    size_t size;
    struct Block* next;
    struct Block* prev;
}Block;

static Block* free_list_head = NULL;

void* page_ptr;

void *allocate_page(){
    return mmap(NULL, ALLOCATOR_PAGE_SIZE, PROT_WRITE | PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);  
}

void add_to_free_list(Block* block){
    if(!free_list_head){
        free_list_head = block;
        block->prev = block->next = NULL;
    }
    else{
        block->next = free_list_head;
        block->prev = NULL;
        free_list_head->prev = block;
        free_list_head = block;
    }
} 

void remove_from_free_list(Block* block){
    if(block->prev) block->prev->next = block->next;
    if(block->next) block->next->prev = block->prev;
    if(block == free_list_head) free_list_head = block->next;
}

Block* find_free_block(size_t size){
    Block* curr = free_list_head;
    while(curr){
        if(curr->size >= size){
            remove_from_free_list(curr);
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

void init_alloc(){
    page_ptr = allocate_page();
    Block* inital_block = (Block*)page_ptr;
    inital_block->size = ALLOCATOR_PAGE_SIZE;
    inital_block->is_free = 1;
}

void destroy_alloc(){
    munmap(page_ptr, ALLOCATOR_PAGE_SIZE);
}

void* allocate(size_t size){
    size_t total_size = sizeof(Block) + size + (ALLOCATOR_PAGE_SIZE - 1);
    total_size &= ~(ALLOCATOR_PAGE_SIZE-1);
    Block* curr = find_free_block(total_size);
    if(!curr){
        curr = (Block*)allocate_page();
        curr->size = ALLOCATOR_PAGE_SIZE;
        curr->is_free = 0;
    }
    if(curr->size > total_size){
        Block* new_block = (Block*)((char*)curr + total_size);
        new_block->size = curr->size - total_size - sizeof(Block);
        new_block->is_free = 1;
        add_to_free_list(new_block);
    }
    curr->is_free = 0;
    return(void*)(curr + 1);
}

void release(void* ptr){
    if(!ptr) return;
    Block* block = (Block*)((char*)ptr - sizeof(Block));
    block->is_free = 1;

    if(block->prev && block->prev->is_free){
        Block* prev_block = block->prev;
        prev_block->size += block->size + sizeof(Block);
        if(block->next) block->next->prev = prev_block;
        block = prev_block;
    }
    if(block->next && block->next->is_free){
        Block* next_block = block->next;
        block->size += next_block->size + sizeof(Block);
        block->next = next_block->next;
        if(next_block->next) next_block->next->prev = block;
        block = next_block;
    }
    add_to_free_list(block);
}

int main(){
    init_alloc();
    int* x = (int*)allocate(sizeof(int));
    *x = 5;
    printf("%d\n", *x);
    release(x);
    destroy_alloc();
    return 0;
}