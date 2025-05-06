#include "TARServiceImpl.hpp"
#include <iostream>

TARServiceImpl::TARServiceImpl(const std::string& server_id, const std::vector<std::string>& peers)
    : algorithm_(server_id, peers) {}

grpc::Status TARServiceImpl::RouteTask(grpc::ServerContext*,
                                       const tar::RouteTaskRequest* request,
                                       tar::RouteTaskResponse* response) {
    if (!request) {
return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Null request");
}

    // Log the incoming task details
    std::cout << "[Server] Received RouteTask request:" << std::endl;
    std::cout << "  Task ID: " << request->task().id() << std::endl;
    std::cout << "  Priority: " << request->task().priority() << std::endl;
    std::cout << "  Requester ID: " << request->requester_metrics().server_id() << std::endl;

    auto targets = algorithm_.routeTask(request->task(), request->requester_metrics());
    response->mutable_target_servers()->Add(targets.begin(), targets.end());
    response->set_is_coordinator(algorithm_.shouldBecomeCoordinator());

    // Log the routing decision
    std::cout << "[Server] Task routed to: ";
    for (const auto& target : targets) {
        std::cout << target << " ";
    }
    std::cout << std::endl;

    return grpc::Status::OK;
}

grpc::Status TARServiceImpl::AcknowledgeTask(grpc::ServerContext*,
                                             const tar::TaskAck* request,
                                             tar::TaskAck* response) {
    if (!request) {
return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Null request");
}

    // Log the acknowledgment details
    std::cout << "[Server] Received AcknowledgeTask request:" << std::endl;
    std::cout << "  Task ID: " << request->task_id() << std::endl;
    std::cout << "  From Server: " << request->server_id() << std::endl;

    bool success = algorithm_.acknowledgeTask(*request);
    response->CopyFrom(*request);
    response->set_success(success);

    // Log the acknowledgment result
    std::cout << "[Server] AcknowledgeTask result: " << (success ? "Success" : "Failure") << std::endl;

    return grpc::Status::OK;
}

grpc::Status TARServiceImpl::Heartbeat(grpc::ServerContext*,
                                       const tar::ServerMetrics* request,
                                       tar::ServerMetrics* response) {
    if (!request) {
return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Null request");
}

    // Log the heartbeat details
    std::cout << "[Server] Received Heartbeat from: " << request->server_id() << std::endl;
    std::cout << "  CPU Utilization: " << request->cpu_utilization() << std::endl;
    std::cout << "  Queue Length: " << request->queue_length() << std::endl;
    std::cout << "  Last Heartbeat: " << request->last_heartbeat() << std::endl;

    algorithm_.updateServerMetrics(*request);
    response->CopyFrom(*request);

    // Log the updated metrics
    std::cout << "[Server] Updated metrics for: " << request->server_id() << std::endl;

    return grpc::Status::OK;
}

grpc::Status TARServiceImpl::RequestTaskTransfer(grpc::ServerContext*,
                                                 const tar::ServerMetrics* request,
                                                 tar::Task* response) {
    if (!request) {
return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Null request");
}

    // Log the task transfer request
    std::cout << "[Server] Received RequestTaskTransfer from: " << request->server_id() << std::endl;

    auto task_opt = algorithm_.requestTaskTransfer(*request);
    if (!task_opt.has_value()) {
std::cout << "[Server] No task available for transfer to: " << request->server_id() << std::endl;
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "No task available");
    }

    response->CopyFrom(task_opt.value());

    // Log the task transfer details
    std::cout << "[Server] Task transferred to: " << request->server_id() << std::endl;
    std::cout << "  Task ID: " << response->id() << std::endl;

    return grpc::Status::OK;
}

void TARServiceImpl::addTaskToQueue(const tar::Task& task) {
    algorithm_.addTaskToQueue(task);
}

void TARServiceImpl::updateServerMetrics(const tar::ServerMetrics& metrics) {
    algorithm_.updateServerMetrics(metrics);
}
