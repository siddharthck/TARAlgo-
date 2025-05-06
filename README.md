# TARAlgo

TARAlgo is a distributed task allocation and replication algorithm designed to handle dynamic workloads in a decentralized environment. It ensures efficient task routing, load balancing, and fault tolerance across multiple nodes in a distributed system. The algorithm leverages features like task stealing, leader election, and adaptive replication to maintain high performance and scalability.

---

## **Features**
1. **Task Routing**:
   - Tasks are routed to the most suitable nodes based on real-time metrics such as CPU utilization, queue length, and network latency.
   - A weighted scoring mechanism ensures that tasks are assigned to healthy nodes.

2. **Task Replication**:
   - Critical tasks are replicated across multiple nodes to ensure fault tolerance.
   - Lazy replication is used for low-priority tasks to optimize resource usage.

3. **Task Stealing**:
   - Underloaded nodes proactively steal tasks from overloaded nodes to balance the workload.
   - A hop counter ensures that tasks are not endlessly transferred between nodes.

4. **Leader Election**:
   - A score-based leader election mechanism dynamically elects a leader when the current leader becomes unreachable.
   - The leader coordinates task routing and replication.

5. **Heartbeat Mechanism**:
   - Nodes exchange periodic heartbeats to share metrics and detect failures.
   - Metrics include CPU utilization, queue length, and last heartbeat timestamp.

6. **Scalability**:
   - Supports both weak scaling (handling increasing workloads) and strong scaling (improving performance with additional nodes).
   - Configurable thresholds and environment variables allow the system to adapt to different environments.

---

## **Project Structure**
- **`src/`**: Contains the source code for the TAR algorithm, server, and client implementations.
  - `TARAlgorithm.cpp`: Core logic for task routing, replication, and leader election.
  - `TARServiceImpl.cpp`: gRPC service implementation for server-to-server and client-to-server communication.
  - `server_main.cpp`: Entry point for the server application.
  - `client_main.cpp`: Entry point for the client application.
- **`scripts/`**: Contains scripts to run the servers and clients.
  - `run_servers.sh`: Launches multiple servers on a single machine.
  - `run_servers_computer1.sh` and `run_servers_computer2.sh`: Launch servers on two separate computers for distributed testing.
  - `run_client.sh`: Simulates task submission to the servers.
- **`proto/`**: Contains the Protocol Buffers (`.proto`) file defining the gRPC services and messages.
- **`README.md`**: Project documentation.

---

## **How to Build and Run**

### **1. Prerequisites**
- **C++ Compiler**: GCC, Clang, or MSVC.
- **CMake**: Build system generator.
- **gRPC and Protocol Buffers**: Installed and configured.
- **sysinfo Library**: For fetching real-time CPU utilization.

### **2. Build the Project**
```bash
mkdir build
cd build
cmake ..
make
