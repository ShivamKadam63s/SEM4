#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define RADIX 10
#define PRIME 101   // A prime number used for modulo operations

void rabinKarpSearch(const char *pattern, const char *text) {
    int m = strlen(pattern);   
    int n = strlen(text);      
    int i, j;
    int patternHash = 0;       // Hash value for the pattern
    int textHash = 0;          // Hash value for the current window of text
    int h = 1;                 // The value of RADIX^(m-1) modulo PRIME

    // Precompute h = RADIX^(m-1) % PRIME.
    // This value is used to remove the leading digit in the rolling hash.
    for (i = 0; i < m - 1; i++) {
        h = (h * RADIX) % PRIME;
    }

    // Calculate the initial hash values for the pattern and the first window of text.
    for (i = 0; i < m; i++) {
        patternHash = (RADIX * patternHash + pattern[i]) % PRIME;
        textHash = (RADIX * textHash + text[i]) % PRIME;
    }

    // Slide the pattern over text one by one.
    for (i = 0; i <= n - m; i++) {
        // If the hash values match, then only check the characters one by one.
        if (patternHash == textHash) {
            /* Check for spurious hit by comparing the actual characters */
            for (j = 0; j < m; j++) {
                if (text[i + j] != pattern[j])
                    break;
            }
            // If the pattern is found, print the shift (i.e., starting index).
            if (j == m) {
                printf("Pattern found at index %d\n", i);
            }
        }

        // Calculate hash value for the next window of text:
        // Remove the leading digit and add the trailing digit.
        if (i < n - m) {
            textHash = (RADIX * (textHash - text[i] * h) + text[i + m]) % PRIME;

            // In case textHash becomes negative, convert it to positive.
            if (textHash < 0)
                textHash = textHash + PRIME;
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
    // Remove newline character if present.
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
