#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

struct Process {
    int id, at, bt, ct, tat, wt, rt;
    int priority;
    // For non-preemptive, 'start' is the time at which process first got the CPU.
    // In preemptive, a process may have several CPU segments.
    int start; 
};

struct Segment {
    int start, end;
    int pid; // 0 indicates CPU Idle
};

// Comparator to sort processes by arrival time; if equal, by id.
int cmpArrival(const void *a, const void *b) {
    struct Process *p1 = (struct Process *)a;
    struct Process *p2 = (struct Process *)b;
    if(p1->at == p2->at)
        return p1->id - p2->id;
    return p1->at - p2->at;
}

// Print CPU timeline: each segment printed as: start(Pid_or Idle)end -> ... END
void printTimeline(struct Segment seg[], int count) {
    printf("CPU Timeline: ");
    for (int i = 0; i < count; i++) {
        printf("%d(", seg[i].start);
        if(seg[i].pid == 0)
            printf("Idle");
        else
            printf("P%d", seg[i].pid);
        printf(")%d -> ", seg[i].end);
    }
    printf("END\n");
}

// Compute waiting segments from the CPU timeline.
// For each process, its waiting intervals are computed as:
//   - The first waiting interval is from its arrival (AT) to the start of its first CPU segment.
//   - Subsequent waiting intervals are from the end of one CPU segment to the start of the next segment for that process.
void computeWaitingSegments(struct Segment cpuSeg[], int cpuCount, struct Process p[], int n, struct Segment waitSeg[], int *waitCount) {
    int count = 0;
    for (int i = 0; i < n; i++) {
        int first = 1;
        int last_end = p[i].at;  // initial waiting begins at arrival time
        for (int j = 0; j < cpuCount; j++) {
            if (cpuSeg[j].pid == p[i].id) {
                if (first) {
                    // Waiting interval from arrival to the start of first CPU segment.
                    if (cpuSeg[j].start >= p[i].at) {
                        waitSeg[count].start = p[i].at;
                        waitSeg[count].end = cpuSeg[j].start;
                        waitSeg[count].pid = p[i].id;
                        count++;
                    }
                    first = 0;
                    last_end = cpuSeg[j].end;
                } else {
                    // Waiting interval from end of previous segment to start of current segment.
                    if (cpuSeg[j].start >= last_end) {
                        waitSeg[count].start = last_end;
                        waitSeg[count].end = cpuSeg[j].start;
                        waitSeg[count].pid = p[i].id;
                        count++;
                    }
                    last_end = cpuSeg[j].end;
                }
            }
        }
    }
    *waitCount = count;
}

// Comparator for waiting segments: sort by start time; if equal, by process id.
int cmpWaitSegment(const void *a, const void *b) {
    struct Segment *s1 = (struct Segment *)a;
    struct Segment *s2 = (struct Segment *)b;
    if(s1->start == s2->start)
        return s1->pid - s2->pid;
    return s1->start - s2->start;
}

// Print waiting segments in order.
void printWaitingSegments(struct Segment waitSeg[], int waitCount) {
    // Sort waiting segments by start time.
    qsort(waitSeg, waitCount, sizeof(struct Segment), cmpWaitSegment);
    printf("Waiting Queue: ");
    for (int i = 0; i < waitCount; i++) {
        printf("%d(P%d)%d -> ", waitSeg[i].start, waitSeg[i].pid, waitSeg[i].end);
    }
    printf("END\n");
}

// ---------------- Scheduling Algorithms ----------------

// FCFS Scheduling
void fcfs(struct Process p[], int n) {
    // sort by arrival time (and id if equal)
    qsort(p, n, sizeof(struct Process), cmpArrival);
    
    struct Segment seg[1000];
    int segCount = 0;
    int time = 0;
    for (int i = 0; i < n; i++) {
        if (time < p[i].at) {
            // CPU idle segment
            seg[segCount].start = time;
            seg[segCount].end = p[i].at;
            seg[segCount].pid = 0;
            segCount++;
            time = p[i].at;
        }
        p[i].start = time; // first time process gets CPU
        seg[segCount].start = time;
        seg[segCount].end = time + p[i].bt;
        seg[segCount].pid = p[i].id;
        segCount++;
        time += p[i].bt;
        p[i].ct = time;
    }
    
    printTimeline(seg, segCount);
    struct Segment waitSeg[1000];
    int waitCount = 0;
    computeWaitingSegments(seg, segCount, p, n, waitSeg, &waitCount);
    printWaitingSegments(waitSeg, waitCount);
    
    // Calculate metrics
    for (int i = 0; i < n; i++) {
        p[i].tat = p[i].ct - p[i].at;
        p[i].wt  = p[i].tat - p[i].bt;
        p[i].rt  = p[i].start - p[i].at;
    }
}

// SJF Preemptive (Shortest Remaining Time First)
void sjf_preemptive(struct Process p[], int n) {
    struct Segment seg[10000];
    int segCount = 0;
    int time = 0, completed = 0;
    int last_pid = -1;
    int rem[100];
    // For each process, initialize remaining time and ensure start is -1.
    for (int i = 0; i < n; i++) {
        rem[i] = p[i].bt;
        p[i].start = -1;
    }
    
    while (completed < n) {
        int idx = -1;
        int minRem = INT_MAX;
        // Find process with smallest remaining time among those that have arrived.
        for (int i = 0; i < n; i++) {
            if (p[i].at <= time && rem[i] > 0 && rem[i] < minRem) {
                minRem = rem[i];
                idx = i;
            } else if (p[i].at <= time && rem[i] > 0 && rem[i] == minRem) {
                if (p[i].id < p[idx].id)
                    idx = i;
            }
        }
        if (idx == -1) {
            // CPU idle for one time unit.
            if (last_pid != 0) {
                seg[segCount].start = time;
                seg[segCount].end = time + 1;
                seg[segCount].pid = 0;
                segCount++;
                last_pid = 0;
            } else {
                seg[segCount - 1].end++;
            }
            time++;
            continue;
        }
        // If process is newly scheduled (after waiting), record its first start time.
        if(p[idx].start == -1)
            p[idx].start = time;
        // If the currently running process is the same as last, extend its segment.
        if (last_pid == p[idx].id) {
            seg[segCount - 1].end++;
        } else {
            seg[segCount].start = time;
            seg[segCount].end = time + 1;
            seg[segCount].pid = p[idx].id;
            segCount++;
            last_pid = p[idx].id;
        }
        rem[idx]--;  // run for 1 time unit
        time++;
        if (rem[idx] == 0) {
            p[idx].ct = time;
            completed++;
        }
    }
    printTimeline(seg, segCount);
    struct Segment waitSeg[1000];
    int waitCount = 0;
    computeWaitingSegments(seg, segCount, p, n, waitSeg, &waitCount);
    printWaitingSegments(waitSeg, waitCount);
    
    for (int i = 0; i < n; i++) {
        p[i].tat = p[i].ct - p[i].at;
        p[i].wt  = p[i].tat - p[i].bt;
        p[i].rt  = p[i].start - p[i].at;
    }
}

// SJF Non-Preemptive Scheduling
void sjf_non_preemptive(struct Process p[], int n) {
    struct Segment seg[1000];
    int segCount = 0;
    int time = 0, completed = 0;
    int done[100] = {0};
    // For each process, reset start.
    for (int i = 0; i < n; i++) {
        p[i].start = -1;
    }
    while (completed < n) {
        int idx = -1, minBT = INT_MAX;
        for (int i = 0; i < n; i++) {
            if (!done[i] && p[i].at <= time && p[i].bt < minBT) {
                minBT = p[i].bt;
                idx = i;
            } else if (!done[i] && p[i].at <= time && p[i].bt == minBT && p[i].id < p[idx].id) {
                idx = i;
            }
        }
        if (idx == -1) {
            // CPU idle for one unit.
            if(segCount > 0 && seg[segCount - 1].pid == 0)
                seg[segCount - 1].end++;
            else {
                seg[segCount].start = time;
                seg[segCount].end = time + 1;
                seg[segCount].pid = 0;
                segCount++;
            }
            time++;
            continue;
        }
        p[idx].start = time;
        seg[segCount].start = time;
        seg[segCount].end = time + p[idx].bt;
        seg[segCount].pid = p[idx].id;
        segCount++;
        time += p[idx].bt;
        p[idx].ct = time;
        done[idx] = 1;
        completed++;
    }
    printTimeline(seg, segCount);
    struct Segment waitSeg[1000];
    int waitCount = 0;
    computeWaitingSegments(seg, segCount, p, n, waitSeg, &waitCount);
    printWaitingSegments(waitSeg, waitCount);
    
    for (int i = 0; i < n; i++) {
        p[i].tat = p[i].ct - p[i].at;
        p[i].wt  = p[i].tat - p[i].bt;
        p[i].rt  = p[i].start - p[i].at;
    }
}

void priority_preemptive(struct Process p[], int n) {
    int rem[100];
    for (int i = 0; i < n; i++) {
        rem[i] = p[i].bt;
        p[i].start = -1;
    }
    
    struct Segment seg[10000];
    int segCount = 0;
    int time = 0, completed = 0;
    int last_pid = -1;  // tracks the process id that was running last (0 for Idle)
    
    while (completed < n) {
        int idx = -1;
        int best = INT_MAX;
        // Find the process with the highest priority (lowest numerical value) among those that have arrived.
        for (int i = 0; i < n; i++) {
            if (p[i].at <= time && rem[i] > 0 && p[i].priority < best) {
                best = p[i].priority;
                idx = i;
            } else if (p[i].at <= time && rem[i] > 0 && p[i].priority == best) {
                if (idx == -1 || p[i].id < p[idx].id)
                    idx = i;
            }
        }
        if (idx == -1) {
            // CPU idle for one time unit.
            if (last_pid != 0) {
                seg[segCount].start = time;
                seg[segCount].end = time + 1;
                seg[segCount].pid = 0;
                segCount++;
                last_pid = 0;
            } else {
                seg[segCount - 1].end++;
            }
            time++;
            continue;
        }
        // Record the first time this process gets the CPU.
        if (p[idx].start == -1)
            p[idx].start = time;
        // If the same process continues running, extend its segment; otherwise, create a new segment.
        if (last_pid == p[idx].id) {
            seg[segCount - 1].end++;
        } else {
            seg[segCount].start = time;
            seg[segCount].end = time + 1;
            seg[segCount].pid = p[idx].id;
            segCount++;
            last_pid = p[idx].id;
        }
        // Execute process for 1 time unit.
        rem[idx]--;
        time++;
        if (rem[idx] == 0) {
            p[idx].ct = time;
            completed++;
        }
    }
    // Print the CPU timeline.
    printTimeline(seg, segCount);
    // Compute and print waiting intervals.
    struct Segment waitSeg[1000];
    int waitCount = 0;
    computeWaitingSegments(seg, segCount, p, n, waitSeg, &waitCount);
    printWaitingSegments(waitSeg, waitCount);
    
    // Calculate final metrics.
    for (int i = 0; i < n; i++) {
        p[i].tat = p[i].ct - p[i].at;
        p[i].wt  = p[i].tat - p[i].bt;
        p[i].rt  = p[i].start - p[i].at;
    }
}


// Priority Scheduling (Non-Preemptive); lower priority value means higher priority.
void priority_scheduling(struct Process p[], int n) {
    struct Segment seg[1000];
    int segCount = 0;
    int time = 0, completed = 0;
    int done[100] = {0};
    for (int i = 0; i < n; i++) {
        p[i].start = -1;
    }
    while (completed < n) {
        int idx = -1, best = INT_MAX;
        for (int i = 0; i < n; i++) {
            if (!done[i] && p[i].at <= time && p[i].priority < best) {
                best = p[i].priority;
                idx = i;
            } else if (!done[i] && p[i].at <= time && p[i].priority == best && p[i].id < p[idx].id) {
                idx = i;
            }
        }
        if (idx == -1) {
            if(segCount > 0 && seg[segCount - 1].pid == 0)
                seg[segCount - 1].end++;
            else {
                seg[segCount].start = time;
                seg[segCount].end = time + 1;
                seg[segCount].pid = 0;
                segCount++;
            }
            time++;
            continue;
        }
        p[idx].start = time;
        seg[segCount].start = time;
        seg[segCount].end = time + p[idx].bt;
        seg[segCount].pid = p[idx].id;
        segCount++;
        time += p[idx].bt;
        p[idx].ct = time;
        done[idx] = 1;
        completed++;
    }
    printTimeline(seg, segCount);
    struct Segment waitSeg[1000];
    int waitCount = 0;
    computeWaitingSegments(seg, segCount, p, n, waitSeg, &waitCount);
    printWaitingSegments(waitSeg, waitCount);
    
    for (int i = 0; i < n; i++) {
        p[i].tat = p[i].ct - p[i].at;
        p[i].wt  = p[i].tat - p[i].bt;
        p[i].rt  = p[i].start - p[i].at;
    }
}

// Round Robin Scheduling
void round_robin(struct Process p[], int n, int tq) {
    int rem[100];
    for (int i = 0; i < n; i++) {
        rem[i] = p[i].bt;
        p[i].start = -1;
    }
    int time = 0, completed = 0;
    int queue[100], front = 0, rear = 0;
    int inQueue[100] = {0};
    
    // Enqueue processes that have arrived at time 0.
    for (int i = 0; i < n; i++) {
        if(p[i].at <= time) {
            queue[rear++] = i;
            inQueue[i] = 1;
        }
    }
    
    struct Segment seg[10000];
    int segCount = 0, last_pid = -1;
    while (completed < n) {
        if (front == rear) {
            // CPU idle: advance time to next arrival.
            if(last_pid != 0) {
                seg[segCount].start = time;
                seg[segCount].end = time + 1;
                seg[segCount].pid = 0;
                segCount++;
                last_pid = 0;
            } else {
                seg[segCount - 1].end++;
            }
            time++;
            for (int i = 0; i < n; i++) {
                if(p[i].at <= time && rem[i] > 0 && !inQueue[i]) {
                    queue[rear++] = i;
                    inQueue[i] = 1;
                }
            }
            continue;
        }
        int idx = queue[front++];
        inQueue[idx] = 0;
        if(p[idx].start == -1)
            p[idx].start = time;
        int run = (rem[idx] < tq) ? rem[idx] : tq;
        if(last_pid == p[idx].id) {
            seg[segCount - 1].end = time + run;
        } else {
            seg[segCount].start = time;
            seg[segCount].end = time + run;
            seg[segCount].pid = p[idx].id;
            segCount++;
            last_pid = p[idx].id;
        }
        time += run;
        rem[idx] -= run;
        // Enqueue any new arrivals during this time slice.
        for (int i = 0; i < n; i++) {
            if(p[i].at > time - run && p[i].at <= time && rem[i] > 0 && !inQueue[i]) {
                queue[rear++] = i;
                inQueue[i] = 1;
            }
        }
        if(rem[idx] > 0) {
            queue[rear++] = idx;
            inQueue[idx] = 1;
        } else {
            p[idx].ct = time;
            completed++;
        }
    }
    printTimeline(seg, segCount);
    struct Segment waitSeg[1000];
    int waitCount = 0;
    computeWaitingSegments(seg, segCount, p, n, waitSeg, &waitCount);
    printWaitingSegments(waitSeg, waitCount);
    
    for (int i = 0; i < n; i++) {
        p[i].tat = p[i].ct - p[i].at;
        p[i].wt  = p[i].tat - p[i].bt;
        p[i].rt  = p[i].start - p[i].at;
    }
}

// ---------------- Main Function ----------------

void displayMetrics(struct Process p[], int n) {
    printf("\nID\tAT\tBT\tCT\tTAT\tWT\tRT\n");
    for (int i = 0; i < n; i++) {
        printf("P%d\t%d\t%d\t%d\t%d\t%d\t%d\n", p[i].id, p[i].at, p[i].bt, p[i].ct, p[i].tat, p[i].wt, p[i].rt);
    }
}

int main() {
    int n, choice, tq;
    printf("Enter number of devotees: ");
    scanf("%d", &n);
    struct Process p[100];
    for (int i = 0; i < n; i++) {
        p[i].id = i + 1;
        printf("Enter Arrival Time (AT) and Burst Time (BT) for P%d: ", p[i].id);
        scanf("%d %d", &p[i].at, &p[i].bt);
        p[i].ct = p[i].tat = p[i].wt = p[i].rt = 0;
        p[i].priority = 0;
        p[i].start = -1;
    }
    printf("\nChoose Scheduling Algorithm:\n");
    printf("1. FCFS\n");
    printf("2. SJF (Preemptive)\n");
    printf("3. SJF (Non-Preemptive)\n");
    printf("4. Priority Scheduling (Preemptive)\n");
    printf("5. Priority Scheduling (Non-Preemptive)\n");
    printf("6. Round Robin\n");
    printf("Enter choice: ");
    scanf("%d", &choice);
    if(choice == 4 || choice == 5) {
        for (int i = 0; i < n; i++) {
            printf("Enter priority for P%d (lower value => higher priority): ", p[i].id);
            scanf("%d", &p[i].priority);
        }
    }
    if(choice == 6) {
        printf("Enter Time Quantum: ");
        scanf("%d", &tq);
    }
    
    // Execute selected algorithm.
    switch(choice) {
        case 1:
            fcfs(p, n);
            break;
        case 2:
            sjf_preemptive(p, n);
            break;
        case 3:
            sjf_non_preemptive(p, n);
            break;
        case 4:
            priority_preemptive(p,n);
            break;
        case 5:
            priority_scheduling(p, n);
            break;
        case 6:
            round_robin(p, n, tq);
            break;
        default:
            printf("Invalid choice.\n");
            return 0;
    }
    displayMetrics(p, n);
    return 0;
}
