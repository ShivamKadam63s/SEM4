#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <math.h>

#define INF INT_MAX // Define infinity as the maximum integer value

// Structure for adjacency node in the graph
typedef struct AdjNode {
    int vertex;       // Vertex this node points to
    int weight;       // Weight of the edge to the vertex
    struct AdjNode* next;  // Pointer to the next adjacent node
} AdjNode;

// Structure for the graph, which uses an array of adjacency lists
typedef struct {
    AdjNode** head; // Array of adjacency list heads (each element is a list of AdjNode)
    int V;           // Number of vertices
} Graph;

// Function to create a new adjacency node (representing an edge)
AdjNode* createNode(int v, int w) {
    AdjNode* newNode = (AdjNode*)malloc(sizeof(AdjNode)); // Allocate memory for the node
    newNode->vertex = v;
    newNode->weight = w;
    newNode->next = NULL; // Initialize next to NULL
    return newNode;
}

// Function to create a graph with a specified number of vertices (V)
Graph* createGraph(int V) {
    Graph* graph = (Graph*)malloc(sizeof(Graph)); // Allocate memory for the graph
    graph->V = V;
    graph->head = (AdjNode**)malloc(V * sizeof(AdjNode*)); // Allocate memory for adjacency list heads

    // Initialize each adjacency list head to NULL
    for (int i = 0; i < V; i++) {
        graph->head[i] = NULL;
    }

    return graph;
}

// Function to add an undirected edge between vertices u and v with weight w
void addEdge(Graph* graph, int u, int v, int w) {
    // Create nodes for both directions (u -> v) and (v -> u)
    AdjNode* newNode = createNode(v, w);
    newNode->next = graph->head[u];
    graph->head[u] = newNode; // Add to the adjacency list of u

    newNode = createNode(u, w);
    newNode->next = graph->head[v];
    graph->head[v] = newNode; // Add to the adjacency list of v
}

// Structure for a Fibonacci heap node
typedef struct FibNode {
    int vertex;        // Vertex number
    int key;           // Key (priority) of the node
    int degree;        // Degree (number of children) of the node
    struct FibNode* parent;   // Parent node
    struct FibNode* child;    // Child node
    struct FibNode* left;     // Left sibling
    struct FibNode* right;    // Right sibling
    bool mark;          // A mark to indicate whether the node has lost a child
} FibNode;

// Structure for a Fibonacci heap
typedef struct FibHeap {
    FibNode* min;      // Pointer to the minimum node in the heap
    int n;             // Number of nodes in the heap
} FibHeap;

// Function to create a new Fibonacci heap node
FibNode* makeFibNode(int vertex, int key) {
    FibNode* node = (FibNode*)malloc(sizeof(FibNode)); // Allocate memory for the node
    node->vertex = vertex;
    node->key = key;
    node->degree = 0;        // Initially no children
    node->parent = NULL;     // No parent initially
    node->child = NULL;      // No children initially
    node->left = node;       // Initially points to itself
    node->right = node;      // Initially points to itself
    node->mark = false;      // Initially not marked
    return node;
}

// Function to create a new Fibonacci heap
FibHeap* makeFibHeap() {
    FibHeap* heap = (FibHeap*)malloc(sizeof(FibHeap)); // Allocate memory for the heap
    heap->min = NULL; // Initially, the heap is empty
    heap->n = 0;      // No nodes initially
    return heap;
}

// Function to insert a new node into the Fibonacci heap
void fibHeapInsert(FibHeap* H, FibNode* x) {
    x->degree = 0;     // Initialize degree to 0
    x->parent = NULL;  // Initially, no parent
    x->child = NULL;   // Initially, no children
    x->mark = false;   // Initially, not marked

    // Insert the node into the root list of the heap
    if (H->min == NULL) {
        H->min = x;     // If the heap is empty, set this node as the minimum
        x->left = x;
        x->right = x;
    } else {
        x->left = H->min;
        x->right = H->min->right;
        H->min->right->left = x;
        H->min->right = x;
        if (x->key < H->min->key) {
            H->min = x;  // Update the minimum node if needed
        }
    }

    H->n++;  // Increase the size of the heap
}

// Function to link two nodes in the Fibonacci heap (used in consolidation)
void fibHeapLink(FibHeap* H, FibNode* y, FibNode* x) {
    // Remove y from the root list
    y->left->right = y->right;
    y->right->left = y->left;
    
    // Make y a child of x
    y->parent = x;
    if (x->child == NULL) {
        x->child = y;    // If x has no children, make y its first child
        y->left = y;
        y->right = y;
    } else {
        y->left = x->child;
        y->right = x->child->right;
        x->child->right->left = y;
        x->child->right = y;  // Insert y into the child list of x
    }

    x->degree++; // Increment the degree of x
    y->mark = false; // Reset the mark on y
}

// Function to consolidate the Fibonacci heap (used after extracting the minimum)
void fibHeapConsolidate(FibHeap* H) {
    int D = (int)(floor(log2(H->n))) + 1;  // Max number of degrees possible
    FibNode** A = (FibNode**)calloc(D, sizeof(FibNode*)); // Array of pointers to nodes by degree

    // Count the number of root nodes
    int rootCount = 0;
    if (H->min != NULL) {
        FibNode* x = H->min;
        do {
            rootCount++;
            x = x->right;
        } while (x != H->min);
    }

    // Array to hold the root nodes
    FibNode** roots = (FibNode**)malloc(rootCount * sizeof(FibNode*));
    int idx = 0;
    if (H->min != NULL) {
        FibNode* x = H->min;
        do {
            roots[idx++] = x;
            x = x->right;
        } while (x != H->min);
    }

    // Consolidate nodes by linking nodes with the same degree
    for (int i = 0; i < rootCount; i++) {
        FibNode* x = roots[i];
        int d = x->degree;
        while (A[d] != NULL) {
            FibNode* y = A[d];
            if (x->key > y->key) {
                FibNode* temp = x;
                x = y;
                y = temp;  // Swap x and y to ensure x has the smaller key
            }
            fibHeapLink(H, y, x); // Link y to x
            A[d] = NULL;  // Set A[d] to NULL as we've processed this degree
            d++;  // Increase the degree
        }
        A[d] = x; // Place the node in the correct position in the array
    }
    free(roots); // Free the roots array

    // Update the min pointer to the minimum node
    H->min = NULL;
    for (int i = 0; i < D; i++) {
        if (A[i] != NULL) {
            A[i]->left = A[i];
            A[i]->right = A[i];
            if (H->min == NULL) {
                H->min = A[i]; // Set the first node as the minimum
            } else {
                A[i]->left = H->min;
                A[i]->right = H->min->right;
                H->min->right->left = A[i];
                H->min->right = A[i];
                if (A[i]->key < H->min->key) {
                    H->min = A[i]; // Update minimum node if necessary
                }
            }
        }
    }
    free(A); // Free the degree array
}

// Function to extract the minimum node from the Fibonacci heap
FibNode* fibHeapExtractMin(FibHeap* H) {
    FibNode* z = H->min; // Save the minimum node
    if (z != NULL) {
        // Move all the children of z to the root list
        FibNode* x = z->child;
        if (x != NULL) {
            int childCount = 0;
            FibNode* temp = x;
            do {
                childCount++;
                temp = temp->right;
            } while (temp != x);

            FibNode** children = (FibNode**)malloc(childCount * sizeof(FibNode*));
            int idx = 0;
            do {
                children[idx++] = x;
                x = x->right;
            } while (x != z->child);

            // Insert all children to the root list
            for (int i = 0; i < childCount; i++) {
                FibNode* child = children[i];
                child->parent = NULL;
                child->left = child;
                child->right = child;
                child->left = H->min;
                child->right = H->min->right;
                H->min->right->left = child;
                H->min->right = child;
            }
            free(children); // Free the children array
        }

        // Remove z from the root list
        z->left->right = z->right;
        z->right->left = z->left;

        if (z == z->right) {
            H->min = NULL; // If z is the only node, the heap is empty now
        } else {
            H->min = z->right;
            fibHeapConsolidate(H); // Consolidate the heap after removal
        }
        H->n--; // Decrease the size of the heap
    }
    return z;
}

// Function to decrease the key of a node in the Fibonacci heap
void fibHeapDecreaseKey(FibHeap* H, FibNode* x, int k) {
    if (k > x->key) {
        fprintf(stderr, "Error: new key is greater than current key\n");
        return;
    }
    x->key = k; // Update the key
    FibNode* y = x->parent;
    if (y != NULL && x->key < y->key) {
        // If the new key violates the heap property, cut the node
        x->left->right = x->right;
        x->right->left = x->left;
        if (y->child == x) {
            if (x->right != x) {
                y->child = x->right; // Update the child pointer if necessary
            } else {
                y->child = NULL;
            }
        }
        y->degree--; // Decrease the degree of the parent
        x->parent = NULL;
        x->left = H->min;
        x->right = H->min->right;
        H->min->right->left = x;
        H->min->right = x;
        x->mark = false; // Reset the mark on x

        // Continue cutting the parent nodes
        while (y != NULL && y->mark) {
            FibNode* p = y->parent;
            y->left->right = y->right;
            y->right->left = y->left;
            if (p != NULL && p->child == y) {
                if (y->right != y) {
                    p->child = y->right;
                } else {
                    p->child = NULL;
                }
            }
            if (p != NULL) {
                p->degree--;
            }
            y->parent = NULL;
            y->left = H->min;
            y->right = H->min->right;
            H->min->right->left = y;
            H->min->right = y;
            y->mark = false;
            y = p; // Move to the next parent node
        }
    }

    // If the node has become the minimum, update the heap minimum
    if (H->min == NULL || x->key < H->min->key) {
        H->min = x;
    }
}

// Array of Fibonacci heap nodes to be used in Prim's algorithm
FibNode** fibNodes;

// Function to run Prim's algorithm to find the Minimum Spanning Tree (MST) using Fibonacci Heap
void primMST_FibHeap(Graph* graph, int start) {
    int V = graph->V;
    int dist[V]; // Array to store the minimum distance for each vertex
    int parentArr[V]; // Array to store the parent of each vertex in the MST
    bool inMST[V]; // Array to track whether a vertex is included in the MST

    // Initialize all distances as infinity and parents as -1 (no parent)
    for (int i = 0; i < V; i++) {
        dist[i] = INF;
        parentArr[i] = -1;
        inMST[i] = false;
    }
    dist[start] = 0; // Distance of the start vertex is 0

    // Create a new Fibonacci Heap and insert all nodes with infinite key
    FibHeap* H = makeFibHeap();
    fibNodes = (FibNode**)malloc(V * sizeof(FibNode*));

    for (int v = 0; v < V; v++) {
        FibNode* node = makeFibNode(v, dist[v]);
        fibHeapInsert(H, node);
        fibNodes[v] = node; // Store the node in the fibNodes array
    }

    // Run Prim's algorithm
    while (H->n > 0) {
        FibNode* minNode = fibHeapExtractMin(H); // Extract the minimum node
        int u = minNode->vertex;
        inMST[u] = true; // Mark the vertex as included in MST

        // Traverse all adjacent vertices of u
        AdjNode* crawl = graph->head[u];
        while (crawl != NULL) {
            int v = crawl->vertex;
            int w = crawl->weight;

            // If v is not in MST and the weight is smaller, update the distance and parent
            if (!inMST[v] && w < dist[v]) {
                dist[v] = w;
                parentArr[v] = u;
                fibHeapDecreaseKey(H, fibNodes[v], w); // Decrease the key in Fibonacci heap
            }
            crawl = crawl->next;
        }
    }

    // Output the edges in the MST and the total weight
    int mstWeight = 0;
    printf("\nEdges in the MST (Prim's with Fibonacci Heap):\n");
    for (int v = 1; v < V; v++) {
        if (parentArr[v] != -1) {
            printf("Edge (%d -- %d) with weight %d\n", parentArr[v], v, dist[v]);
            mstWeight += dist[v]; // Add the weight of the edge
        }
    }
    printf("Total MST weight = %d\n", mstWeight);
    free(fibNodes); // Free the array of Fibonacci heap nodes
}

// Main function to run the program
int main() {
    int V, E;
    printf("Enter number of vertices: ");
    scanf("%d", &V); // Read the number of vertices
    printf("Enter number of edges: ");
    scanf("%d", &E); // Read the number of edges

    Graph* graph = createGraph(V); // Create a new graph
    printf("Enter edges (u v weight):\n");

    // Read edges and add them to the graph
    for (int i = 0; i < E; i++) {
        int u, v, w;
        scanf("%d %d %d", &u, &v, &w); // Read edge details
        addEdge(graph, u, v, w); // Add the edge to the graph
    }

    // Run Prim's MST algorithm starting from vertex 0
    primMST_FibHeap(graph, 0);
    return 0; // Return 0 to indicate successful completion
}
