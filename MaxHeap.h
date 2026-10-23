#pragma once
#include "Job.h"
#include <algorithm>
#include <memory>
#include <ostream>
#include <vector>
using namespace std;

// Binary max heap implemented from scratch. Higher scheduling rank is closer to the root.
class WaitingMaxHeap {
private:
    vector<shared_ptr<Job>> data;
    SchedulingMode mode{SchedulingMode::PRIORITY};
    size_t maxSize{0};
    long long insertions{0}, extractions{0}, removals{0}, priorityUpdates{0}, rebuilds{0};

    bool higherRank(const shared_ptr<Job>& a, const shared_ptr<Job>& b) const {
        if (mode == SchedulingMode::PRIORITY) {
            if (a->priority != b->priority) return a->priority > b->priority;
        } else {
            if (a->estimatedTime != b->estimatedTime) return a->estimatedTime < b->estimatedTime;
            if (a->priority != b->priority) return a->priority > b->priority;
        }
        if (a->submissionTime != b->submissionTime) return a->submissionTime < b->submissionTime;
        return a->id < b->id;
    }
    void siftUp(size_t i) {
        while (i > 0) {
            size_t p = (i - 1) / 2;
            if (!higherRank(data[i], data[p])) break;
            swap(data[i], data[p]); i = p;
        }
    }
    void siftDown(size_t i) {
        while (true) {
            size_t best = i, l = 2 * i + 1, r = 2 * i + 2;
            if (l < data.size() && higherRank(data[l], data[best])) best = l;
            if (r < data.size() && higherRank(data[r], data[best])) best = r;
            if (best == i) break;
            swap(data[i], data[best]); i = best;
        }
    }
    void rebuildHeap() {
        if (data.size() > 1) for (int i = static_cast<int>(data.size() / 2) - 1; i >= 0; --i) siftDown(static_cast<size_t>(i));
    }
public:
    size_t size() const { return data.size(); }
    bool empty() const { return data.empty(); }
    bool contains(int id) const {
        return any_of(data.begin(), data.end(), [id](const auto& j) { return j->id == id; });
    }
    void insert(const shared_ptr<Job>& job) {
        data.push_back(job); ++insertions; maxSize = max(maxSize, data.size()); siftUp(data.size() - 1);
    }
    shared_ptr<Job> extractMax() {
        if (data.empty()) return nullptr;
        auto result = data.front();
        if (data.size() == 1) data.pop_back();
        else { data.front() = data.back(); data.pop_back(); siftDown(0); }
        ++extractions;
        return result;
    }
    bool remove(int id) {
        for (size_t i = 0; i < data.size(); ++i) {
            if (data[i]->id == id) {
                if (i + 1 == data.size()) data.pop_back();
                else {
                    data[i] = data.back(); data.pop_back();
                    if (i > 0 && higherRank(data[i], data[(i - 1) / 2])) siftUp(i); else siftDown(i);
                }
                ++removals; return true;
            }
        }
        return false;
    }
    void priorityUpdated(int id) {
        for (size_t i = 0; i < data.size(); ++i) if (data[i]->id == id) {
            ++priorityUpdates;
            if (i > 0 && higherRank(data[i], data[(i - 1) / 2])) siftUp(i); else siftDown(i);
            return;
        }
    }
    void switchToShortJobs() {
        if (mode == SchedulingMode::SHORT_JOBS) return;
        mode = SchedulingMode::SHORT_JOBS;
        rebuildHeap(); ++rebuilds;
    }
    vector<shared_ptr<Job>> entries() const { return data; }
    void print(ostream& out) const {
        out << "MAX_HEAP: [";
        for (size_t i = 0; i < data.size(); ++i) {
            if (i) out << ", ";
            out << '(' << data[i]->id << ',' << data[i]->priority << ',' << data[i]->estimatedTime << ')';
        }
        out << "]\n";
    }
    void printStats(ostream& out) const {
        out << "HEAP_STATS\nsize=" << data.size() << "\nmax_size=" << maxSize
            << "\ninsertions=" << insertions << "\nextractions=" << extractions
            << "\nremovals=" << removals << "\npriority_updates=" << priorityUpdates
            << "\nrebuilds=" << rebuilds << "\nEND_HEAP_STATS\n";
    }
};

