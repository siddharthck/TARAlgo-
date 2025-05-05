servers=(localhost:50051 localhost:50052 localhost:50053 localhost:50054 localhost:50055)
client_exe="../build/client"

$client_exe "${servers[@]}"

