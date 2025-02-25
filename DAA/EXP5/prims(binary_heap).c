 #include <stdio.h>
 #include <stdlib.h>
 #include <stdbool.h>
 #include <limits.h>
 
 #define INF INT_MAX
 
 // ----------------- Adjacency List Structures -----------------
 typedef struct AdjNode {
     int vertex;
     int weight;
     struct AdjNode* next;
 } AdjNode;
 
 typedef struct {
     AdjNode** head;  // array of pointers to adjacency lists
     int V;
 } Graph;
 
 // Create a new adjacency node
 AdjNode* createNode(int v, int w) {
     AdjNode* newNode = (AdjNode*)malloc(sizeof(AdjNode));
     newNode->vertex = v;
     newNode->weight = w;
     newNode->next = NULL;
     return newNode;
 }
 
 // Initialize a graph with V vertices
 Graph* createGraph(int V) {
     Graph* graph = (Graph*)malloc(sizeof(Graph));
     graph->V = V;
     graph->head = (AdjNode**)malloc(V * sizeof(AdjNode*));
     for (int i = 0; i < V; i++) {
         graph->head[i] = NULL;
     }
     return graph;
 }
 
 // Add an undirected edge (u, v) of weight w to the graph
 void addEdge(Graph* graph, int u, int v, int w) {
     // Add edge (u->v)
     AdjNode* newNode = createNode(v, w);
     newNode->next = graph->head[u];
     graph->head[u] = newNode;
     // Since it's undirected, add edge (v->u)
     newNode = createNode(u, w);
     newNode->next = graph->head[v];
     graph->head[v] = newNode;
 }
 
 // ------------------- Min-Heap Structures ---------------------
 typedef struct {
     int vertex;
     int dist; // Key for the min-heap
 } MinHeapNode;
 
 typedef struct {
     int size;      // current number of elements in min heap
     int capacity;  // max capacity
     int *pos;      // pos[v] = index of vertex v in the heap array
     MinHeapNode *array;
 } MinHeap;
 
 // Create a new min heap
 MinHeap* createMinHeap(int capacity) {
     MinHeap* minHeap = (MinHeap*)malloc(sizeof(MinHeap));
     minHeap->pos = (int*)malloc(capacity * sizeof(int));
     minHeap->size = 0;
     minHeap->capacity = capacity;
     minHeap->array = (MinHeapNode*)malloc(capacity * sizeof(MinHeapNode));
     return minHeap;
 }
 
 // Swap two heap nodes
 void swapMinHeapNodes(MinHeapNode* a, MinHeapNode* b) {
     MinHeapNode temp = *a;
     *a = *b;
     *b = temp;
 }
 
 // Heapify at given idx
 void minHeapify(MinHeap* minHeap, int idx) {
     int smallest = idx;
     int left = 2 * idx + 1;
     int right = 2 * idx + 2;
 
     if (left < minHeap->size &&
         minHeap->array[left].dist < minHeap->array[smallest].dist) {
         smallest = left;
     }
     if (right < minHeap->size &&
         minHeap->array[right].dist < minHeap->array[smallest].dist) {
         smallest = right;
     }
     if (smallest != idx) {
         // Update positions
         MinHeapNode smallestNode = minHeap->array[smallest];
         MinHeapNode idxNode = minHeap->array[idx];
 
         minHeap->pos[smallestNode.vertex] = idx;
         minHeap->pos[idxNode.vertex] = smallest;
 
         // Swap
         swapMinHeapNodes(&minHeap->array[smallest], &minHeap->array[idx]);
         minHeapify(minHeap, smallest);
     }
 }
 
 // Check if minHeap is empty
 bool isEmpty(MinHeap* minHeap) {
     return minHeap->size == 0;
 }
 
 // Extract the node with minimum dist
 MinHeapNode extractMin(MinHeap* minHeap) {
     if (isEmpty(minHeap)) {
         // Return some invalid node
         MinHeapNode emptyNode = {-1, INF};
         return emptyNode;
     }
 
     // Store the root node
     MinHeapNode root = minHeap->array[0];
 
     // Replace root with last node
     MinHeapNode lastNode = minHeap->array[minHeap->size - 1];
     minHeap->array[0] = lastNode;
 
     // Update pos of last node
     minHeap->pos[root.vertex] = minHeap->size - 1;
     minHeap->pos[lastNode.vertex] = 0;
 
     // Reduce heap size and heapify
     minHeap->size--;
     minHeapify(minHeap, 0);
 
     return root;
 }
 
 // Decrease dist value of a given vertex v
 void decreaseKey(MinHeap* minHeap, int v, int dist) {
     // Get index of v in heap array
     int i = minHeap->pos[v];
     minHeap->array[i].dist = dist;
 
     // Travel up while the complete tree is not heapified
     while (i > 0 &&
            minHeap->array[i].dist < minHeap->array[(i - 1) / 2].dist) {
         // Swap this node with its parent
         minHeap->pos[minHeap->array[i].vertex] = (i - 1) / 2;
         minHeap->pos[minHeap->array[(i - 1) / 2].vertex] = i;
         swapMinHeapNodes(&minHeap->array[i],
                          &minHeap->array[(i - 1) / 2]);
 
         // move to parent index
         i = (i - 1) / 2;
     }
 }
 
 // Check if a vertex is in the min heap
 bool isInMinHeap(MinHeap *minHeap, int v) {
     if (minHeap->pos[v] < minHeap->size)
         return true;
     return false;
 }
 
 // --------------- Prim's Algorithm (Binary Heap) -------------
 void primMST_BinaryHeap(Graph* graph, int start) {
     int V = graph->V;
     int parent[V];  // Array to store MST
     int dist[V];    // dist[v] = best edge weight to connect v to MST
     bool inMST[V];  // True if vertex v is included in MST
 
     // Initialize
     for (int v = 0; v < V; v++) {
         dist[v] = INF;
         inMST[v] = false;
         parent[v] = -1;
     }
 
     // Distance of start vertex = 0 so that it is picked first
     dist[start] = 0;
 
     // Create min heap
     MinHeap* minHeap = createMinHeap(V);
     // Initialize min heap with all vertices
     for (int v = 0; v < V; v++) {
         minHeap->array[v].vertex = v;
         minHeap->array[v].dist   = dist[v];
         minHeap->pos[v]          = v;
     }
     minHeap->size = V;
 
     // The standard Prim's algorithm loop
     while (!isEmpty(minHeap)) {
         // Extract the vertex with minimum dist value
         MinHeapNode heapNode = extractMin(minHeap);
         int u = heapNode.vertex;
 
         inMST[u] = true; // Include u in MST
 
         // For each neighbor of u
         AdjNode* crawl = graph->head[u];
         while (crawl != NULL) {
             int v = crawl->vertex;
             int weight = crawl->weight;
 
             // If v is not in MST and weight < dist[v]
             if (!inMST[v] && weight < dist[v]) {
                 dist[v] = weight;
                 parent[v] = u;
                 // Decrease key in min heap
                 decreaseKey(minHeap, v, dist[v]);
             }
             crawl = crawl->next;
         }
     }
 
     // Print MST edges and total weight
     int mstWeight = 0;
     printf("\nEdges in the MST (Prim's with Binary Heap):\n");
     for (int v = 1; v < V; v++) {
         // parent[v] = u means edge (u, v)
         if (parent[v] != -1) {
             printf("Edge (%d -- %d) with weight %d\n",
                    parent[v], v, dist[v]);
             mstWeight += dist[v];
         }
     }
     printf("Total MST weight = %d\n", mstWeight);
 }

 int main() {
     int V, E;
     printf("=== Prim's Algorithm with Binary Heap ===\n");
     printf("Enter number of vertices: ");
     scanf("%d", &V);
     printf("Enter number of edges: ");
     scanf("%d", &E);
 
     Graph* graph = createGraph(V);
 
     printf("Enter edges (u v weight):\n");
     for (int i = 0; i < E; i++) {
         int u, v, w;
         scanf("%d %d %d", &u, &v, &w);
         addEdge(graph, u, v, w);
     }
 
     primMST_BinaryHeap(graph, 0);
 
     return 0;
 }
 
