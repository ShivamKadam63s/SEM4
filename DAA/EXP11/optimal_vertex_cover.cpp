#include <iostream>
#include <vector>
#include <limits>
using namespace std;

// Globals to record the best solution found
int        bestSize;
vector<int> bestCover;

// Recursive backtracking: try to cover all edges
//   n         = number of vertices
//   edges     = list of edges (u,v), 0-based
//   incident  = for each vertex, list of incident edge-indices
//   covered   = covered[e] == true if edge e is already covered
//   inCover   = inCover[v] == true if we've chosen v
//   currSize  = number of vertices chosen so far
void backtrack(int n,
               const vector<pair<int,int> >& edges,
               const vector<vector<int> >& incident,
               vector<bool>& covered,
               vector<bool>& inCover,
               int currSize)
{
    // 1) Prune if we already exceed the best solution
    if (currSize >= bestSize) 
        return;

    // 2) Find the first uncovered edge
    int e = -1;
    for (int i = 0; i < (int)edges.size(); i++) {
        if (!covered[i]) { e = i; break; }
    }

    // 3) If no uncovered edge remains, record this cover
    if (e == -1) {
        bestSize = currSize;
        bestCover.clear();
        for (int v = 0; v < n; v++)
            if (inCover[v])
                bestCover.push_back(v);
        return;
    }

    // 4) Otherwise, edge e=(u,v) must be covered by either u or v
    int u = edges[e].first;
    int v = edges[e].second;

    // --- Branch A: include u ---
    inCover[u] = true;
    vector<int> newlyCovered;
    for (int eid : incident[u]) {
        if (!covered[eid]) {
            covered[eid] = true;
            newlyCovered.push_back(eid);
        }
    }
    backtrack(n, edges, incident, covered, inCover, currSize+1);
    // undo
    inCover[u] = false;
    for (int eid : newlyCovered)
        covered[eid] = false;

    // --- Branch B: include v ---
    inCover[v] = true;
    newlyCovered.clear();
    for (int eid : incident[v]) {
        if (!covered[eid]) {
            covered[eid] = true;
            newlyCovered.push_back(eid);
        }
    }
    backtrack(n, edges, incident, covered, inCover, currSize+1);
    // undo
    inCover[v] = false;
    for (int eid : newlyCovered)
        covered[eid] = false;
}

int main() {
    int V, E;
    cout << "Enter number of vertices: ";
    cin >> V;
    cout << "Enter number of edges: ";
    cin >> E;

    vector<pair<int,int> > edges(E);
    cout << "Enter each edge (u v) with 0-based indices:\n";
    for (int i = 0; i < E; i++) {
        cin >> edges[i].first >> edges[i].second;
    }

    // Build incident‐list
    vector<vector<int> > incident(V);
    for (int i = 0; i < E; i++) {
        int u = edges[i].first;
        int v = edges[i].second;
        incident[u].push_back(i);
        incident[v].push_back(i);
    }

    // Initialize bestSize to ∞
    bestSize = V + 1;
    bestCover.clear();
    vector<bool> covered(E, false), inCover(V, false);

    // Launch backtracking
    backtrack(V, edges, incident, covered, inCover, 0);

    // Output
    cout << "Minimum vertex cover size = " << bestSize << "\n";
    cout << "Vertices:";
    for (int v : bestCover) cout << " " << v;
    cout << "\n";
    return 0;
}
