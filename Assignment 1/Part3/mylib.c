#include <stdio.h>
#include <sys/mman.h>

#define MEM_ALLOC(curr) (void*)((char*)curr + 8);
#define NEXT_BLOCK(curr) (Block*)((char*)curr+curr->size)

typedef struct Block
{
	int size;
	struct Block *next;
	struct Block *prev;
} Block;

Block start = {24, NULL, NULL};
Block *head = &start;


void mem_remove(Block *curr) {
	Block *prev = curr->prev;
	prev->next = curr->next;
	if(curr->next) curr->next->prev = prev;
	return;
}

void mem_add(Block *curr) {
	curr->next = head->next;
	curr->prev = head;
	if(head->next) head->next->prev = curr;
	head->next = curr;
	return;
}

Block *mem_split(Block *curr, int size) {
	Block *new_block = (Block*)((char*)curr + size);
	new_block->size = curr->size - size;
	new_block->next = NULL;
	new_block->prev = NULL;
	return new_block;
}

Block *find_first(Block *curr) {
	Block *temp = head->next;
	while(temp) {
		if(NEXT_BLOCK(temp) == curr) return temp;
		temp = temp->next;
	}
	return NULL;
}

Block *find_second(Block *curr) {
	Block *temp = head->next;
	Block *second = NEXT_BLOCK(curr);
	while(temp) {
		if(temp == second) return temp;
		temp = temp->next;
	}
	return NULL;
}

void *memalloc(unsigned long size) 
{
	if(size == 0) return NULL;
	
	int required_size = ((size + 7)/8 + 1)*8;
	// printf("memalloc() called\n");
	Block *curr = head->next;
	while(curr) {
		if(curr->size >= required_size) {
			if((curr->size - required_size ) < 24) {
				// allocate full block and remove the block from the list
				mem_remove(curr);
			}
			else {
				// allocate part of the block and add the rest to the front of the list
				mem_remove(curr);
				mem_add(mem_split(curr, required_size));
				curr->size = required_size;
			}
			return MEM_ALLOC(curr);
		} 
		curr = curr->next;
	}
	// allocate new memory and link it to the head
	int required_page_size = ((size+7)/(4*1024) + 1) * (4*1024);
	Block *new_memory = (Block *)mmap(NULL, required_page_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	if(new_memory == MAP_FAILED) {
		perror("mmap failed\n");
		return NULL;
	}
	
	new_memory->size = required_page_size;
	new_memory->next = NULL;
	new_memory->prev = NULL;
	if(required_page_size >= 24 + required_size) {
		mem_add(mem_split((Block*)new_memory, required_size));
	}

	return MEM_ALLOC(new_memory);
}

int memfree(void *ptr)
{
	// printf("memfree() called\n");
	Block *curr = (Block*)((char*)ptr - 8);
	// printf("size of block: %d\n", curr->size);
	curr->next = NULL;
	curr->prev = NULL;
	Block *first = find_first(curr);
	Block *second = find_second(curr);
	if(first == NULL && second == NULL) {
		// both are NULL
		// if(curr->size < 24) return -1;
		mem_add(curr);
	}
	else if(first == NULL) {
		// only first is NULL
		first->size = first->size + second->size;
		mem_add(curr);
		mem_remove(second);
	}
	else if(second == NULL) {
		// only second is NULL
		first->size = first->size + curr->size;
		mem_remove(first);
		mem_add(first);
	}
	else {
		// none is NULL
		first->size = first->size + curr->size + second->size;
		mem_remove(first);
		mem_remove(second);
		mem_add(first);
	}
	return 0;
}	
