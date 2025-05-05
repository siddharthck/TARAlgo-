#include "TARServiceImpl.hpp"
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <chrono>
#include <atomic>

// Atomic flag to control the heartbeat thread
std::atomic<bool> keep_sending_heartbeats(true);

// Function to send heartbeats to peers
void SendHeartbeats(const std::string& server_id,
                    const std::vector<std::string>& peer_addresses,
                    TARServiceImpl* service) {
    while (keep_sending_heartbeats) {
        tar::ServerMetrics metrics;
        metrics.set_server_id(server_id);
        metrics.set_cpu_utilization(0.5); // Example value, replace with actual CPU utilization
        metrics.set_queue_length(service->getTaskQueueLength()); // Example, implement this in TARServiceImpl
        metrics.set_last_heartbeat(std::chrono::system_clock::now().time_since_epoch().count());

        for (const auto& peer : peer_addresses) {
            auto stub = tar::TARService::NewStub(
                grpc::CreateChannel(peer, grpc::InsecureChannelCredentials()));

            tar::ServerMetrics response;
            grpc::ClientContext context;
            auto status = stub->Heartbeat(&context, metrics, &response);

            if (status.ok()) {
                std::cout << "[Heartbeat] Sent to " << peer << std::endl;
            } else {
                std::cerr << "[Heartbeat] Failed to send to " << peer << ": " << status.error_message() << std::endl;
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(5)); // Send heartbeats every 5 seconds
    }
}

void RunServer(const std::string& server_id,
               const std::string& bind_address,
               const std::vector<std::string>& peer_addresses) {
    std::cout << "[Server] ID: " << server_id << ", Binding on: " << bind_address << std::endl;

    TARServiceImpl* service = new TARServiceImpl(server_id, peer_addresses);

    grpc::EnableDefaultHealthCheckService(true);
    grpc::reflection::InitProtoReflectionServerBuilderPlugin();

    grpc::ServerBuilder builder;
    builder.AddListeningPort(bind_address, grpc::InsecureServerCredentials());
    builder.RegisterService(service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());

    if (!server) {
        std::cerr << "[Error] Failed to start gRPC server on " << bind_address << std::endl;
        return;
    }

    std::cout << "[Server] Running on " << bind_address << std::endl;

    // Start a single thread to send heartbeats
    std::thread heartbeat_thread(SendHeartbeats, server_id, peer_addresses, service);

    // Wait for the server to shut down
    server->Wait();

    // Stop the heartbeat thread when the server shuts down
    keep_sending_heartbeats = false;
    heartbeat_thread.join();
}

int main(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <server_id> <bind_address> <peer1> [peer2] ..." << std::endl;
        return 1;
    }

    std::string server_id = argv[1];
    std::string bind_address = argv[2];
    std::vector<std::string> peers;

    for (int i = 3; i < argc; ++i) {
        peers.push_back(argv[i]);
    }

    RunServer(server_id, bind_address, peers);
    return 0;
}
