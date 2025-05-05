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
        float score = 1.0f / (metrics.queue_length() + 1)
                    + 1.0f / (metrics.cpu_utilization() + 1)
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
    std::cout << "[ACK] Task: " << ack.task_id() << " from server: " << ack.server_id() << std::endl;
    return true;  // Extend for quorum logic later
}

void TARAlgorithm::updateServerMetrics(const tar::ServerMetrics& metrics) {
    std::lock_guard<std::mutex> lock(mutex_);
    peer_metrics_[metrics.server_id()] = metrics;
    std::cout << "[HEARTBEAT] Updated metrics from " << metrics.server_id() << std::endl;
}

std::optional<tar::Task> TARAlgorithm::requestTaskTransfer(const tar::ServerMetrics& requester) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto it = task_queue_.begin(); it != task_queue_.end(); ++it) {
        tar::Task task = it->second;
        task_queue_.erase(it);
        return task;
    }
    return std::nullopt;
}

bool TARAlgorithm::shouldBecomeCoordinator() const {
    return true; // Always true for now
}
