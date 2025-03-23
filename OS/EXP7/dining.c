/*
* Dining Philosophers Simulation using Shared Memory, Manual Scheduling,
* and a Predefined Command Sequence (Manual Mode)
*
* Five philosophers share five forks (stored in shared memory) and each
* follows a 6-step routine:
*
*   1. Think()           - runs automatically (no command needed)
*   2. Pick LEFT fork     - fork operations use a command from a global sequence
*   3. Pick RIGHT fork
*   4. Eat()              - runs automatically
*   5. Release LEFT fork
*   6. Release RIGHT fork
*
* At startup the user is prompted once:
*   a) Simulation mode: (0 = automatic, 1 = manual, -1 = exit)
*   b) If manual is chosen, a sequence of commands (space‐separated integers)
*      is read from standard input. This sequence is used for all fork operations.
*      (A command of 0 means “go on”, 1 means “context switch” and –1 means “exit”.)
*
* In automatic mode all fork operations proceed without interruption.
*
* In manual mode, when a philosopher reaches a fork operation it obtains
* the next command from the command sequence. For example, if philosopher 0,
* while trying to pick up its left fork, reads a 0 then it picks it up.
* Later, if the next fork operation for the same philosopher returns a 1,
* it saves its state and yields control. Only then does the next philosopher run.
*
* Deadlock is detected when every philosopher holds its left fork (i.e.
* for each i, shared->forks[i] == i). In that case, a deadlock message is printed
* and the simulation terminates.
*
* Compile with: gcc -o dining dining.c -lpthread
*/


#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>
#include <sched.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#define NUM_PHILOSOPHERS 5
#define STACK_SIZE 10
#define MAX_CMD_COUNT 100
// Enumeration for the 6 steps.
typedef enum {
    STATE_THINK = 1,       // 1. Think()
    STATE_PICK_LEFT,       // 2. Pick LEFT fork (fork index = philosopher id)
    STATE_PICK_RIGHT,      // 3. Pick RIGHT fork (fork index = (id+1)%5)
    STATE_EAT,             // 4. Eat()
    STATE_RELEASE_LEFT,    // 5. Release LEFT fork
    STATE_RELEASE_RIGHT    // 6. Release RIGHT fork
} State;
// Philosopher structure (local to each thread).
typedef struct {
    int id;
    State state;              // current step
    State stateStack[STACK_SIZE];
    int top;                  // top index for stateStack (-1 means empty)
} Philosopher;
// Shared memory structure.
typedef struct {
    int forks[NUM_PHILOSOPHERS];   // Each fork: -1 means free; otherwise holds philosopher id.
    int terminate_simulation;      // 0 = running, 1 = terminate.
} SharedData;
SharedData *shared = NULL;  // Pointer to shared memory.
// Global scheduler variables: only one philosopher runs at a time.
int current_philosopher = 0;
pthread_mutex_t sched_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t sched_cond = PTHREAD_COND_INITIALIZER;
// Global mutex for printing and shared memory updates.
pthread_mutex_t global_mutex = PTHREAD_MUTEX_INITIALIZER;
// Simulation mode: 0 = automatic, 1 = manual.
int simulation_mode = 0;
// In manual mode, a global command sequence is read at startup.
int commandSequence[MAX_CMD_COUNT];
int cmdCount = 0;
int cmdIndex = 0;
// --- Scheduler Functions ---
// Wait until it is this philosopher's turn.
void wait_for_turn(int id) {
    pthread_mutex_lock(&sched_mutex);
    while (current_philosopher != id && shared->terminate_simulation == 0) {
        pthread_cond_wait(&sched_cond, &sched_mutex);
    }
    pthread_mutex_unlock(&sched_mutex);
}
// Yield control to the next philosopher (round-robin).
void yield_scheduler() {
    pthread_mutex_lock(&sched_mutex);
    current_philosopher = (current_philosopher + 1) % NUM_PHILOSOPHERS;
    pthread_cond_broadcast(&sched_cond);
    pthread_mutex_unlock(&sched_mutex);
}
// --- Philosopher Helper Functions ---
// Save current state (push onto the philosopher's state stack).
void push_state(Philosopher *p) {
    if (p->top < STACK_SIZE - 1) {
        p->top++;
        p->stateStack[p->top] = p->state;
    } else {
        pthread_mutex_lock(&global_mutex);
        printf("Philosopher %d: Stack overflow!\n", p->id);
        pthread_mutex_unlock(&global_mutex);
    }
}
// (Optional) Pop state from the philosopher's state stack.
State pop_state(Philosopher *p) {
    if (p->top >= 0) {
        State s = p->stateStack[p->top];
        p->top--;
        return s;
    }
    return p->state;
}
// For fork operations, if in manual mode, take the next command from the sequence.
// In automatic mode, always return 0.
int get_next_command() {
    if (simulation_mode == 1) {
        if (cmdIndex < cmdCount) {
            return commandSequence[cmdIndex++];
        } else {
            // If we run out of commands, default to 0.
            return 0;
        }
    }
    return 0;
}
// For nonfork steps (thinking, eating), just print the action.
void do_action_no_prompt(int phil_id, const char *action_desc) {
    pthread_mutex_lock(&global_mutex);
    printf("Philosopher %d %s\n", phil_id, action_desc);
    pthread_mutex_unlock(&global_mutex);
}
// Check for deadlock: if for every i, shared->forks[i] == i, then deadlock.
bool deadlock_detected() {
    bool deadlock = true;
    pthread_mutex_lock(&global_mutex);
    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        if (shared->forks[i] != i) {
            deadlock = false;
            break;
        }
    }
    if (deadlock) {
        printf("\nDEADLOCK DETECTED: Every philosopher holds its LEFT fork!\n");
        shared->terminate_simulation = 1;
    }
    pthread_mutex_unlock(&global_mutex);
    return deadlock;
}
// --- Philosopher Routine ---
// Only one philosopher runs at a time. At fork operations, the next command from the
// command sequence is used: 0 = execute operation, 1 = context switch (save state and yield),
// -1 = exit simulation.
void *philosopher(void *arg) {
    Philosopher *p = (Philosopher *)arg;
    while (shared->terminate_simulation == 0) {
        // Wait for our turn.
        wait_for_turn(p->id);
        int cmd = 0;  // command for fork operations
        switch(p->state) {
            case STATE_THINK:
                // Thinking runs automatically.
                do_action_no_prompt(p->id, "is Thinking.");
                p->state = STATE_PICK_LEFT;
                yield_scheduler();
                break;
            case STATE_PICK_LEFT: {
                // Use the next command (do not prompt again).
                cmd = get_next_command();
                pthread_mutex_lock(&global_mutex);
                printf("Philosopher %d wants to pick up LEFT fork %d: command = %d\n", p->id, p->id, cmd);
                pthread_mutex_unlock(&global_mutex);
                if (cmd == -1) {
                    pthread_mutex_lock(&global_mutex);
                    shared->terminate_simulation = 1;
                    pthread_mutex_unlock(&global_mutex);
                    yield_scheduler();
                    break;
                }
                if (cmd == 1) {
                    push_state(p);
                    yield_scheduler();
                    break;
                }
                // Command 0: go on.
                pthread_mutex_lock(&global_mutex);
                if (shared->forks[p->id] == -1) {
                    shared->forks[p->id] = p->id;
                    printf("Philosopher %d picked up LEFT fork %d.\n", p->id, p->id);
                    p->state = STATE_PICK_RIGHT;
                } else {
                    printf("Philosopher %d failed to pick up LEFT fork %d (already taken).\n", p->id, p->id);
                }
                pthread_mutex_unlock(&global_mutex);
                yield_scheduler();
                break;
            }
            case STATE_PICK_RIGHT: {
                int right_fork = (p->id + 1) % NUM_PHILOSOPHERS;
                cmd = get_next_command();
                pthread_mutex_lock(&global_mutex);
                printf("Philosopher %d wants to pick up RIGHT fork %d: command = %d\n", p->id, right_fork, cmd);
                pthread_mutex_unlock(&global_mutex);
                if (cmd == -1) {
                    pthread_mutex_lock(&global_mutex);
                    shared->terminate_simulation = 1;
                    pthread_mutex_unlock(&global_mutex);
                    yield_scheduler();
                    break;
                }
                if (cmd == 1) {
                    push_state(p);
                    yield_scheduler();
                    break;
                }
                pthread_mutex_lock(&global_mutex);
                if (shared->forks[right_fork] == -1) {
                    shared->forks[right_fork] = p->id;
                    printf("Philosopher %d picked up RIGHT fork %d.\n", p->id, right_fork);
                    p->state = STATE_EAT;
                } else {
                    printf("Philosopher %d failed to pick up RIGHT fork %d (already taken).\n", p->id, right_fork);
                }
                pthread_mutex_unlock(&global_mutex);
                yield_scheduler();
                break;
            }
            case STATE_EAT:
                // Eating runs automatically.
                do_action_no_prompt(p->id, "is Eating.");
                p->state = STATE_RELEASE_LEFT;
                yield_scheduler();
                break;
            case STATE_RELEASE_LEFT: {
                cmd = get_next_command();
                pthread_mutex_lock(&global_mutex);
                printf("Philosopher %d wants to release LEFT fork %d: command = %d\n", p->id, p->id, cmd);
                pthread_mutex_unlock(&global_mutex);
                if (cmd == -1) {
                    pthread_mutex_lock(&global_mutex);
                    shared->terminate_simulation = 1;
                    pthread_mutex_unlock(&global_mutex);
                    yield_scheduler();
                    break;
                }
                if (cmd == 1) {
                    push_state(p);
                    yield_scheduler();
                    break;
                }
                pthread_mutex_lock(&global_mutex);
                shared->forks[p->id] = -1;
                printf("Philosopher %d released LEFT fork %d.\n", p->id, p->id);
                pthread_mutex_unlock(&global_mutex);
                p->state = STATE_RELEASE_RIGHT;
                yield_scheduler();
                break;
            }
            case STATE_RELEASE_RIGHT: {
                int right_fork = (p->id + 1) % NUM_PHILOSOPHERS;
                cmd = get_next_command();
                pthread_mutex_lock(&global_mutex);
                printf("Philosopher %d wants to release RIGHT fork %d: command = %d\n", p->id, right_fork, cmd);
                pthread_mutex_unlock(&global_mutex);
                if (cmd == -1) {
                    pthread_mutex_lock(&global_mutex);
                    shared->terminate_simulation = 1;
                    pthread_mutex_unlock(&global_mutex);
                    yield_scheduler();
                    break;
                }
                if (cmd == 1) {
                    push_state(p);
                    yield_scheduler();
                    break;
                }
                pthread_mutex_lock(&global_mutex);
                shared->forks[right_fork] = -1;
                printf("Philosopher %d released RIGHT fork %d.\n", p->id, right_fork);
                pthread_mutex_unlock(&global_mutex);
                // End of cycle; go back to Thinking.
                p->state = STATE_THINK;
                yield_scheduler();
                break;
            }
            default:
                break;
        } // end switch
        // Check for deadlock after each step.
        if (deadlock_detected())
            break;
        usleep(100000);
    }
    return NULL;
}
int main() {
    int initMode;
    printf("Enter simulation mode (0 = Auto, 1 = Manual for fork operations, -1 = Exit): ");
    scanf("%d", &initMode);
    if (initMode == -1) {
        printf("Exiting simulation.\n");
        exit(0);
    }
    simulation_mode = initMode;
    if (simulation_mode == 1) {
        printf("Enter up to %d commands (space separated) for fork operations:\n", MAX_CMD_COUNT);
        cmdCount = 0;
        while (cmdCount < MAX_CMD_COUNT && scanf("%d", &commandSequence[cmdCount]) == 1) {
            if (commandSequence[cmdCount] == -1) {
                // If -1 appears in the sequence, that command will terminate the simulation.
                cmdCount++;
                break;
            }
            cmdCount++;
        }
    }
    // Set up shared memory.
    key_t shm_key = ftok("dining_philosophers", 65);
    int shm_id = shmget(shm_key, sizeof(SharedData), IPC_CREAT | 0666);
    if (shm_id < 0) {
        perror("shmget failed");
        exit(1);
    }
    shared = (SharedData *)shmat(shm_id, NULL, 0);
    if (shared == (void *) -1) {
        perror("shmat failed");
        exit(1);
    }
    // Initialize shared memory.
    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        shared->forks[i] = -1;
    }
    shared->terminate_simulation = 0;
    // Create philosopher structures and threads.
    Philosopher philosophers[NUM_PHILOSOPHERS];
    pthread_t threads[NUM_PHILOSOPHERS];
    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        philosophers[i].id = i;
        philosophers[i].state = STATE_THINK;
        philosophers[i].top = -1;
        if (pthread_create(&threads[i], NULL, philosopher, &philosophers[i]) != 0) {
            perror("Failed to create thread");
            exit(1);
        }
    }
    // Wait for threads to finish.
    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        pthread_join(threads[i], NULL);
    }
    pthread_mutex_lock(&global_mutex);
    printf("Simulation terminated.\n");
    pthread_mutex_unlock(&global_mutex);
    // Detach and remove shared memory.
    shmdt(shared);
    shmctl(shm_id, IPC_RMID, NULL);
    return 0;
}
