import random
import json

# Configuration
NUM_FOG_NODES = 10
NUM_PROCESSES = 20
LOCATIONS = ["Zone_A", "Zone_B", "Zone_C", "Zone_D"]
APPLICATION_TYPES = [
    "video_streaming", "gaming", "cloud_offload", "ai_inference",
    "autonomous_driving", "industrial_automation", "iot_data_collection", "virtual_reality"
]

# Helper function to generate usage history
def generate_usage_history():
    return ",".join([str(round(random.uniform(0, 1), 2)) for _ in range(3)])

# Generate fog nodes with mixed performance
fog_nodes = []

for i in range(1, NUM_FOG_NODES + 1):
    if random.random() < 0.6:  # 60% chance for high-performance nodes
        delay = round(random.uniform(1, 15), 2)  # Low delay (1-15 ms)
        packet_loss = round(random.uniform(0, 0.005), 4)  # Low packet loss (0-0.005)
        bandwidth = random.randint(200, 1000)  # High bandwidth
        cpu_capacity = random.randint(8, 16)  # Higher CPU
        memory = random.randint(16, 32)  # Higher memory
    else:  # 40% regular nodes
        delay = round(random.uniform(20, 100), 2)  # Higher delay
        packet_loss = round(random.uniform(0.01, 0.1), 4)  # Higher packet loss
        bandwidth = random.randint(10, 200)  # Standard bandwidth
        cpu_capacity = random.randint(1, 8)  # Standard CPU
        memory = random.randint(2, 16)  # Standard memory
    node = {
        "id": i,
        "cpu_capacity": cpu_capacity,
        "memory": memory,
        "network_bandwidth": bandwidth,
        "current_load": round(random.uniform(0, 0.3), 2),  # Lower initial load
        "delay": delay,
        "packet_loss": packet_loss,
        "location": random.choice(LOCATIONS),
        "is_active": True  # Ensure all nodes are active
    }
    fog_nodes.append(node)

# Generate processes with mixed requirements
processes = []
arrival_time = 0
for i in range(1, NUM_PROCESSES + 1):
    arrival_time += random.uniform(0.5, 5)
    if random.random() < 0.3:  # 30% strict processes
        max_delay = round(random.uniform(1, 20), 2)  # Low max delay
        max_packet_loss = round(random.uniform(0, 0.005), 4)  # Low max packet loss
        required_bandwidth = random.randint(50, 100)  # Higher bandwidth
        required_cpu = round(random.uniform(1, 4), 2)
        required_memory = round(random.uniform(2, 8), 2)
    else:  # 70% lenient processes
        max_delay = round(random.uniform(20, 100), 2)
        max_packet_loss = round(random.uniform(0.005, 0.1), 4)
        required_bandwidth = random.randint(1, 50)
        required_cpu = round(random.uniform(0.1, 2), 2)
        required_memory = round(random.uniform(0.5, 4), 2)
    process = {
        "id": i,
        "arrival_time": round(arrival_time, 2),
        "burst_time": random.randint(1, 60),
        "priority": random.randint(1, 5),
        "user_id": random.randint(100, 200),
        "mobility": round(random.uniform(0, 1), 2),
        "relinquish_probability": round(random.uniform(0, 1), 2),
        "usage_history": generate_usage_history(),
        "nps": random.randint(1, 10),
        "application_type": random.choice(APPLICATION_TYPES),
        "latency_sensitivity": round(random.uniform(0, 1), 2),
        "current_task_load": round(random.uniform(0, 1), 2),
        "request_location": random.choice(LOCATIONS),
        "request_time": round(arrival_time + random.uniform(0, 1), 2),
        "required_bandwidth": required_bandwidth,
        "max_packet_loss": max_packet_loss,
        "max_delay": max_delay,
        "battery_lifetime": random.randint(0, 100),
        "required_resources": {
            "cpu": required_cpu,
            "memory": required_memory
        },
        "data_size": random.randint(100, 2000)
    }
    processes.append(process)

# Create config structure
config = {
    "scheduler": "ContextAware",
    "fog_nodes": fog_nodes,
    "processes": processes
}

# Save to file
with open("src/simulation/config.json", "w") as f:
    json.dump(config, f, indent=4)

print(f"Generated 'config.json' with {NUM_FOG_NODES} fog nodes and {NUM_PROCESSES} processes.")