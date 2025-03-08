#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define MAX_PROCESSES 10
#define MAX_RESOURCES 10

// Global data structures for allocation, max need, available resources, etc.
int n, m;  // n = number of users (processes), m = number of resource types
int allocation[MAX_PROCESSES][MAX_RESOURCES];
int max_need[MAX_PROCESSES][MAX_RESOURCES];
int need[MAX_PROCESSES][MAX_RESOURCES];
int available[MAX_RESOURCES];

pthread_mutex_t lock;  // Mutex to protect global data

// Function declarations
void compute_need();
int find_safe_sequence(int safe_seq[]);
void print_state();
void print_available_matrix(int safe_seq[]);

// Compute 'need' = 'max_need' - 'allocation'
void compute_need() {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            need[i][j] = max_need[i][j] - allocation[i][j];
        }
    }
}

// Banker's Algorithm to check if state is safe
int find_safe_sequence(int safe_seq[]) {
    int work[MAX_RESOURCES];
    int finish[MAX_PROCESSES] = {0};
    int count = 0;
    int found;

    for (int j = 0; j < m; j++) {
        work[j] = available[j];
    }

    do {
        found = 0;
        for (int i = 0; i < n; i++) {
            if (!finish[i]) {
                int can_finish = 1;
                for (int j = 0; j < m; j++) {
                    if (need[i][j] > work[j]) {
                        can_finish = 0;
                        break;
                    }
                }
                if (can_finish) {
                    for (int j = 0; j < m; j++) {
                        work[j] += allocation[i][j];
                    }
                    safe_seq[count++] = i;
                    finish[i] = 1;
                    found = 1;
                }
            }
        }
    } while (found);

    return count == n;  // Safe sequence found
}

// Print current allocation, max, and need states
void print_state() {
    printf("User\tAllocation\tMax\t\tNeed\n");
    for (int i = 0; i < n; i++) {
        printf("U%d\t", i);
        for (int j = 0; j < m; j++) {
            printf("%d ", allocation[i][j]);
        }
        printf("\t\t");
        for (int j = 0; j < m; j++) {
            printf("%d ", max_need[i][j]);
        }
        printf("\t\t");
        for (int j = 0; j < m; j++) {
            printf("%d ", need[i][j]);
        }
        printf("\n");
    }
    printf("-------------------------------------------------\n");
}

// Print evolution of available resources based on a safe sequence
void print_available_matrix(int safe_seq[]) {
    int avail_sim[MAX_PROCESSES+1][MAX_RESOURCES];

    for (int j = 0; j < m; j++) {
        avail_sim[0][j] = available[j];
    }

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            avail_sim[i+1][j] = avail_sim[i][j] + allocation[safe_seq[i]][j];
        }
    }

    printf("\n--- Available Matrix (Evolution of available resources) ---\n");
    printf("Step\t");
    for (int j = 0; j < m; j++) {
        printf("R%d\t", j);
    }
    printf("\n");
    for (int i = 0; i < n+1; i++) {
        printf("Step %d\t", i);
        for (int j = 0; j < m; j++) {
            printf("%d\t", avail_sim[i][j]);
        }
        printf("\n");
    }
    printf("-----------------------------------------------------------\n\n");
}

// Thread function to request resources
void* request_resources(void *arg) {
    RequestInfo *info = (RequestInfo *)arg;
    int u = info->user_id;

    pthread_mutex_lock(&lock);

    printf("\n[Thread] User U%d attempting request: ", u);
    for (int j = 0; j < m; j++) {
        printf("%d ", info->request[j]);
    }
    printf("\n");

    // Check if request is valid
    for (int j = 0; j < m; j++) {
        if (info->request[j] > need[u][j]) {
            printf("Error: Request exceeds Need for User U%d.\n", u);
            pthread_mutex_unlock(&lock);
            return NULL;
        }
    }
    for (int j = 0; j < m; j++) {
        if (info->request[j] > available[j]) {
            printf("Error: Request exceeds Available for User U%d.\n", u);
            pthread_mutex_unlock(&lock);
            return NULL;
        }
    }

    // Tentative allocation
    for (int j = 0; j < m; j++) {
        available[j] -= info->request[j];
        allocation[u][j] += info->request[j];
        need[u][j] -= info->request[j];
    }

    int safe_seq[MAX_PROCESSES];
    if (find_safe_sequence(safe_seq)) {
        printf("Request granted to User U%d. New state:\n", u);
        print_state();
        printf("New safe sequence: ");
        for (int i = 0; i < n-1; i++) {
            printf("U%d->", safe_seq[i]);
        }
        printf("U%d\n", safe_seq[n-1]);
        print_available_matrix(safe_seq);
    } else {
        // Revert allocation if unsafe
        printf("Request leads to unsafe state. Reverting...\n");
        for (int j = 0; j < m; j++) {
            available[j] += info->request[j];
            allocation[u][j] -= info->request[j];
            need[u][j] += info->request[j];
        }
        print_state();
    }

    pthread_mutex_unlock(&lock);
    return NULL;
}

// Main function
int main() {
    pthread_mutex_init(&lock, NULL);

    printf("Enter number of users (max %d): ", MAX_PROCESSES);
    scanf("%d", &n);
    printf("Enter number of resource types (max %d): ", MAX_RESOURCES);
    scanf("%d", &m);

    // Read Allocation
    printf("\nEnter allocation for each user and resource:\n");
    for (int i = 0; i < n; i++) {
        printf("For User U%d: ", i);
        for (int j = 0; j < m; j++) {
            scanf("%d", &allocation[i][j]);
        }
    }

    // Read Max
    printf("\nEnter maximum resources for each user:\n");
    for (int i = 0; i < n; i++) {
        printf("For User U%d: ", i);
        for (int j = 0; j < m; j++) {
            scanf("%d", &max_need[i][j]);
        }
    }

    // Read Available
    printf("\nEnter available resources for each type: ");
    for (int j = 0; j < m; j++) {
        scanf("%d", &available[j]);
    }

    compute_need();

    printf("\nInitial State:\n");
    print_state();

    int safe_seq[MAX_PROCESSES];
    if (find_safe_sequence(safe_seq)) {
        printf("Safe sequence found: ");
        for (int i = 0; i < n-1; i++) {
            printf("U%d->", safe_seq[i]);
        }
        printf("U%d\n", safe_seq[n-1]);
        print_available_matrix(safe_seq);
    } else {
        printf("No safe sequence exists initially.\n");
    }

    int num_requests;
    printf("How many new resource requests to simulate? ");
    scanf("%d", &num_requests);

    pthread_t threads[num_requests];
    RequestInfo req_info[num_requests];

    for (int r = 0; r < num_requests; r++) {
        printf("\nRequest #%d:\n", r+1);
        int user_id;
        printf("Enter the user number that is requesting resources: ");
        scanf("%d", &user_id);

        // Handle new users
        if (user_id >= n) {
            user_id = n;
            if (user_id >= MAX_PROCESSES) {
                printf("Cannot add more users.\n");
                req_info[r].user_id = -1; 
                continue;
            }
            for (int j = 0; j < m; j++) {
                scanf("%d", &max_need[user_id][j]);
                allocation[user_id][j] = 0;
            }
            for (int j = 0; j < m; j++) {
                need[user_id][j] = max_need[user_id][j];
            }
            n++;
        }

        req_info[r].user_id = user_id;
        printf("Enter the request for each resource for User U%d: ", user_id);
        for (int j = 0; j < m; j++) {
            scanf("%d", &req_info[r].request[j]);
        }
    }

    for (int r = 0; r < num_requests; r++) {
        if (req_info[r].user_id >= 0) {
            pthread_create(&threads[r], NULL, request_resources, &req_info[r]);
        }
    }

    for (int r = 0; r < num_requests; r++) {
        if (req_info[r].user_id >= 0) {
            pthread_join(threads[r], NULL);
        }
    }

    pthread_mutex_destroy(&lock);

    printf("\nFinal State:\n");
    print_state();

    if (find_safe_sequence(safe_seq)) {
        printf("Final safe sequence: ");
        for (int i = 0; i < n-1; i++) {
            printf("U%d->", safe_seq[i]);
        }
        printf("U%d\n", safe_seq[n-1]);
        print_available_matrix(safe_seq);
    } else {
        printf("No safe sequence in final state.\n");
    }

    printf("Exiting program.\n");
    return 0;
}
