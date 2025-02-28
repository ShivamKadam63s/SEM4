#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

// -------------------------- Adjacency List Structures --------------------------
typedef struct AdjNode {
    int vertex;             // destination vertex
    int weight;             // edge weight
    struct AdjNode* next;   // pointer to the next adjacency node
} AdjNode;

// For simplicity, we will assume the maximum number of vertices is not huge
// We'll store an adjacency list in a global array of pointers to AdjNodes
#define MAXV 10000
AdjNode* graph[MAXV];   // graph[i] is head of adjacency list for vertex i

// -------------------------- Min-Heap (Priority Queue) Structures --------------------------
typedef struct {
    int vertex;
    int dist;  // distance key
} HeapNode;

typedef struct {
    HeapNode* array;
    int size;      // number of elements currently in the heap
    int capacity;  // total capacity of the heap array
    // For quick decrease-key, we can keep an index-position map, but here we do a simpler approach.
} MinHeap;

// -------------------------- Utility: Create a new adjacency node --------------------------
AdjNode* createNode(int v, int w) {
    AdjNode* newNode = (AdjNode*)malloc(sizeof(AdjNode));
    newNode->vertex = v;
    newNode->weight = w;
    newNode->next = NULL;
    return newNode;
}

void buildGraph(int V, int E) {
    // Initialize adjacency lists
    for(int i = 0; i < V; i++) {
        graph[i] = NULL;
    }

    printf("Enter each edge in the format: src dest weight\n");
    for(int i = 0; i < E; i++) {
        int u, v, w;
        scanf("%d %d %d", &u, &v, &w);

        // Create a new adjacency node for edge (u -> v)
        AdjNode* newNode = createNode(v, w);
        newNode->next = graph[u];
        graph[u] = newNode;
    }
}

// -------------------------- Min-Heap Helper Functions --------------------------
MinHeap* createMinHeap(int capacity) {
    MinHeap* minHeap = (MinHeap*)malloc(sizeof(MinHeap));
    minHeap->array = (HeapNode*)malloc(capacity * sizeof(HeapNode));
    minHeap->size = 0;
    minHeap->capacity = capacity;
    return minHeap;
}

void swapHeapNodes(HeapNode* a, HeapNode* b) {
    HeapNode temp = *a;
    *a = *b;
    *b = temp;
}

// Moves the element at index 'idx' up the heap to maintain min-heap property
void heapifyUp(MinHeap* minHeap, int idx) {
    while(idx > 0) {
        int parent = (idx - 1) / 2;
        if(minHeap->array[idx].dist < minHeap->array[parent].dist) {
            swapHeapNodes(&minHeap->array[idx], &minHeap->array[parent]);
            idx = parent;
        } 
        else {
            break;
        }
    }
}

// Moves the element at index 'idx' down the heap to maintain min-heap property
void heapifyDown(MinHeap* minHeap, int idx) {
    int left, right, smallest;
    while(1) {
        left = 2 * idx + 1;
        right = 2 * idx + 2;
        smallest = idx;

        if(left < minHeap->size &&
           minHeap->array[left].dist < minHeap->array[smallest].dist) {
            smallest = left;
        }
        if(right < minHeap->size &&
           minHeap->array[right].dist < minHeap->array[smallest].dist) {
            smallest = right;
        }
        if(smallest != idx) {
            swapHeapNodes(&minHeap->array[idx], &minHeap->array[smallest]);
            idx = smallest;
        } else {
            break;
        }
    }
}

// Insert a new (vertex, dist) into the min-heap
void pushMinHeap(MinHeap* minHeap, int vertex, int dist) {
    if(minHeap->size == minHeap->capacity) {
        printf("Heap overflow!\n");
        return;
    }
    // Insert at the end
    int idx = minHeap->size;
    minHeap->size++;
    minHeap->array[idx].vertex = vertex;
    minHeap->array[idx].dist = dist;

    // Fix the min-heap property
    heapifyUp(minHeap, idx);
}

// Extract the node with the smallest dist
HeapNode popMinHeap(MinHeap* minHeap) {
    if(minHeap->size == 0) {
        // Return a dummy
        HeapNode dummy = {-1, INT_MAX};
        return dummy;
    }
    // The root of the heap is the min element
    HeapNode root = minHeap->array[0];

    // Move the last element to the root and reduce size
    minHeap->array[0] = minHeap->array[minHeap->size - 1];
    minHeap->size--;

    // Fix down
    heapifyDown(minHeap, 0);

    return root;
}

int isEmpty(MinHeap* minHeap) {
    return (minHeap->size == 0);
}

// Complexity: O(E log V) with a proper min-heap approach.
void dijkstra(int V, int source) {
    // dist[i] holds the shortest distance from 'source' to i
    int* dist = (int*)malloc(V * sizeof(int));
    // To keep track of which vertices are already "finalized"
    int* visited = (int*)calloc(V, sizeof(int));

    // Initialize all distances
    for(int i = 0; i < V; i++) {
        dist[i] = INT_MAX;
    }
    dist[source] = 0;

    // Create a min-heap and insert (source, 0)
    MinHeap* minHeap = createMinHeap(V);
    pushMinHeap(minHeap, source, 0);

    // While there are vertices in the min-heap
    while(!isEmpty(minHeap)) {
        // Extract the vertex with the smallest dist
        HeapNode top = popMinHeap(minHeap);
        int u = top.vertex;

        // If this vertex is already visited/finalized, skip
        if(visited[u]) continue;
        visited[u] = 1;

        // Relax edges from u
        AdjNode* temp = graph[u];
        while(temp != NULL) {
            int v = temp->vertex;
            int w = temp->weight;

            // If we can improve the distance to v, update and push
            if(!visited[v] && dist[u] != INT_MAX && dist[u] + w < dist[v]) {
                dist[v] = dist[u] + w;
                pushMinHeap(minHeap, v, dist[v]);
            }
            temp = temp->next;
        }
    }

    printf("\nShortest-path distances from source %d:\n", source);
    for(int i = 0; i < V; i++) {
        if(dist[i] == INT_MAX) {
            printf("Vertex %d: INF\n", i);
        } else {
            printf("Vertex %d: %d\n", i, dist[i]);
        }
    }

    free(dist);
    free(visited);
    free(minHeap->array);
    free(minHeap);
}

int main() {
    int V, E;
    int source;

    printf("Enter the number of vertices: ");
    scanf("%d", &V);
    printf("Enter the number of edges: ");
    scanf("%d", &E);

    buildGraph(V, E);

    printf("Enter the source vertex: ");
    scanf("%d", &source);
    dijkstra(V, source);

    return 0;
}