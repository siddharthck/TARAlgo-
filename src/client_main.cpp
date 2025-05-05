#include <grpcpp/grpcpp.h>
#include <iostream>
#include <memory>
#include <string>
#include <chrono>
#include <thread>
#include <random>
#include <vector>

#include "tar.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

class TARClient {
public:
    TARClient(const std::string& server_address)
        : server_address_(server_address) {
        stub_ = tar::TARService::NewStub(
            grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials()));
    }

    bool SendTask(const std::string& task_id, tar::Priority priority) {
        tar::RouteTaskRequest request;
        tar::Task* task = request.mutable_task();
        task->set_id(task_id);
        task->set_payload("payload_" + task_id);
        task->set_priority(priority);
        task->set_timestamp(std::chrono::system_clock::now().time_since_epoch().count());

        tar::ServerMetrics* metrics = request.mutable_requester_metrics();
        metrics->set_server_id("test_client");
        metrics->set_cpu_utilization(0.0);
        metrics->set_queue_length(0);
        metrics->set_last_heartbeat(std::chrono::system_clock::now().time_since_epoch().count());

        tar::RouteTaskResponse response;
        ClientContext context;
        Status status = stub_->RouteTask(&context, request, &response);

        if (status.ok()) {
            std::cout << "[Client] Task " << task_id << " routed to: ";
            for (const auto& s : response.target_servers()) {
                std::cout << s << " ";
            }
            std::cout << std::endl;
            return true;
        } else {
            std::cerr << "[Client] RouteTask failed: " << status.error_message() << std::endl;
            return false;
        }
    }

private:
    std::string server_address_;
    std::unique_ptr<tar::TARService::Stub> stub_;
};

// Simulate test workload
int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <server_address1> [server_address2] ..." << std::endl;
        return 1;
    }

    std::vector<std::unique_ptr<TARClient>> clients;
    for (int i = 1; i < argc; ++i) {
        clients.emplace_back(std::make_unique<TARClient>(argv[i]));
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> priority_dist(0, 2);
    std::uniform_int_distribution<> client_dist(0, clients.size() - 1);

    for (int i = 0; i < 10; ++i) {
        std::string task_id = "task_" + std::to_string(i);
        tar::Priority priority = static_cast<tar::Priority>(priority_dist(gen));
        int client_index = client_dist(gen);
        clients[client_index]->SendTask(task_id, priority);

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    return 0;
}
