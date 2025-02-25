#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int u, v;
    int weight;
} Edge;

// A utility function to swap two edges (for use in quicksort or any sorting method)
void swap(Edge *a, Edge *b) {
    Edge temp = *a;
    *a = *b;
    *b = temp;
}

// A comparison function to sort edges by weight (ascending)
int compareEdges(const void *a, const void *b) {
    Edge *e1 = (Edge *)a;
    Edge *e2 = (Edge *)b;
    return e1->weight - e2->weight;
}

// Disjoint Set (Union-Find) data structure
// parent[i] = parent of i
// rank[i]   = rank (roughly the tree height) of i
int findSet(int parent[], int x) {
    // Path compression heuristic
    if (parent[x] != x) {
        parent[x] = findSet(parent, parent[x]);
    }
    return parent[x];
}

// Union by rank heuristic
void unionSet(int parent[], int rank[], int x, int y) {
    int rootX = findSet(parent, x);
    int rootY = findSet(parent, y);

    if (rootX != rootY) {
        if (rank[rootX] > rank[rootY]) {
            parent[rootY] = rootX;
        } else if (rank[rootX] < rank[rootY]) {
            parent[rootX] = rootY;
        } else {
            parent[rootY] = rootX;
            rank[rootX]++;
        }
    }
}

// Detect cycle function: returns 1 if edge (u, v) forms a cycle, 0 otherwise
int detectCycle(int u, int v, int parent[]) {
    int rootU = findSet(parent, u);
    int rootV = findSet(parent, v);
    if (rootU == rootV) {
        return 1;  // They are in the same set, so it forms a cycle
    }
    return 0;
}

// Kruskal's algorithm function
// Parameters:
//   V: number of vertices
//   E: number of edges
//   edges: array of edges
void kruskalMST(int V, int E, Edge edges[]) {
    // Sort edges by ascending weight
    qsort(edges, E, sizeof(Edge), compareEdges);

    // Allocate memory for Disjoint Set arrays
    int *parent = (int *)malloc(V * sizeof(int));
    int *rank   = (int *)malloc(V * sizeof(int));

    // Initialize Disjoint Set (each vertex is its own parent)
    for (int i = 0; i < V; i++) {
        parent[i] = i;
        rank[i]   = 0;
    }

    printf("\nEdges in the MST:\n");
    int mstWeight = 0;   // To store total weight of MST
    int edgesUsed = 0;   // Count how many edges are added to MST

    // Kruskal's main loop
    for (int i = 0; i < E && edgesUsed < V - 1; i++) {
        int u = edges[i].u;
        int v = edges[i].v;
        int w = edges[i].weight;

        // Check if adding this edge (u, v) will form a cycle
        if (!detectCycle(u, v, parent)) {
            // If it doesn't form a cycle, include it in MST
            unionSet(parent, rank, u, v);
            printf("Edge (%d -- %d) with weight %d\n", u, v, w);
            mstWeight += w;
            edgesUsed++;
        }
        // Otherwise, skip this edge
    }

    printf("\nTotal weight of MST = %d\n", mstWeight);

    // Clean up
    free(parent);
    free(rank);
}

int main() {
    int V, E;
    printf("Enter the number of vertices: ");
    scanf("%d", &V);
    printf("Enter the number of edges: ");
    scanf("%d", &E);

    // Create an array of edges
    Edge *edges = (Edge *)malloc(E * sizeof(Edge));

    printf("Enter each edge in the format (u v weight):\n");
    for (int i = 0; i < E; i++) {
        scanf("%d %d %d", &edges[i].u, &edges[i].v, &edges[i].weight);
        // If your vertices are 1-based, you may consider subtracting 1 here
        // edges[i].u--;
        // edges[i].v--;
    }

    // Run Kruskal's algorithm
    kruskalMST(V, E, edges);

    free(edges);
    return 0;
}
