#include <stdio.h>

int main() {
    unsigned long physical_size, page_size;

    printf("Enter the physical memory size (in bytes): ");
    if (scanf("%lu", &physical_size) != 1) {
        fprintf(stderr, "Invalid input for physical memory size.\n");
        return 1;
    }

    printf("Enter the page size (in bytes): ");
    if (scanf("%lu", &page_size) != 1 || page_size == 0) {
        fprintf(stderr, "Invalid input. Page size must be a positive integer.\n");
        return 1;
    }

    unsigned long num_pages = physical_size / page_size;

    printf("Number of pages in physical memory: %lu\n", num_pages);
    return 0;
}

