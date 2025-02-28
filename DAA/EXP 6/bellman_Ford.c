#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

typedef struct {
    int u, v, weight;
} Edge;

void bellmanFord(int V, int E, Edge edges[], int source) {
    // Distance array
    int *dist = (int *)malloc(V * sizeof(int));

    //Initialize distances from source to all vertices as infinite and dist[source] = 0
    for (int i = 0; i < V; i++) {
        dist[i] = INT_MAX;
    }
    dist[source] = 0;

    //Relax all edges |V| - 1 times
    //A simple shortest path from source to any other vertex can have at most |V|-1 edges
    for (int i = 0; i < V - 1; i++) {
        for (int j = 0; j < E; j++) {
            int u = edges[j].u;
            int v = edges[j].v;
            int w = edges[j].weight;
            if (dist[u] != INT_MAX && dist[u] + w < dist[v]) {
                dist[v] = dist[u] + w;
            }
        }
    }

    // Check for negative-weight cycles
    // If we can still relax an edge, then we have a negative cycle
    for (int j = 0; j < E; j++) {
        int u = edges[j].u;
        int v = edges[j].v;
        int w = edges[j].weight;
        if (dist[u] != INT_MAX && dist[u] + w < dist[v]) {
            printf("A negative-weight cycle exists in the graph reachable from the source.\n");
            free(dist);
            return;
        }
    }

    //If no negative cycle, print the distances
    printf("Shortest distances from vertex %d:\n", source);
    for (int i = 0; i < V; i++) {
        if (dist[i] == INT_MAX)
            printf("Vertex %d: INF\n", i);
        else
            printf("Vertex %d: %d\n", i, dist[i]);
    }

    free(dist);
}

int main() {
    int V, E;
    int source;   

    printf("Enter the number of vertices: ");
    scanf("%d", &V);
    printf("Enter the number of edges: ");
    scanf("%d", &E);

    // Allocate array for all edges
    Edge *edges = (Edge *)malloc(E * sizeof(Edge));

    printf("Enter each edge in the format: u v weight\n");
    for (int i = 0; i < E; i++) {
        int u, v, w;
        scanf("%d %d %d", &u, &v, &w);
        edges[i].u = u;
        edges[i].v = v;
        edges[i].weight = w;
    }
    printf("Enter the source vertex index: ");
    scanf("%d", &source);
    bellmanFord(V, E, edges, source);
    free(edges);
    return 0;
}
