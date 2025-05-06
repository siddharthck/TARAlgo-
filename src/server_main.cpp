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
#include <cstdlib> // For std::getenv
#include <sysinfo.h> // Include the sysinfo library

// Atomic flag to control the heartbeat thread
std::atomic<bool> keep_sending_heartbeats(true);

// Helper function to get environment variable or default value
int GetEnvOrDefault(const char* env_var, int default_value) {
    const char* value = std::getenv(env_var);
    return value ? std::stoi(value) : default_value;
}

// Function to send heartbeats to peers
void SendHeartbeats(const std::string& server_id,
                    const std::vector<std::string>& peer_addresses,
                    TARServiceImpl* service) {
    int leader_timeout = GetEnvOrDefault("LEADER_TIMEOUT", 10); // Timeout in seconds
    auto last_leader_heartbeat = std::chrono::system_clock::now();

    // Load thresholds from environment variables or use default values
    int underloaded_threshold = GetEnvOrDefault("UNDERLOADED_THRESHOLD", 2);
    int overloaded_threshold = GetEnvOrDefault("OVERLOADED_THRESHOLD", 10);
    int max_hop_count = GetEnvOrDefault("MAX_HOP_COUNT", 2);

    sysinfo::System system_info; // Create a system info object
    system_info.refresh_cpu();   // Refresh CPU metrics

    while (keep_sending_heartbeats) {
        system_info.refresh_cpu(); // Refresh CPU metrics before each heartbeat

        tar::ServerMetrics metrics;
        metrics.set_server_id(server_id);
        metrics.set_cpu_utilization(system_info.cpu_usage()); // Set actual CPU utilization
        metrics.set_queue_length(service->getTaskQueueLength());
        metrics.set_last_heartbeat(std::chrono::system_clock::now().time_since_epoch().count());

        bool is_underloaded = metrics.queue_length() < underloaded_threshold;

        for (const auto& peer : peer_addresses) {
            auto stub = tar::TARService::NewStub(
                grpc::CreateChannel(peer, grpc::InsecureChannelCredentials()));

            tar::ServerMetrics response;
            grpc::ClientContext context;

            // Measure latency
            auto start = std::chrono::high_resolution_clock::now();
            auto status = stub->Heartbeat(&context, metrics, &response);
            auto end = std::chrono::high_resolution_clock::now();

            if (status.ok()) {
                auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
                std::cout << "[Latency] Measured latency to " << peer << ": " << latency << " ms" << std::endl;

                // Update metrics
                response.set_network_latency(latency);
                service->updateServerMetrics(response);

                // Update leader heartbeat if the current leader responds
                if (response.server_id() == service->getAlgorithm().getCurrentLeader()) {
                    last_leader_heartbeat = std::chrono::system_clock::now();
                }

                // Check if the peer is overloaded
                bool is_overloaded = response.queue_length() > overloaded_threshold;

                // Steal tasks if this server is underloaded and the peer is overloaded
                if (is_underloaded && is_overloaded) {
                    tar::Task stolen_task;
                    grpc::ClientContext steal_context;
                    auto steal_status = stub->RequestTaskTransfer(&steal_context, metrics, &stolen_task);

                    if (steal_status.ok()) {
                        // Check the hop count of the stolen task
                        if (stolen_task.hop_count() < max_hop_count) {
                            stolen_task.set_hop_count(stolen_task.hop_count() + 1);
                            std::cout << "[Task Stealing] Stole task: " << stolen_task.id()
                                      << " (Hop Count: " << stolen_task.hop_count() << ") from " << peer << std::endl;
                            service->addTaskToQueue(stolen_task); // Add the stolen task to the local queue
                        } else {
                            std::cout << "[Task Stealing] Task " << stolen_task.id()
                                      << " has reached max hop count. Keeping it on the current server." << std::endl;
                        }
                    } else {
                        std::cout << "[Task Stealing] No tasks available to steal from " << peer << std::endl;
                    }
                }
            } else {
                std::cerr << "[Heartbeat] Failed to send to " << peer << ": " << status.error_message() << std::endl;
            }
        }

        // Check if the leader is unreachable
        auto now = std::chrono::system_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - last_leader_heartbeat).count() > leader_timeout) {
            std::cout << "[Leader Election] Current leader is unreachable. Electing a new leader..." << std::endl;
            service->getAlgorithm().electLeader();
            last_leader_heartbeat = now; // Reset the timeout
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
