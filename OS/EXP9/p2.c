#include <stdio.h>

int main() {
    unsigned long page_size;

    printf("Enter the page size (in bytes): ");
    if (scanf("%lu", &page_size) != 1 || page_size == 0) {
        fprintf(stderr, "Invalid input. Page size must be a positive integer.\n");
        return 1;
    }

    // Total virtual address space is 2^32 bytes
    unsigned long max_pages = (1ULL << 32) / page_size;

    printf("Maximum number of pages: %lu\n", max_pages);
    return 0;
}

