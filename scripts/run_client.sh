#!/bin/bash


# ./build/client 192.168.1.101:50051 192.168.1.101:50052 192.168.1.102:50053 192.168.1.102:50054 192.168.1.102:50055

# Script to run the client and generate a large number of tasks
if [ "$#" -lt 1 ]; then
    echo "Usage: $0 <server_address1> [server_address2] ..."
    exit 1
fi
client_executable="../build/client"
# List of server addresses
servers=("$@")

# Number of tasks to generate
num_tasks=50

# Generate tasks with random priorities and send them to random servers
for ((i=0; i<num_tasks; i++)); do
    task_id="task_$i"
    priority=$((RANDOM % 3)) # Random priority: 0 (LOW), 1 (MODERATE), 2 (URGENT)
    server_index=$((RANDOM % ${#servers[@]})) # Random server index
    server_address=${servers[$server_index]}

    echo "Sending Task $task_id with priority $priority to $server_address"
    $client_executable "$server_address" "$task_id" "$priority" &
    sleep 0.1 # Small delay between task submissions
done

# Wait for all background tasks to complete
wait

echo "All tasks have been sent."