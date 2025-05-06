#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <mutex>
#include "tar.pb.h"

class TARAlgorithm {
public:
    TARAlgorithm(const std::string& self_id, const std::vector<std::string>& peers);

    // Main task routing function
    std::vector<std::string> routeTask(const tar::Task& task, const tar::ServerMetrics& requester);

    // Acknowledge replication
    bool acknowledgeTask(const tar::TaskAck& ack);

    // Update peer metrics
    void updateServerMetrics(const tar::ServerMetrics& metrics);

    // Handle work stealing request
    std::optional<tar::Task> requestTaskTransfer(const tar::ServerMetrics& requester);

    // Optional coordinator role (for future expansion)
    bool shouldBecomeCoordinator() const;

    // New method to get task queue length
    int getTaskQueueLength() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return task_queue_.size();
    }

    // Add task to queue
    void addTaskToQueue(const tar::Task& task);

    // New methods for leader election
    std::string electLeader();
    std::string getCurrentLeader() const;

private:
    std::string self_id_;
    std::vector<std::string> peers_;
    std::map<std::string, tar::ServerMetrics> peer_metrics_;
    std::map<std::string, tar::Task> task_queue_;
    mutable std::mutex mutex_;
    std::string current_leader_; // Store the current leader
};
