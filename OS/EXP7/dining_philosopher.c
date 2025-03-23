#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define N 5

// Philosopher states
#define THINKING 0
#define HUNGRY 1
#define EATING 2

// Shared variables
int state[N];                   // Array to keep track of each philosopher's state
pthread_mutex_t mutex;          // Mutex for monitor
pthread_cond_t cond[N];         // Condition variable for each philosopher

// Test if philosopher i can start eating
void test(int i) {
    int left = (i + N - 1) % N;   // index of left neighbor
    int right = (i + 1) % N;      // index of right neighbor

    // If philosopher i is hungry and both neighbors are not eating, allow i to eat
    if (state[i] == HUNGRY && state[left] != EATING && state[right] != EATING) {
        state[i] = EATING;
        // Signal philosopher i that they can now eat
        pthread_cond_signal(&cond[i]);
    }
}

// Pickup chopsticks (entering the monitor)
void pickup(int i) {
    pthread_mutex_lock(&mutex);  // Enter monitor

    state[i] = HUNGRY;
    test(i);                   // Try to acquire chopsticks

    // Wait until signaled that chopsticks are available
    while (state[i] != EATING)
        pthread_cond_wait(&cond[i], &mutex);

    pthread_mutex_unlock(&mutex); // Exit monitor
}

// Put down chopsticks (leaving the monitor)
void putdown(int i) {
    pthread_mutex_lock(&mutex);  // Enter monitor

    state[i] = THINKING;         // Philosopher is done eating

    // Test if left and right neighbors can now eat
    test((i + N - 1) % N);
    test((i + 1) % N);

    pthread_mutex_unlock(&mutex); // Exit monitor
}

// Philosopher routine
void *philosopher(void *arg) {
    int i = *(int *)arg;
    while (1) {
        // Philosopher is thinking
        printf("Philosopher %d is thinking.\n", i);
        sleep(rand() % 3 + 1); // Think for a random period (1-3 seconds)

        // Philosopher gets hungry and tries to pick up chopsticks
        pickup(i);

        // Philosopher is eating
        printf("Philosopher %d is eating.\n", i);
        sleep(rand() % 2 + 1); // Eat for a random period (1-2 seconds)

        // Philosopher puts down chopsticks and goes back to thinking
        putdown(i);
    }
    return NULL;
}

int main() {
    int i;
    pthread_t thread_id[N];
    int phil[N];
    pthread_mutex_init(&mutex, NULL);

    // Initialize condition variables and state for each philosopher
    for (i = 0; i < N; i++) {
        pthread_cond_init(&cond[i], NULL);
        state[i] = THINKING;
        phil[i] = i;
    }
    for (i = 0; i < N; i++) {
        pthread_create(&thread_id[i], NULL, philosopher, &phil[i]);
    }
    //below part is never reached in this infinite simulation
    for (i = 0; i < N; i++) {
        pthread_join(thread_id[i], NULL);
    }
    pthread_mutex_destroy(&mutex);
    for (i = 0; i < N; i++) {
        pthread_cond_destroy(&cond[i]);
    }

    return 0;
}
