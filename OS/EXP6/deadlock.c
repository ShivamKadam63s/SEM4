#include <stdio.h>
#include <stdlib.h>


#define MAX_PROCESSES 10
#define MAX_RESOURCES 10


void compute_need(int n, int m, int allocation[][MAX_RESOURCES], int max[][MAX_RESOURCES], int need[][MAX_RESOURCES]) {
   int i, j;
   for (i = 0; i < n; i++) {
       for (j = 0; j < m; j++) {
           need[i][j] = max[i][j] - allocation[i][j];
       }
   }
}
// Find a safe sequence using Banker's algorithm.
// If a safe sequence exists, safe_seq[] will hold the order and the function returns 1; otherwise, returns 0.
int find_safe_sequence(int n, int m, int allocation[][MAX_RESOURCES], int need[][MAX_RESOURCES], int available[], int safe_seq[]) {
   int work[MAX_RESOURCES];
   int finish[MAX_PROCESSES] = {0};
   int i, j, count = 0, found;
  
   // Copy available resources into work array
   for (j = 0; j < m; j++) {
       work[j] = available[j];
   }
  
   // Try to find an order in which all processes can finish
   do {
       found = 0;
       for (i = 0; i < n; i++) {
           if (!finish[i]) {
               int can_finish = 1;
               // Check if process i's need can be met by work
               for (j = 0; j < m; j++) {
                   if (need[i][j] > work[j]) {
                       can_finish = 0;
                       break;
                   }
               }
               if (can_finish) {
                   // Simulate process i finishing and releasing its allocation
                   for (j = 0; j < m; j++) {
                       work[j] += allocation[i][j];
                   }
                   safe_seq[count] = i;
                   count++;
                   finish[i] = 1;
                   found = 1;
               }
           }
       }
   } while(found);
  
   // If count equals number of processes, a safe sequence was found
   if (count == n)
       return 1;
   return 0;
}


void print_state(int n, int m, int allocation[][MAX_RESOURCES], int max[][MAX_RESOURCES], int need[][MAX_RESOURCES]) {
   int i, j;
   printf("Process\tAllocation  \tMax\t\tNeed\n");
   for (i = 0; i < n; i++) {
       printf("P%d\t", i);
       for (j = 0; j < m; j++) {
           printf("%d ", allocation[i][j]);
       }
       printf("\t\t");
       for (j = 0; j < m; j++) {
           printf("%d ", max[i][j]);
       }
       printf("\t\t");
       for (j = 0; j < m; j++) {
           printf("%d ", need[i][j]);
       }
       printf("\n");
   }
   printf("-------------------------------------------------\n");
}


void print_available_matrix(int n, int m, int allocation[][MAX_RESOURCES], int available[], int safe_seq[]) {
   int i, j;
   int avail_sim[MAX_PROCESSES+1][MAX_RESOURCES];
   for (j = 0; j < m; j++) {
       avail_sim[0][j] = available[j];
   }
   // For each process in the safe sequence, update the available resources
   for (i = 0; i < n; i++) {
       for (j = 0; j < m; j++) {
           avail_sim[i+1][j] = avail_sim[i][j] + allocation[safe_seq[i]][j];
       }
   }
  
   // Print the available matrix
   printf("\n--- Available Matrix (Evolution of available resources) ---\n");
   printf("Step\t");
   for (j = 0; j < m; j++) {
       printf("R%d\t", j);
   }
   printf("\n");
   for (i = 0; i < n+1; i++) {
       printf("Step %d\t", i);
       for (j = 0; j < m; j++) {
           printf("%d\t", avail_sim[i][j]);
       }
       printf("\n");
   }
   printf("-----------------------------------------------------------\n\n");
}


int main() {
   int n, m;
   int allocation[MAX_PROCESSES][MAX_RESOURCES];
   int max[MAX_PROCESSES][MAX_RESOURCES];
   int need[MAX_PROCESSES][MAX_RESOURCES];
   int available[MAX_RESOURCES];
   int safe_seq[MAX_PROCESSES];
   int i, j;
  
   printf("Enter number of processes (max %d): ", MAX_PROCESSES);
   scanf("%d", &n);
   printf("Enter number of resource types (max %d): ", MAX_RESOURCES);
   scanf("%d", &m);
  
   printf("\nEnter allocation for each process and resource:\n");
   for (i = 0; i < n; i++) {
       printf("For Process P%d:", i);
       for (j = 0; j < m; j++) {
           scanf("%d", &allocation[i][j]);
       }
   }
  
   printf("\nEnter maximum resources for each process and resource:\n");
   for (i = 0; i < n; i++) {
       printf("For Process P%d:", i);
       for (j = 0; j < m; j++) {
           scanf("%d", &max[i][j]);
       }
   }
  
   printf("\nEnter available resources for each type:");
   for (j = 0; j < m; j++) {
       scanf("%d", &available[j]);
   }
  
   compute_need(n, m, allocation, max, need);
  
   printf("\nInitial State:\n");
   print_state(n, m, allocation, max, need);
  
   // Check for safe sequence (deadlock detection)
   if (find_safe_sequence(n, m, allocation, need, available, safe_seq)) {
       printf("\nSafe sequence found: ");
       for (i = 0; i < n-1; i++) {
           printf("P%d->", safe_seq[i]);
       }
       printf("P%d\n",safe_seq[n-1]);
       // Print the evolution of available resources based on safe sequence
       print_available_matrix(n, m, allocation, available, safe_seq);
   } else {
       printf("\nNo safe sequence exists. Deadlock detected.\n");
   }
  
   // Prompt user for a resource request
   int req[MAX_RESOURCES];
   int pid;
   printf("\nEnter the process number that is requesting resources: ");
   scanf("%d", &pid);
   if (pid < 0 || pid >= n) {
       printf("Invalid process number.\n");
       return 1;
   }
   printf("Enter the request for each resource for Process P%d:", pid);
   for (j = 0; j < m; j++) {
       scanf("%d", &req[j]);
   }
  
   // Check if the request exceeds the process's need
   for (j = 0; j < m; j++) {
       if (req[j] > need[pid][j]) {
           printf("Error: Request for Resource R%d exceeds need for Process P%d.\n", j, pid);
           return 1;
       }
   }
  
   // Check if the request exceeds current available resources
   for (j = 0; j < m; j++) {
       if (req[j] > available[j]) {
           printf("Error: Request for Resource R%d exceeds available resources.\n", j);
           return 1;
       }
   }
  
   // Tentatively allocate requested resources
   for (j = 0; j < m; j++) {
       available[j] -= req[j];
       allocation[pid][j] += req[j];
       need[pid][j] -= req[j];
   }
  
   // Check if new state is safe after allocation
   if (find_safe_sequence(n, m, allocation, need, available, safe_seq)) {
       printf("\nThe request can be granted. New state:\n");
       print_state(n, m, allocation, max, need);
       printf("\nNew safe sequence: ");
       for (i = 0; i < n-1; i++) {
           printf("P%d->", safe_seq[i]);
       }
       printf("P%d\n",safe_seq[n-1]);
       print_available_matrix(n, m, allocation, available, safe_seq);
   } else {
       // Revert allocation if new state is unsafe
       printf("\nThe request cannot be granted as it leads to an unsafe state.\n");
       for (j = 0; j < m; j++) {
           available[j] += req[j];
           allocation[pid][j] -= req[j];
           need[pid][j] += req[j];
       }
       print_state(n, m, allocation, max, need);
       if (find_safe_sequence(n, m, allocation, need, available, safe_seq)) {
           printf("\nSafe sequence remains: ");
           for (i = 0; i < n-1; i++) {
               printf("P%d->", safe_seq[i]);
           }
           printf("P%d\n",safe_seq[n-1]);
           print_available_matrix(n, m, allocation, available, safe_seq);
       }
   }
   return 0;
}
