#include "Job.h"
#include "AVLTree.h"
#include "MaxHeap.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <queue>
#include <sstream>
#include <string>
#include <vector>
using namespace std;

struct RunningEntry {
    int completionTime;
    int jobId;
    shared_ptr<Job> job;
};
struct RunningLater {
    bool operator()(const RunningEntry& a, const RunningEntry& b) const {
        if (a.completionTime != b.completionTime) return a.completionTime > b.completionTime;
        return a.jobId > b.jobId;
    }
};

class Scheduler {
private:
    int capacity{1};
    int simulationTime{0};
    AVLTree tree;
    WaitingMaxHeap waiting;
    priority_queue<RunningEntry, vector<RunningEntry>, RunningLater> running;
    vector<shared_ptr<Job>> cancelled;
    vector<shared_ptr<Job>> executionOrder;

    shared_ptr<Job> cancelledJob(int id) const {
        for (const auto& j : cancelled) if (j->id == id) return j;
        return nullptr;
    }
    void completeThrough(int time) {
        while (!running.empty() && running.top().completionTime <= time) {
            int finishedAt = running.top().completionTime;
            simulationTime = max(simulationTime, finishedAt);
            while (!running.empty() && running.top().completionTime == finishedAt) {
                auto entry = running.top(); running.pop();
                entry.job->status = JobStatus::COMPLETED;
            }
        }
        simulationTime = max(simulationTime, time);
    }
    int runningCount() const { return static_cast<int>(running.size()); }
    void printJobLine(ostream& out, const shared_ptr<Job>& j) const {
        out << "Job " << j->id << ": submitted=" << j->submissionTime
            << ", started=" << (j->startTime < 0 ? "null" : to_string(j->startTime))
            << ", actual_time=" << (j->startTime < 0 ? "null" : to_string(j->actualTime))
            << ", completed=" << (j->completionTime < 0 ? "null" : to_string(j->completionTime))
            << ", status=" << statusName(j->status) << '\n';
    }
    void printStatusTable(ostream& out) const {
        out << "JOB STATUS\n";
        out << left << setw(10) << "Job" << setw(17) << "Submitted" << setw(11) << "Started"
            << setw(15) << "Actual Time" << setw(15) << "Completed" << "Status\n";
        for (const auto& j : tree.inorderJobs()) {
            out << left << setw(10) << j->id << setw(17) << j->submissionTime
                << setw(11) << (j->startTime < 0 ? "null" : to_string(j->startTime))
                << setw(15) << (j->startTime < 0 ? "null" : to_string(j->actualTime))
                << setw(15) << (j->completionTime < 0 ? "null" : to_string(j->completionTime))
                << statusName(j->status) << '\n';
        }
        for (const auto& j : cancelled) {
            out << left << setw(10) << j->id << setw(17) << j->submissionTime
                << setw(11) << (j->startTime < 0 ? "null" : to_string(j->startTime))
                << setw(15) << (j->startTime < 0 ? "null" : to_string(j->actualTime))
                << setw(15) << (j->completionTime < 0 ? "null" : to_string(j->completionTime))
                << statusName(j->status) << '\n';
        }
        out << "END JOB STATUS\n";
    }
public:
    explicit Scheduler(int k) : capacity(k) {}
    void command(int timestamp, const string& op, istringstream& args, ostream& out) {
        completeThrough(timestamp);
        if (op == "SUBMIT") {
            int id, priority, estimated, delta;
            if (!(args >> id >> priority >> estimated >> delta)) return;
            if (tree.contains(id) || cancelledJob(id)) { out << "ERROR: Job " << id << " already exists\n"; return; }
            auto job = make_shared<Job>();
            job->id = id; job->submissionTime = timestamp; job->priority = priority;
            job->estimatedTime = estimated; job->delta = delta; job->actualTime = estimated + delta;
            tree.insert(job); waiting.insert(job);
            out << "Job " << id << " submitted at " << timestamp << " ms\n";
        } else if (op == "PROCESS_NEXT") {
            if (waiting.empty()) { out << "NO_WAITING_JOBS\n"; return; }
            if (runningCount() >= capacity) {
                if (!running.empty()) completeThrough(running.top().completionTime);
            }
            auto job = waiting.extractMax();
            if (!job) { out << "NO_WAITING_JOBS\n"; return; }
            job->startTime = simulationTime;
            job->completionTime = simulationTime + job->actualTime;
            job->status = JobStatus::RUNNING;
            running.push({job->completionTime, job->id, job});
            executionOrder.push_back(job);
            out << "Job " << job->id << " started at " << job->startTime << " ms\n";
        } else if (op == "FIND") {
            int id; if (!(args >> id)) return;
            if (auto job = tree.findShared(id)) {
                printJobLine(out, job);
            } else if (auto job = cancelledJob(id)) printJobLine(out, job);
            else out << "ERROR: Job " << id << " not found\n";
        } else if (op == "JOB_STATUS") {
            printStatusTable(out);
        } else if (op == "CANCEL") {
            int id; if (!(args >> id)) return;
            Job* found = tree.find(id);
            if (!found) {
                if (cancelledJob(id)) out << "ERROR: Job " << id << " cannot be cancelled while status is CANCELLED\n";
                else out << "ERROR: Job " << id << " not found\n";
                return;
            }
            if (found->status != JobStatus::WAITING) {
                out << "ERROR: Job " << id << " cannot be cancelled while status is " << statusName(found->status) << '\n';
                return;
            }
            auto ref = tree.findShared(id);
            waiting.remove(id); tree.erase(id); ref->status = JobStatus::CANCELLED; cancelled.push_back(ref);
            out << "Job " << id << " cancelled\n";
        } else if (op == "UPDATE_PRIORITY") {
            int id, newPriority; if (!(args >> id >> newPriority)) return;
            Job* found = tree.find(id);
            if (!found) {
                out << "ERROR: Job " << id << " not found\n";
                return;
            }
            if (found->status != JobStatus::WAITING) {
                out << "ERROR: Job " << id << " priority cannot be updated while status is " << statusName(found->status) << '\n';
                return;
            }
            found->priority = newPriority; waiting.priorityUpdated(id);
            out << "Job " << id << " priority updated to " << newPriority << '\n';
        } else if (op == "SHORT_JOBS") {
            waiting.switchToShortJobs();
            out << "Scheduling mode changed to SHORT_JOBS\n";
        }
    }
    void printFinal(ostream& out) const {
        out << "EXECUTION_ORDER\n";
        for (const auto& j : executionOrder) out << j->id << ' ' << j->startTime << ' ' << j->completionTime << '\n';
        out << "END_EXECUTION_ORDER\n";
        tree.printStructure(out); tree.printInorder(out); waiting.print(out); tree.printStats(out); waiting.printStats(out);
    }
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: ./jobScheduler input_file\n";
        return 1;
    }
    string inputPath = argv[1];
    ifstream input(inputPath);
    if (!input) { cerr << "Error: cannot open input file\n"; return 1; }
    string line;
    int capacity = -1;
    vector<string> commands;
    while (getline(input, line)) {
        if (line.find_first_not_of(" \t\r\n") == string::npos) continue;
        istringstream check(line);
        if (capacity < 0) {
            string keyword; check >> keyword >> capacity;
            if (keyword != "CAPACITY" || capacity <= 0) { cerr << "Error: first non-empty line must be CAPACITY <positive integer>\n"; return 1; }
        } else commands.push_back(line);
    }
    if (capacity < 0) { cerr << "Error: missing CAPACITY line\n"; return 1; }
    string outputPath = inputPath;
    size_t slash = outputPath.find_last_of('/');
    size_t dot = outputPath.find_last_of('.');
    if (dot == string::npos || (slash != string::npos && dot < slash)) dot = outputPath.size();
    outputPath = outputPath.substr(0, dot) + "_output_file.txt";
    ofstream output(outputPath);
    if (!output) { cerr << "Error: cannot create output file\n"; return 1; }
    Scheduler scheduler(capacity);
    for (const auto& cmdLine : commands) {
        istringstream row(cmdLine);
        int timestamp; string op;
        if (!(row >> timestamp >> op)) continue;
        scheduler.command(timestamp, op, row, output);
    }
    scheduler.printFinal(output);
    return 0;
}
