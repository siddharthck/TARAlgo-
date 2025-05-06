#include "TARAlgorithm.hpp"
#include <algorithm>
#include <iostream>
#include <chrono>

TARAlgorithm::TARAlgorithm(const std::string& self_id, const std::vector<std::string>& peers)
    : self_id_(self_id), peers_(peers) {
    std::cout << "TARAlgorithm created with server_id: " << self_id_ << std::endl;
    std::cout << "Initial peers: ";
    for (const auto& peer : peers_) std::cout << peer << " ";
    std::cout << std::endl;
}

std::vector<std::string> TARAlgorithm::routeTask(const tar::Task& task, const tar::ServerMetrics& requester) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::pair<std::string, float>> scored_peers;

    for (const auto& [peer_id, metrics] : peer_metrics_) {
        float score = 1.5f / (metrics.queue_length() + 1)
                    + 2.0f / (metrics.cpu_utilization() + 1)
                    + 1.0f / ((std::chrono::system_clock::now().time_since_epoch().count() - metrics.last_heartbeat()) + 1)
                    - metrics.network_latency(); // Lower latency is better
        scored_peers.emplace_back(peer_id, score);
    }

    // Fallback if metrics are not yet available
    if (scored_peers.empty()) {
        return {peers_.begin(), peers_.begin() + std::min<size_t>(peers_.size(), 2)};
    }

    std::sort(scored_peers.begin(), scored_peers.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    int replication_factor = (task.priority() == tar::Priority::URGENT) ? 3 :
                             (task.priority() == tar::Priority::MODERATE) ? 2 : 1;

    std::vector<std::string> selected;
    for (int i = 0; i < std::min(replication_factor, (int)scored_peers.size()); ++i) {
        selected.push_back(scored_peers[i].first);
    }

    task_queue_[task.id()] = task; // store locally
    return selected;
}

bool TARAlgorithm::acknowledgeTask(const tar::TaskAck& ack) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::cout << "[ACK] Task: " << ack.task_id() << " acknowledged by server: " << ack.server_id() << std::endl;

    // Updated log to reflect the lack of full commitment tracking
    std::cout << "[INFO] Task " << ack.task_id() << " acknowledgment received. Commitment tracking is not implemented yet." << std::endl;

    return true;  // Extend for quorum logic later
}

void TARAlgorithm::updateServerMetrics(const tar::ServerMetrics& metrics) {
    std::lock_guard<std::mutex> lock(mutex_);
    peer_metrics_[metrics.server_id()] = metrics;
    std::cout << "[HEARTBEAT] Updated metrics from " << metrics.server_id() << std::endl;
}

std::optional<tar::Task> TARAlgorithm::requestTaskTransfer(const tar::ServerMetrics& requester) {
    int max_hop_count = GetEnvOrDefault("MAX_HOP_COUNT", 2);

    std::lock_guard<std::mutex> lock(mutex_);

    for (auto it = task_queue_.begin(); it != task_queue_.end(); ++it) {
        tar::Task& task = it->second;

        // Check if the task has reached the maximum hop count
        if (task.hop_count() >= max_hop_count) {
            std::cout << "[Task Transfer] Task " << task.id() << " has reached max hop count. Skipping." << std::endl;
            continue; // Skip this task and check the next one
        }

        // Increment the hop count and transfer the task
        task.set_hop_count(task.hop_count() + 1);
        tar::Task transferred_task = task;
        task_queue_.erase(it); // Remove the task from the queue
        return transferred_task;
    }

    return std::nullopt; // No transferable task found
}

bool TARAlgorithm::shouldBecomeCoordinator() const {
    return true; // Always true for now
}

void TARAlgorithm::addTaskToQueue(const tar::Task& task) {
    std::lock_guard<std::mutex> lock(mutex_);
    task_queue_[task.id()] = task;
    std::cout << "[Task Queue] Added task: " << task.id() << std::endl;
}

std::string TARAlgorithm::electLeader() {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::pair<std::string, float>> scored_peers;

    // Calculate scores for all peers, including self
    for (const auto& [peer_id, metrics] : peer_metrics_) {
        float score = 1.5f / (metrics.queue_length() + 1)
                    + 2.0f / (metrics.cpu_utilization() + 1)
                    + 1.0f / ((std::chrono::system_clock::now().time_since_epoch().count() - metrics.last_heartbeat()) + 1)
                    - metrics.network_latency(); // Lower latency is better
        scored_peers.emplace_back(peer_id, score);
    }

    // Add self to the scoring
    float self_score = 1.5f / (task_queue_.size() + 1)
                     + 2.0f / (0.5 + 1) // Example CPU utilization for self
                     + 1.0f / 1; // Assume self is always responsive
    scored_peers.emplace_back(self_id_, self_score);

    // Sort peers by score in descending order
    std::sort(scored_peers.begin(), scored_peers.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    // The peer with the highest score becomes the leader
    current_leader_ = scored_peers.front().first;
    std::cout << "[Leader Election] New leader elected: " << current_leader_ << std::endl;

    return current_leader_;
}

std::string TARAlgorithm::getCurrentLeader() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return current_leader_;
}
