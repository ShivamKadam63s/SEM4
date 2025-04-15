#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <string>
using namespace std;

// Function for FCFS (First Come First Served) scheduling algorithm.
void fcfs(const vector<int>& requests, int start) {
    cout << "\n--- FCFS Scheduling ---\n";
    int current = start;
    int totalMovement = 0;
    
    cout << "Path: " << current;
    for (int req : requests) {
        totalMovement += abs(req - current);
        cout << " -> " << req;
        current = req;
    }
    cout << "\nTotal head movement: " << totalMovement << "\n";
}

// Function for SSTF (Shortest Seek Time First) scheduling algorithm.
void sstf(vector<int> requests, int start, const string& direction) {
    cout << "\n--- SSTF Scheduling (with direction tie-break: " << direction << ") ---\n";
    int current = start;
    int totalMovement = 0;
    cout << "Path: " << current;

    while (!requests.empty()) {
        int idx = 0;
        int minDistance = abs(requests[0] - current);
        
        for (size_t i = 1; i < requests.size(); i++) {
            int dist = abs(requests[i] - current);
            if (dist < minDistance) {
                minDistance = dist;
                idx = i;
            } else if (dist == minDistance) {
                // Tie-breaking logic based on direction
                if (direction == "right" && requests[i] > current && requests[idx] < current) {
                    idx = i;
                } else if (direction == "left" && requests[i] < current && requests[idx] > current) {
                    idx = i;
                }
            }
        }

        totalMovement += abs(current - requests[idx]);
        current = requests[idx];
        cout << " -> " << current;
        requests.erase(requests.begin() + idx);
    }
    
    cout << "\nTotal head movement: " << totalMovement << "\n";
}


// Function for SCAN (Elevator) scheduling algorithm.
// It simulates the head moving to one end then reversing direction.
// For left direction, the head goes to track 0. For right direction, it goes to maxTrack.
void scan(vector<int> requests, int start, int maxTrack, const string &direction) {
    cout << "\n--- SCAN Scheduling (Elevator) ---\n";
    
    int totalMovement = 0;
    int current = start;
    vector<int> left, right;
    
    // Divide requests into those to the left and right of the current head position.
    for (int req : requests) {
        if (req < start)
            left.push_back(req);
        else
            right.push_back(req);
    }
    sort(left.begin(), left.end());
    sort(right.begin(), right.end());

    cout << "Path: " << current;
    
    if(direction == "left"){
        // First service left side in descending order.
        reverse(left.begin(), left.end());
        for (int req : left) {
            totalMovement += abs(current - req);
            current = req;
            cout << " -> " << current;
        }
        // Go to beginning of disk even if no request is at track 0.
        if(current != 0){
            totalMovement += abs(current - 0);
            current = 0;
            cout << " -> " << current;
        }
        // Now service the right side in ascending order.
        for (int req : right) {
            totalMovement += abs(current - req);
            current = req;
            cout << " -> " << current;
        }
    }
    else if(direction == "right"){
        // First service the right side in ascending order.
        for (int req : right) {
            totalMovement += abs(current - req);
            current = req;
            cout << " -> " << current;
        }
        // Go to maximum track of the disk if not already there.
        if(current != maxTrack){
            totalMovement += abs(current - maxTrack);
            current = maxTrack;
            cout << " -> " << current;
        }
        // Then service the left side in descending order.
        reverse(left.begin(), left.end());
        for (int req : left) {
            totalMovement += abs(current - req);
            current = req;
            cout << " -> " << current;
        }
    }
    else{
        cout << "\nInvalid direction. Use \"left\" or \"right\".\n";
        return;
    }
    
    cout << "\nTotal head movement: " << totalMovement << "\n";
}

// Function for C-SCAN (Circular SCAN) scheduling algorithm.
// The head services requests in one direction only. When it reaches the end (maxTrack for right direction),
// it jumps to the beginning without adding that jump cost.
void cscan(vector<int> requests, int start, int maxTrack) {
    cout << "\n--- C-SCAN Scheduling ---\n";
    int totalMovement = 0;
    int current = start;
    vector<int> left, right;
    
    // Divide requests into those less than and greater than or equal to start.
    for (int req : requests) {
        if (req < start)
            left.push_back(req);
        else
            right.push_back(req);
    }
    sort(left.begin(), left.end());
    sort(right.begin(), right.end());
    
    cout << "Path: " << current;
    // Service the right side in ascending order.
    for (int req : right) {
        totalMovement += abs(current - req);
        current = req;
        cout << " -> " << current;
    }
    
    // If there are any requests on the left side, jump from the last request on right side to the first request of left side.
    if(!left.empty()){
        // Go to end of disk (if current is not already at maxTrack) and then jump to beginning.
        if(current != maxTrack){
            totalMovement += abs(current - maxTrack);
            current = maxTrack;
            cout << " -> " << current;
        }
        // Jump from maxTrack to 0 is not added to cost.
        current = 0;
        cout << " -> " << current;
        // Service the left side in ascending order.
        for (int req : left) {
            totalMovement += abs(current - req);
            current = req;
            cout << " -> " << current;
        }
    }
    cout << "\nTotal head movement: " << totalMovement << "\n";
}

// Function for LOOK scheduling algorithm.
// The head only goes as far as the furthest request in each direction before reversing.
void look(vector<int> requests, int start, const string &direction) {
    cout << "\n--- LOOK Scheduling ---\n";
    int totalMovement = 0;
    int current = start;
    vector<int> left, right;
    
    for (int req : requests) {
        if (req < start)
            left.push_back(req);
        else
            right.push_back(req);
    }
    sort(left.begin(), left.end());
    sort(right.begin(), right.end());
    
    cout << "Path: " << current;
    
    if(direction == "left"){
        // Service left requests in descending order.
        reverse(left.begin(), left.end());
        for (int req : left) {
            totalMovement += abs(current - req);
            current = req;
            cout << " -> " << current;
        }
        // Then service right requests in ascending order.
        for (int req : right) {
            totalMovement += abs(current - req);
            current = req;
            cout << " -> " << current;
        }
    }
    else if(direction == "right"){
        // Service right requests in ascending order.
        for (int req : right) {
            totalMovement += abs(current - req);
            current = req;
            cout << " -> " << current;
        }
        // Then service left requests in descending order.
        reverse(left.begin(), left.end());
        for (int req : left) {
            totalMovement += abs(current - req);
            current = req;
            cout << " -> " << current;
        }
    }
    else {
        cout << "\nInvalid direction. Use \"left\" or \"right\".\n";
        return;
    }
    cout << "\nTotal head movement: " << totalMovement << "\n";
}

// Function for C-LOOK scheduling algorithm.
// The head services in one direction up to the furthest request and then jumps to the opposite end (jump cost not added) and services the remaining requests.
void clook(vector<int> requests, int start) {
    cout << "\n--- C-LOOK Scheduling ---\n";
    int totalMovement = 0;
    int current = start;
    vector<int> left, right;
    
    for (int req : requests) {
        if(req < start)
            left.push_back(req);
        else
            right.push_back(req);
    }
    sort(left.begin(), left.end());
    sort(right.begin(), right.end());
    
    cout << "Path: " << current;
    // Service the right side (requests greater than or equal to the head) in ascending order.
    for (int req : right) {
        totalMovement += abs(current - req);
        current = req;
        cout << " -> " << current;
    }
    
    // If there are requests on the left side, jump from the last right to the first left.
    if(!left.empty()){
        // The jump is not counted.
        current = left.front();
        cout << " -> " << current;
        // Then service the remaining left requests in ascending order.
        for (size_t i = 1; i < left.size(); i++) {
            totalMovement += abs(current - left[i]);
            current = left[i];
            cout << " -> " << current;
        }
    }
    cout << "\nTotal head movement: " << totalMovement << "\n";
}

int main(){
    int numReq;
    cout << "Enter number of disk requests: ";
    cin >> numReq;
    
    vector<int> requests(numReq);
    cout << "Enter the disk requests (track numbers): ";
    for (int i = 0; i < numReq; i++) {
        cin >> requests[i];
    }
    
    int head;
    cout << "Enter initial head position: ";
    cin >> head;
    
    int maxTrack = 0;
    cout << "Enter maximum track number (e.g., 199): ";
    cin >> maxTrack;
    
    string direction;
    cout << "Enter direction(left/right): ";
    cin >> direction;
    
    fcfs(requests, head);
    sstf(requests, head, direction);
    scan(requests, head, maxTrack, direction);
    cscan(requests, head, maxTrack);
    look(requests, head, direction);
    clook(requests, head);
    
    return 0;
}
