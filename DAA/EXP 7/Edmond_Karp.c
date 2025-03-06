#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX 100  // Maximum number of vertices

int bfs(int n, int s, int t, int residual[MAX][MAX], int parent[MAX]) {
    int visited[MAX];
    memset(visited, 0, sizeof(visited));
    int queue[MAX];
    int front = 0, rear = 0;

    // Enqueue the source and mark visited
    queue[rear++] = s;
    visited[s] = 1;
    parent[s] = -1;  // source has no parent

    while (front < rear) {
        int u = queue[front++];
        for (int v = 0; v < n; v++) {
            // If there's available capacity and v is not visited
            if (!visited[v] && residual[u][v] > 0) {
                queue[rear++] = v;
                parent[v] = u;
                visited[v] = 1;
                // If we've reached the sink, we're done
                if (v == t) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

int edmondKarp(int n, int s, int t, int capacity[MAX][MAX]) {
    int flow[MAX][MAX];       // flow[u][v]
    int residual[MAX][MAX];   // residual[u][v]
    int parent[MAX];          // to store path found by BFS
    int maxFlow = 0;

    // Initialize flow and residual
    memset(flow, 0, sizeof(flow));
    for (int u = 0; u < n; u++) {
        for (int v = 0; v < n; v++) {
            residual[u][v] = capacity[u][v]; 
        }
    }

    int iteration = 1;

    // While we can find a path from s to t in the residual graph
    while (bfs(n, s, t, residual, parent)) {
        // Find the bottleneck capacity along this path
        int path_flow = 1e9;  // a large number
        int v = t;
        while (v != s) {
            int u = parent[v];
            if (residual[u][v] < path_flow) {
                path_flow = residual[u][v];
            }
            v = u;
        }

        // Print the augmenting path found
        printf("\nIteration %d:\n", iteration++);
        printf("Augmenting path (sink -> source): ");
        v = t;
        while (v != s) {
            printf("%d <- ", v);
            v = parent[v];
        }
        printf("%d\n", s);
        printf("Bottleneck capacity = %d\n", path_flow);

        // Update flows and residual capacities along this path
        v = t;
        while (v != s) {
            int u = parent[v];
            // If (u->v) is a forward edge in the original graph
            flow[u][v] += path_flow;
            // Subtract path_flow from forward edge capacity
            residual[u][v] -= path_flow;
            // Add path_flow to reverse edge capacity
            residual[v][u] += path_flow;

            v = u;
        }

        maxFlow += path_flow;

        // Print how the flow was augmented on this path
        printf("Updated flow on edges in the path:\n");
        v = t;
        while (v != s) {
            int u = parent[v];
            printf("  Edge (%d -> %d): flow = %d / capacity = %d\n",
                   u, v, flow[u][v], capacity[u][v]);
            v = u;
        }
    }
    printf("\nNo more augmenting paths. Maximum Flow = %d\n", maxFlow);

    // Optionally, print final flow on all edges with capacity > 0
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
    int capacity[MAX][MAX];
    memset(capacity, 0, sizeof(capacity));

    printf("Enter the number of vertices: ");
    scanf("%d", &n);

    printf("Enter the number of edges: ");
    scanf("%d", &m);

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

    int max_flow = edmondKarp(n, source, sink, capacity);
    printf("\nMax Flow (returned by function) = %d\n", max_flow);

    return 0;
}
