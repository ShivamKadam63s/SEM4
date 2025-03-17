#include <stdio.h>
#include <string.h>
#include <stdlib.h>
/*
 * Function: computePrefixFunction
 * -------------------------------
 * Computes the prefix function (also known as the failure function) for the KMP algorithm.
 * 
 * pattern: The pattern string for which we compute the prefix function.
 * prefixArray: An integer array of length m (where m = strlen(pattern)) that will store
 *              the longest proper prefix of the substring pattern[0..i] which is also a suffix
 *              of this substring.
 */
void computePrefixFunction(const char *pattern, int *prefixArray) {
    int m = strlen(pattern);
    int k = 0;              // length of the current longest prefix that is also a suffix
    prefixArray[0] = 0;    
    // We start from i = 1 because prefixArray[0] is already defined
    for (int i = 1; i < m; i++) {
        // If the current characters don't match, reduce k using the prefixArray
        while (k > 0 && pattern[k] != pattern[i]) {
            k = prefixArray[k - 1];
        }
        // If they match, extend the current prefix length
        if (pattern[k] == pattern[i]) {
            k++;
        }
        prefixArray[i] = k;
    }
}
void KMP(const char *pattern, const char *text) {
    int m = strlen(pattern);
    int n = strlen(text);

    // Edge case: if pattern is longer than text, no match is possible
    if (m > n) {
        printf("No occurrences (pattern is longer than text).\n");
        return;
    }
    int *prefixArray = (int *)malloc(m * sizeof(int));
    if (prefixArray == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        return;
    }
    computePrefixFunction(pattern, prefixArray);

    printf("Prefix Array: ");
    for (int i = 0; i < m; i++) {
        printf("%d ", prefixArray[i]);
    }
    printf("\n");

    // 'q' is the number of characters matched so far
    int q = 0;

    // Scan the text from left to right
    for (int i = 0; i < n; i++) {
        // If next character doesn't match, reduce 'q' using the prefix function
        while (q > 0 && pattern[q] != text[i]) {
            q = prefixArray[q - 1];
        }

        // If next character matches, increment 'q'
        if (pattern[q] == text[i]) {
            q++;
        }

        // If we've matched all characters of the pattern
        if (q == m) {
            // Pattern found at index (i - m + 1)
            printf("Pattern found at index %d\n", i - m + 1);

            // Look for the next possible match
            q = prefixArray[q - 1];
        }
    }
    free(prefixArray);
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
    KMP(pattern, text);
    return EXIT_SUCCESS;
}
