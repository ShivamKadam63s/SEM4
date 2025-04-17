#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <virtual_address>\n", argv[0]);
        return 1;
    }

    unsigned long address = strtoul(argv[1], NULL, 10);
    unsigned long page_size = 4096; // 4KB page size

    // Calculate page number (shift right by 12 bits for 4KB pages)
    unsigned long page_number = address >> 12;
    // Calculate offset (mask lower 12 bits)
    unsigned long offset = address & (page_size - 1);

    printf("Page Number: %lu\n", page_number);
    printf("Offset: %lu\n", offset);

    return 0;
}
