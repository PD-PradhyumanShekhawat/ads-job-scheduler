#pragma once
#include <string>

enum class JobStatus { WAITING, RUNNING, COMPLETED, CANCELLED };
enum class SchedulingMode { PRIORITY, SHORT_JOBS };

inline std::string statusName(JobStatus s) {
    switch (s) {
        case JobStatus::WAITING: return "WAITING";
        case JobStatus::RUNNING: return "RUNNING";
        case JobStatus::COMPLETED: return "COMPLETED";
        case JobStatus::CANCELLED: return "CANCELLED";
    }
    return "UNKNOWN";
}

struct Job {
    int id{};
    int submissionTime{};
    int priority{};
    int estimatedTime{};
    int delta{};
    int actualTime{};
    int startTime{-1};
    int completionTime{-1};
    JobStatus status{JobStatus::WAITING};
};
