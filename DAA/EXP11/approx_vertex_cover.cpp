#include <bits/stdc++.h>
using namespace std;

// APPROX‑VERTEX‑COVER(G)
// 1. C = ∅
// 2. E′ = G.E
// 3. while E′ ≠ ∅
// 4.    let (u,v) be an arbitrary edge of E′
// 5.    C = C ∪ {u,v}
// 6.    remove from E′ every edge incident on either u or v
// 7. return C

vector<int> approxVertexCover(int V, const vector<pair<int,int>>& G_E) {
    // (1) Initialize cover C ← empty
    vector<bool> inCover(V, false);
    
    // (2) Make a mutable copy of the edge‑set
    int E = G_E.size();
    vector<bool> active(E, true);        // active[i] == “edge i ∈ E′”
    
    // Build “incident edges” lists so we can remove in O(deg) time
    vector<vector<int>> incident(V);
    for (int i = 0; i < E; i++) {
        int u = G_E[i].first;
        int v = G_E[i].second;
        incident[u].push_back(i);
        incident[v].push_back(i);
    }
    
    
    // We’ll pick “arbitrary” edges by scanning from 0…E–1
    int nextEdge = 0;
    
    // (3) while E′ ≠ ∅
    while (true) {
        // find an active edge
        while (nextEdge < E && !active[nextEdge])
            nextEdge++;
        if (nextEdge == E) break;       // no active edges remain
        
        // (4) let (u,v) be an arbitrary edge of E′
        int eid = nextEdge;
        int u   = G_E[eid].first;
        int v   = G_E[eid].second;
        
        // (5) C = C ∪ {u,v}
        inCover[u] = inCover[v] = true;
        
        // (6) remove from E′ every edge incident on u or on v
        for (int j : incident[u]) active[j] = false;
        for (int j : incident[v]) active[j] = false;
    }
    
    // (7) return C as a list of vertices
    vector<int> C;
    for (int i = 0; i < V; i++)
        if (inCover[i])
            C.push_back(i);
    return C;
}

int main() {
    
    int V, E;
    cout << "Enter the number of vertices: ";
    cin >> V;
    cout << "Enter the number of edges: ";
    cin >> E;
    vector<pair<int,int>> edges(E);
    cout << "Enter each edge (u v):\n";
    for (int i = 0; i < E; i++) {
        cin >> edges[i].first >> edges[i].second;
    }
    
    auto C = approxVertexCover(V, edges);
    cout << "Cover size = " << C.size() << "\nVertices:";
    for (int u : C) cout << " " << u;
    cout << "\n";
    return 0;
}
