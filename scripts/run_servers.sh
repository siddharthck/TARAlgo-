#!/bin/bash

ports=(50051 50052 50053 50054 50055)
server_executable="../build/server"

for i in "${!ports[@]}"; do
  server_id="server$((i+1))"
  addr="localhost:${ports[$i]}"
  peers=""
  for j in "${!ports[@]}"; do
    if [[ $j -ne $i ]]; then
      peers+="localhost:${ports[$j]} "
    fi
  done
  echo "Launching $server_id on $addr with peers: $peers"
  $server_executable $server_id $addr $peers &
  sleep 1
done

wait