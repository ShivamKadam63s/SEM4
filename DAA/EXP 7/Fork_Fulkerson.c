#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX 100  // Maximum number of vertices

// Global matrices for capacities, flows, and residual capacities.
int capacity[MAX][MAX];
int flow[MAX][MAX];
int residual[MAX][MAX];

// DFS to find an augmenting path from 's' to 't'.
// It marks visited vertices and fills the parent[] array to record the path.
// Returns 1 if a path is found, 0 otherwise.
int dfs(int s, int t, int n, int visited[MAX], int parent[MAX]) {
    if (s == t)
        return 1;
    visited[s] = 1;
    for (int v = 0; v < n; v++) {
        if (!visited[v] && residual[s][v] > 0) {
            parent[v] = s;
            if (dfs(v, t, n, visited, parent))
                return 1;
        }
    }
    return 0;
}

// This function prints each augmenting path and the corresponding flow updates.
int fordFulkerson(int n, int s, int t) {
    // Initialize flows to 0 and residual capacities equal to the capacities.
    memset(flow, 0, sizeof(flow));
    for (int u = 0; u < n; u++) {
        for (int v = 0; v < n; v++) {
            residual[u][v] = capacity[u][v];
        }
    }
    
    int maxFlow = 0;
    int iteration = 1;
    int parent[MAX];
    
    // While an augmenting path exists in the residual graph:
    while (1) {
        int visited[MAX] = {0};
        memset(parent, -1, sizeof(parent));
        
        if (!dfs(s, t, n, visited, parent))
            break;  // No path found; exit loop.
        
        // Find the bottleneck capacity along the path found.
        int path_flow = 1e9;  // Start with a very large number.
        int v = t;
        while (v != s) {
            int u = parent[v];
            if (residual[u][v] < path_flow)
                path_flow = residual[u][v];
            v = u;
        }
        
        // Print the augmenting path.
        printf("\nIteration %d:\n", iteration++);
        printf("Augmenting path (from sink to source): ");
        v = t;
        while (v != s) {
            printf("%d <- ", v);
            v = parent[v];
        }
        printf("%d\n", s);
        printf("Bottleneck capacity = %d\n", path_flow);
        
        // Update flows and residual capacities along the path.
        v = t;
        printf("Updated flow on edges in the path:\n");
        while (v != s) {
            int u = parent[v];
            // If edge (u,v) is in the original graph then capacity[u][v] > 0.
            if (capacity[u][v] > 0) {
                flow[u][v] += path_flow;
            } else {
                // Otherwise, it is a reverse edge; subtract flow.
                flow[v][u] -= path_flow;
            }
            // Update the residual network.
            residual[u][v] -= path_flow;
            residual[v][u] += path_flow;
            
            printf("  Edge (%d -> %d): flow = %d / capacity = %d\n", 
                   u, v, flow[u][v], capacity[u][v]);
            v = u;
        }
        
        maxFlow += path_flow;
    }
    
    printf("\nNo more augmenting paths. Maximum Flow = %d\n", maxFlow);
    
    // Print final flow for each edge (only those with positive capacity).
    printf("\nFinal flow on all edges (where capacity > 0):\n");
    for (int u = 0; u < n; u++) {
        for (int v = 0; v < n; v++) {
            if (capacity[u][v] > 0) {
                printf("  Edge (%d -> %d): flow = %d / capacity = %d\n",
                       u, v, flow[u][v], capacity[u][v]);
            }
        }
    }
    return maxFlow;
}

int main() {
    int n, m;  
    printf("Enter the number of vertices: ");
    scanf("%d", &n);
    printf("Enter the number of edges: ");
    scanf("%d", &m);
    memset(capacity, 0, sizeof(capacity));
    
    printf("Enter edges in format (u v capacity):\n");
    for (int i = 0; i < m; i++) {
        int u, v, w;
        scanf("%d %d %d", &u, &v, &w);
        capacity[u][v] = w;
    }
    
    int source, sink;
    printf("Enter source: ");
    scanf("%d", &source);
    printf("Enter sink: ");
    scanf("%d", &sink);
    int max_flow = fordFulkerson(n, source, sink);
    printf("\nMax Flow (returned by function) = %d\n", max_flow);
    
    return 0;
}
