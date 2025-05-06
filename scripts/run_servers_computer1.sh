#!/bin/bash

# Ports for servers on Computer 1
ports=(50051 50052)
server_executable="../build/server"

# IP address of Computer 1
computer1_ip="192.168.1.101"
# IP address of Computer 2
computer2_ip="192.168.1.102"

for i in "${!ports[@]}"; do
  server_id="server$((i+1))"
  addr="$computer1_ip:${ports[$i]}"
  peers="$computer2_ip:50053 $computer2_ip:50054 $computer2_ip:50055" # Peers on Computer 2
  for j in "${!ports[@]}"; do
    if [[ $j -ne $i ]]; then
      peers+=" $computer1_ip:${ports[$j]}" # Add other peers on Computer 1
    fi
  done
  echo "Launching $server_id on $addr with peers: $peers"
  $server_executable $server_id $addr $peers &
  sleep 1
done

wait