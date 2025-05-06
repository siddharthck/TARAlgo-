#!/bin/bash

# Ports for servers on Computer 2
ports=(50053 50054 50055)
server_executable="../build/server"

# IP address of Computer 1
computer1_ip="192.168.1.101"
# IP address of Computer 2
computer2_ip="192.168.1.102"

for i in "${!ports[@]}"; do
  server_id="server$((i+3))" # Start server IDs from 3 for Computer 2
  addr="$computer2_ip:${ports[$i]}"
  peers="$computer1_ip:50051 $computer1_ip:50052" # Peers on Computer 1
  for j in "${!ports[@]}"; do
    if [[ $j -ne $i ]]; then
      peers+=" $computer2_ip:${ports[$j]}" # Add other peers on Computer 2
    fi
  done
  echo "Launching $server_id on $addr with peers: $peers"
  $server_executable $server_id $addr $peers &
  sleep 1
done

wait