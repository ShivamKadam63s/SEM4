#include <stdio.h>
#include <stdlib.h>

int solutionCount = 0; // Global variable to count the number of solutions

// Function to check if it's safe to place a queen at (row, col)
int isSafe(int board[], int row, int col, int n) {
    int i;
    for (i = 0; i < row; i++) {
        // Check for queen in the same column or diagonals
        if (board[i] == col || abs(board[i] - col) == abs(i - row))
            return 0; // Not safe
    }
    return 1; // Safe to place the queen
}

// Function to print the chessboard with queens represented by 'Q'
void printBoard(int board[], int n) {
    int i, j;
    printf("Solution %d:\n", ++solutionCount); // Increment and display solution number
    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            if (board[i] == j)
                printf("Q "); // Place queen
            else
                printf(". "); // Empty space
        }
        printf("\n");
    }
    printf("\n");
}

// Recursive utility function to solve the N-Queens problem
void solveNQueensUtil(int board[], int row, int n, int printSolutions) {
    if (row == n) { // Base case: all queens are placed
        if (printSolutions) {
            printBoard(board, n); // Print the board if in print mode
        } else {
            solutionCount++; // Count the solution without printing
        }
        return;
    }

    // Try placing a queen in each column of the current row
    for (int col = 0; col < n; col++) {
        if (isSafe(board, row, col, n)) {
            board[row] = col; // Place queen
            solveNQueensUtil(board, row + 1, n, printSolutions); // Recur for next row
        }
    }
}

// Main function to handle user input and initiate solving
void solveNQueens(int n, int printSolutions) {
    int *board = (int *)malloc(n * sizeof(int)); #
    if (!board) { 
        printf("Memory allocation failed.\n");
        exit(EXIT_FAILURE);
    }

    solutionCount = 0; // Reset solution count before solving
    solveNQueensUtil(board, 0, n, printSolutions); // Start solving from row 0

    // If not in print mode, display the total number of solutions
    if (!printSolutions) {
        printf("Total number of solutions: %d\n", solutionCount);
    }

    free(board); 
}

int main() {
    int n, mode;#
    printf("Enter the value of N (size of the chess board): ");
    scanf("%d", &n);

    printf("Choose mode:\n");
    printf("1. Print all solutions in chess board format\n");
    printf("2. Print only the number of solutions\n");
    printf("Enter your choice (1 or 2): ");
    scanf("%d", &mode);

    if (mode == 1)
        solveNQueens(n, 1); 
    else if (mode == 2)
        solveNQueens(n, 0); 
    else
        printf("Invalid choice. Please choose either 1 or 2.\n");

    return 0;
}
