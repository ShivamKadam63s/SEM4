#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define RADIX 10
#define PRIME 101

void rabinKarpSearch(const char *pattern, const char *text) {
    int m = strlen(pattern);
    int n = strlen(text);
    int i, j;
    int patternHash = 0;
    int textHash = 0;
    int h = 1;
    
    for (i = 0; i < m - 1; i++) {
        h = (h * RADIX) % PRIME;
    }
    printf("Initial value of h: %d\n", h);

    for (i = 0; i < m; i++) {
        patternHash = (RADIX * patternHash + pattern[i]) % PRIME;
        textHash = (RADIX * textHash + text[i]) % PRIME;
    }

    printf("Initial pattern hash: %d\n", patternHash);
    printf("Initial text hash: %d\n", textHash);

    for (i = 0; i <= n - m; i++) {
        printf("\nWindow at index %d: ", i);
        printf("text substring = \"%.*s\"\n", m, &text[i]);

        if (patternHash == textHash) {
            printf("Hash match found at index %d. Verifying characters...\n", i);
            for (j = 0; j < m; j++) {
                printf("Comparing text[%d] = %c with pattern[%d] = %c\n", i + j, text[i + j], j, pattern[j]);
                if (text[i + j] != pattern[j]) {
                    break;
                }
            }
            if (j == m) {
                printf("Pattern found at index %d\n", i);
            } else {
                printf("No match found, hash matched but characters did not match.\n");
            }
        }
        if (i < n - m) {
            textHash = (RADIX * (textHash - text[i] * h) + text[i + m]) % PRIME;
            if (textHash < 0) {
                textHash = textHash + PRIME;
            }

            printf("Updated text hash after sliding window: %d\n", textHash);
        }
    }
}

int main() {
    char text[1000];
    char pattern[1000];
    printf("Enter the text: ");
    if (fgets(text, sizeof(text), stdin) == NULL) {
        fprintf(stderr, "Error reading text.\n");
        return EXIT_FAILURE;
    }
    text[strcspn(text, "\n")] = '\0';

    printf("Enter the pattern: ");
    if (fgets(pattern, sizeof(pattern), stdin) == NULL) {
        fprintf(stderr, "Error reading pattern.\n");
        return EXIT_FAILURE;
    }
    pattern[strcspn(pattern, "\n")] = '\0';

    if (strlen(pattern) > strlen(text)) {
        printf("The pattern is longer than the text. No match possible.\n");
        return EXIT_SUCCESS;
    }

    rabinKarpSearch(pattern, text);

    return EXIT_SUCCESS;
}
