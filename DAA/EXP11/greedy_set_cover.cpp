#include <iostream>
#include <vector>
#include <limits>
using namespace std;

/*
  WEIGHTED-GREEDY-SET-COVER(U, F, cost)
    Input:  
      U     = {0,1,…,n−1}, the universe of elements
      F     = vector of m subsets; each F[i] is a vector<int> of its elements
      cost  = vector<double> of length m, cost[i] = Cost(F[i])
    Output:
      Cidx  = list of indices i of chosen sets, covering all U

  1. inU[x] ← true for all x ∈ U; uncoveredCount ← n
  2. Cidx ← empty list
  3. while uncoveredCount > 0:
  4.   bestIdx ← −1; bestRatio ← +∞
  5.   for each set i = 0…m−1:
  6.     gain ← number of x ∈ F[i] with inU[x] == true
  7.     if gain > 0:
  8.       ratio ← cost[i] / gain
  9.       if ratio < bestRatio:
 10.         bestRatio ← ratio; bestIdx ← i
 11.  if bestIdx == −1:  // no progress possible
 12.    error: cannot cover all elements
 13.    break
 14.  for each x ∈ F[bestIdx]:
 15.    if inU[x]: inU[x]=false; uncoveredCount--
 16.  append bestIdx to Cidx
 17. return Cidx
*/

vector<int>
weightedGreedySetCover(int n,
                       const vector<vector<int> >& F,
                       const vector<double>& cost)
{
    int m = F.size();
    vector<bool> inU(n, true);
    int uncoveredCount = n;
    vector<int> Cidx;

    while (uncoveredCount > 0) {
        // 4. initialize best
        int   bestIdx   = -1;
        double bestRatio = numeric_limits<double>::infinity();

        // 5–10. find most cost-effective set
        for (int i = 0; i < m; i++) {
            int gain = 0;
            for (int x : F[i]) {
                if (inU[x]) gain++;
            }
            if (gain > 0) {
                double ratio = cost[i] / gain;
                if (ratio < bestRatio) {
                    bestRatio = ratio;
                    bestIdx   = i;
                }
            }
        }

        // 11–13. check coverage possible
        if (bestIdx == -1) {
            cerr << "Error: Unable to cover all elements.\n";
            break;
        }

        // 14–15. remove covered elements
        for (int x : F[bestIdx]) {
            if (inU[x]) {
                inU[x] = false;
                uncoveredCount--;
            }
        }

        // 16. record chosen set
        Cidx.push_back(bestIdx);
    }

    return Cidx;
}

int main() {

    int n, m;
    cout << "Enter |U| (number of elements) and |F| (number of sets): ";
    cin >> n >> m;

    vector<vector<int> > F(m);
    vector<double> cost(m);

    cout << "For each of the " << m << " sets, enter its cost then its size k followed by k elements (0-based):\n";
    for (int i = 0; i < m; i++) {
        int k;
        cin >> cost[i] >> k;
        F[i].resize(k);
        for (int j = 0; j < k; j++) {
            cin >> F[i][j];
        }
    }

    vector<int> cover = weightedGreedySetCover(n, F, cost);

    double totalCost = 0;
    cout << "\nGreedy cover uses " << cover.size() << " sets:\n";
    for (int idx : cover) {
        totalCost += cost[idx];
        cout << "  Set " << idx << " (cost=" << cost[idx] << ")\n";
    }
    cout << "Total cost = " << totalCost << "\n";
    return 0;
}
