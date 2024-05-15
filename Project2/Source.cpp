#include <iostream>
#include <fstream>

using namespace std;

struct Process {
    int id;
    int arrivalTime;
    int burstTime;
    int startTime;
    int finishTime;
    int waitingTime;
    int turnaroundTime;
    int remainingTime;
    bool started;
    bool completed;
};


const int MAX_PROCESS_COUNT = 100;  // the maximum number of processes
Process processes[MAX_PROCESS_COUNT];
int processCount = 0;

int contextSwitchTime;
int quantum;

int currentTime;
double totalWaitingTime;
double totalTurnaroundTime;


// This function is very important to make sure no exceptions will appear
void resetAllValues() {
    for (int i = 0; i < processCount; i++) {
        processes[i].startTime = -1;
        processes[i].finishTime = 0;
        processes[i].waitingTime = 0;
        processes[i].turnaroundTime = 0;
        processes[i].remainingTime = processes[i].burstTime;
        processes[i].started = false;
        processes[i].completed = false;
    }

    currentTime = 0;
    totalWaitingTime = 0;
    totalTurnaroundTime = 0;
}


// To print all the values in the same way without repeating the code
void printResults() {

    cout << "Process ID\tFinish Time\tWaiting Time\tTurnaround Time\n";
    for (int i = 0; i < processCount; i++) {
        cout << "P" << processes[i].id << "\t\t" << processes[i].finishTime << "\t\t" << processes[i].waitingTime << "\t\t" << processes[i].turnaroundTime << "\n";
    }

    // Calculate and display average waiting and turnaround time
    double avgWaitingTime = totalWaitingTime / processCount;
    double avgTurnaroundTime = totalTurnaroundTime / processCount;
    cout << "\nAverage Waiting Time: " << avgWaitingTime << "\n";
    cout << "Average Turnaround Time: " << avgTurnaroundTime << "\n";

    // Calculate CPU utilization
    int totalTime = currentTime - contextSwitchTime;
    double cpuUtilization = 100.0 * (totalTime - contextSwitchTime * (processCount - 1)) / totalTime;
    cout << "CPU Utilization: " << cpuUtilization << "%\n------------------------------------\n\n";
}




void FCFS() {

    resetAllValues();

    cout << "\nGantt Chart:\n";
    for (int i = 0; i < processCount; ++i) {
        if (currentTime < processes[i].arrivalTime) {
            currentTime = processes[i].arrivalTime; // Jump in time to when the process arrives if CPU is idle
        }

        processes[i].startTime = currentTime;
        processes[i].finishTime = processes[i].startTime + processes[i].burstTime;
        processes[i].turnaroundTime = processes[i].finishTime - processes[i].arrivalTime;
        processes[i].waitingTime = processes[i].startTime - processes[i].arrivalTime;
        totalWaitingTime += processes[i].waitingTime;
        totalTurnaroundTime += processes[i].turnaroundTime;

        // Output the Gantt chart as required: process repeated as many times as its burst time
        for (int j = 0; j < processes[i].burstTime; j++) {
            cout << "P" << processes[i].id << " ";
        }
        // Context switch only if it's not the last process
        if (i < processCount - 1) {
            cout << "| CS | ";
        }

        // Increment the current time by the burst time and context switch time
        currentTime += processes[i].burstTime;
        if (i < processCount - 1) {
            currentTime += contextSwitchTime; // Add context switch time only between processes
        }
    }
    cout << "\n";

    printResults();
}


void SRT() {

    resetAllValues();    

    int lastProcess = -1;
    cout << "\nGantt Chart:\n";
    while (true) {
        int idx = -1;
        int minTime = INT_MAX;

        // Find the process with the smallest remaining time
        for (int i = 0; i < processCount; i++) {
            if (processes[i].arrivalTime <= currentTime && !processes[i].completed && processes[i].remainingTime < minTime) {
                minTime = processes[i].remainingTime;
                idx = i;
            }
        }

        if (idx == -1) break; // All processes are completed

        // Handle process execution
        if (lastProcess != idx && lastProcess != -1) {
            cout << "| CS | ";
            currentTime += contextSwitchTime;
        }

        processes[idx].remainingTime--;
        currentTime++;
        if (!processes[idx].started) {
            processes[idx].startTime = currentTime;
            processes[idx].started = true;
        }

        cout << "P" << processes[idx].id << " ";

        if (processes[idx].remainingTime == 0) {
            processes[idx].finishTime = currentTime;
            processes[idx].turnaroundTime = currentTime - processes[idx].arrivalTime;
            processes[idx].waitingTime = processes[idx].turnaroundTime - processes[idx].burstTime;
            totalWaitingTime += processes[idx].waitingTime;
            totalTurnaroundTime += processes[idx].turnaroundTime;
            processes[idx].completed = true;
        }

        lastProcess = idx;
    }
    cout << "\n";

    printResults();

}


void RR() {

    resetAllValues();

    int lastIndex = 0, activeProcesses = 0;
    cout << "\nGantt Chart:\n";

    while (activeProcesses < processCount) {
        bool allWait = true;

        for (int i = 0; i < processCount; i++) {
            int idx = (lastIndex + i) % processCount;  // Wrap around the index
            if (!processes[idx].completed && processes[idx].arrivalTime <= currentTime) {

                if (lastIndex != idx || (!processes[i].completed && processes[i].started)) {
                    cout << "| CS | ";
                    currentTime += contextSwitchTime;
                }

                int runtime = min(quantum, processes[idx].remainingTime);
                for (int q = 0; q < runtime; q++) {
                    cout << "P" << processes[idx].id << " ";
                }
                processes[idx].remainingTime -= runtime;
                currentTime += runtime;

                if (!processes[idx].started) {
                    processes[idx].startTime = currentTime - runtime;
                    processes[idx].started = true;
                }

                if (processes[idx].remainingTime <= 0) {
                    processes[idx].finishTime = currentTime;
                    processes[idx].turnaroundTime = processes[idx].finishTime - processes[idx].arrivalTime;
                    processes[idx].waitingTime = processes[idx].turnaroundTime - processes[idx].burstTime;
                    totalWaitingTime += processes[idx].waitingTime;
                    totalTurnaroundTime += processes[idx].turnaroundTime;
                    processes[idx].completed = true;
                    activeProcesses++;
                }
                lastIndex = (idx + 1) % processCount;  // Prepare next index
                allWait = false;
                break; // Found a process to execute, break to restart search
            }
        }

        if (allWait) { // If no process was ready, increment time
            currentTime++;
        }
    }
    cout << "\n";

    printResults();
}



int main() {

    ifstream fin("input.txt");
    if (!fin.is_open()) {
        cerr << "Failed to open file.\n";
        return 1;
    }

    // Reading context switch time & quantum at the beginning
    fin >> contextSwitchTime >> quantum;

    // Reading processes data
    while (fin >> processes[processCount].id >> processes[processCount].arrivalTime >> processes[processCount].burstTime) {
        processCount++;

        if (processCount >= MAX_PROCESS_COUNT)
            break;  // Prevent exceeding array bounds
    }
    fin.close();



    // Sort processes by arrival time using insertion sort
    for (int i = 1; i < processCount; ++i) {
        Process key = processes[i];
        int j = i - 1;


        while (j >= 0 && processes[j].arrivalTime > key.arrivalTime) {
            processes[j + 1] = processes[j];
            j = j - 1;
        }
        processes[j + 1] = key;
    }


    cout << "1- First-Come First-Served (FCFS) \n2- Shortest Remaining Time (SRT) \n3- Round-Robin (RR) \n4- exit";

    while (1) {
        cout << "\nEnter your choice : ";
        int choice;
        cin >> choice;

        switch (choice) {
        case 1: FCFS(); break;
        case 2: SRT(); break;
        case 3: RR(); break;
        case 4: return 0;
        default: cout << "\ninvalid choice try again";
        }
    }
}