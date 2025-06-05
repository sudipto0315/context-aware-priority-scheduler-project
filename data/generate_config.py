import json
import random

# Configuration
NUM_FOG_NODES = 4
NUM_PROCESSES = 8
LOCATIONS = ["Zone_A", "Zone_B", "Zone_C", "Zone_D"]
APPLICATION_TYPES = [
    "video_streaming", "gaming", "cloud_offload", "ai_inference",
    "autonomous_driving", "industrial_automation", "iot_data_collection", "virtual_reality"
]

# Moderate ranges for node attributes
moderate_cpu = (8, 12)
moderate_memory = (16, 20)
moderate_bandwidth = (400, 600)
moderate_delay = (10, 20)
moderate_packet_loss = (0.001, 0.01)
moderate_current_load = (0.1, 0.3)

# Node profiles with specific strengths and trade-offs
profiles = [
    {"delay": (2.0, 4.0), "cpu_capacity": moderate_cpu, "memory": moderate_memory, "network_bandwidth": moderate_bandwidth, "packet_loss": moderate_packet_loss, "current_load": moderate_current_load},
    {"network_bandwidth": (700, 900), "delay": moderate_delay, "cpu_capacity": moderate_cpu, "memory": moderate_memory, "packet_loss": moderate_packet_loss, "current_load": moderate_current_load},
    {"cpu_capacity": (14, 16), "delay": moderate_delay, "memory": moderate_memory, "network_bandwidth": moderate_bandwidth, "packet_loss": moderate_packet_loss, "current_load": moderate_current_load},
    {"memory": (24, 26), "delay": moderate_delay, "cpu_capacity": moderate_cpu, "network_bandwidth": moderate_bandwidth, "packet_loss": moderate_packet_loss, "current_load": moderate_current_load},
    {"packet_loss": (0.0001, 0.001), "delay": moderate_delay, "cpu_capacity": moderate_cpu, "memory": moderate_memory, "network_bandwidth": moderate_bandwidth, "current_load": moderate_current_load},
    {"current_load": (0.0, 0.1), "delay": moderate_delay, "cpu_capacity": moderate_cpu, "memory": moderate_memory, "network_bandwidth": moderate_bandwidth, "packet_loss": moderate_packet_loss},
    {"delay": (2.0, 4.0), "network_bandwidth": (700, 900), "cpu_capacity": (4, 6), "memory": (12, 14), "packet_loss": moderate_packet_loss, "current_load": moderate_current_load},
    {"cpu_capacity": (14, 16), "memory": (24, 26), "delay": (40, 50), "network_bandwidth": (100, 300), "packet_loss": moderate_packet_loss, "current_load": moderate_current_load}
]

# Generate fog nodes
fog_nodes = []
for i in range(NUM_FOG_NODES):
    profile = profiles[i%len(profiles)]
    node = {
        "id": i + 1,
        "cpu_capacity": random.uniform(*profile["cpu_capacity"]),
        "memory": random.uniform(*profile["memory"]),
        "network_bandwidth": random.uniform(*profile["network_bandwidth"]),
        "current_load": random.uniform(*profile["current_load"]),
        "delay": random.uniform(*profile["delay"]),
        "packet_loss": random.uniform(*profile["packet_loss"]),
        "location": LOCATIONS[i // 2],  # Two nodes per zone
        "is_active": True
    }
    fog_nodes.append(node)

# Generate processes
processes = []
for i in range(NUM_PROCESSES):
    arrival_time = random.uniform(0, 100)
    burst_time = random.randint(5, 60)
    priority = random.randint(1, 5)
    user_id = random.randint(100, 200)
    mobility = random.uniform(0, 1)
    relinquish_probability = random.uniform(0, 1)
    usage_history = ",".join([str(round(random.uniform(0, 1), 2)) for _ in range(3)])
    nps = random.randint(1, 10)
    application_type = random.choice(APPLICATION_TYPES)
    latency_sensitivity = random.uniform(0, 1)
    current_task_load = random.uniform(0, 1)
    request_location = random.choice(LOCATIONS)
    request_time = arrival_time + random.uniform(0, 1)
    required_bandwidth = random.uniform(10, 100)
    
    # Ensure process is schedulable by basing constraints on a random node
    node = random.choice(fog_nodes)
    max_delay = node["delay"] + random.uniform(5, 15)
    max_packet_loss = node["packet_loss"] + random.uniform(0.005, 0.02)
    
    battery_lifetime = random.randint(10, 100)
    
    # Occasionally require partitioning with higher resource demands
    if random.random() < 0.2:
        required_cpu = random.uniform(4.0, 8.0)
        required_memory = random.uniform(8.0, 16.0)
    else:
        required_cpu = random.uniform(0.5, 4.0)
        required_memory = random.uniform(1.0, 8.0)
    
    data_size = random.randint(100, 2000)
    
    process = {
        "id": i + 1,
        "arrival_time": arrival_time,
        "burst_time": burst_time,
        "priority": priority,
        "user_id": user_id,
        "mobility": mobility,
        "relinquish_probability": relinquish_probability,
        "usage_history": usage_history,
        "nps": nps,
        "application_type": application_type,
        "latency_sensitivity": latency_sensitivity,
        "current_task_load": current_task_load,
        "request_location": request_location,
        "request_time": request_time,
        "required_bandwidth": required_bandwidth,
        "max_packet_loss": max_packet_loss,
        "max_delay": max_delay,
        "battery_lifetime": battery_lifetime,
        "required_resources": {
            "cpu": required_cpu,
            "memory": required_memory
        },
        "data_size": data_size
    }
    processes.append(process)

# Create config dictionary
config = {
    "scheduler": "ContextAware",
    "fog_nodes": fog_nodes,
    "processes": processes
}

# Save to file
with open("src/simulation/config3.json", "w") as f:
    json.dump(config, f, indent=4)