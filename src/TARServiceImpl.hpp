#pragma once

#include "TARAlgorithm.hpp"
#include "tar.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include <memory>

class TARServiceImpl final : public tar::TARService::Service {
public:
    TARServiceImpl(const std::string& server_id, const std::vector<std::string>& peers);

    grpc::Status RouteTask(grpc::ServerContext*,
                           const tar::RouteTaskRequest* request,
                           tar::RouteTaskResponse* response) override;

    grpc::Status AcknowledgeTask(grpc::ServerContext*,
                                 const tar::TaskAck* request,
                                 tar::TaskAck* response) override;

    grpc::Status Heartbeat(grpc::ServerContext*,
                           const tar::ServerMetrics* request,
                           tar::ServerMetrics* response) override;

    grpc::Status RequestTaskTransfer(grpc::ServerContext*,
                                     const tar::ServerMetrics* request,
                                     tar::Task* response) override;

    // New method to get task queue length
    int getTaskQueueLength() const {
        return algorithm_.getTaskQueueLength();
    }

private:
    TARAlgorithm algorithm_;
};
