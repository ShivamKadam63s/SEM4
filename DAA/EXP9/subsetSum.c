#include <stdio.h>  
#define MAX 100  

int found = 0; // Global variable to track if a valid subset is found

// Backtracking function to find subsets that sum up to the target
void backtrack(int arr[], int n, int index, int currSum, int target, int sol[], int solIndex)
{
    // Base case: if current sum equals the target, print the subset
    if (currSum == target)
    {
        printf("Ans [");
        for (int i = 0; i < solIndex; i++)
        {
            printf("%d", sol[i]); // Print the subset elements
            if (i < solIndex - 1)
                printf(","); // Add commas between elements
        }
        printf("]\n");
        found = 1; // Mark that a subset has been found
        return;
    }

    // If we've processed all elements or current sum exceeds target, stop recursion
    if (index >= n || currSum > target)
        return;

    // Include the current element in the subset
    sol[solIndex] = arr[index];
    // Recur to include next elements
    backtrack(arr, n, index + 1, currSum + arr[index], target, sol, solIndex + 1);
    // Exclude the current element and move to the next element
    backtrack(arr, n, index + 1, currSum, target, sol, solIndex);
}

int main()
{
    int n, target;
    int arr[MAX], sol[MAX]; 
    printf("Enter the number of elements in the set: ");
    scanf("%d", &n);
    printf("Enter the elements (non-negative integers): ");
    for (int i = 0; i < n; i++)
    {
        scanf("%d", &arr[i]);
    }
    printf("Enter the target sum: ");
    scanf("%d", &target);
    backtrack(arr, n, 0, 0, target, sol, 0);
    if (!found)
    {
        printf("No subset found that adds up to %d.\n", target);
    }
    return 0; 
}
