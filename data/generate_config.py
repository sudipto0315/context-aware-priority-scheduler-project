import random
import json

num_fog_nodes = 6
num_processes = 12
zones = ['Zone_A', 'Zone_B', 'Zone_C', 'Zone_D']
app_types = ['video_streaming', 'gaming', 'cloud_offload', 'ai_inference', 'autonomous_driving', 'industrial_automation', 'big_data_processing', 'iot_monitoring', 'sensor_data_processing', 'real_time_analytics']

fog_nodes = []
for i in range(1, num_fog_nodes + 1):
    fog_node = {
        "id": i,
        "cpu_capacity": random.randint(1, 10),
        "memory": random.randint(2, 16),
        "network_bandwidth": random.randint(20, 200),
        "current_load": round(random.uniform(0.1, 0.9), 2),
        "delay": round(random.uniform(2.0, 20.0), 1),
        "packet_loss": round(random.uniform(0.001, 0.05), 3),
        "location": random.choice(zones),
        "is_active": random.choices([True, False], weights=[0.9, 0.1])[0]
    }
    fog_nodes.append(fog_node)

processes = []
for i in range(1, num_processes + 1):
    arrival_time = i - 1
    process = {
        "id": i,
        "arrival_time": arrival_time,
        "burst_time": random.randint(1, 20),
        "priority": random.randint(1, 5),
        "user_id": 100 + i,
        "mobility": round(random.uniform(0.1, 0.9), 2),
        "relinquish_probability": round(random.uniform(0.01, 0.5), 2),
        "usage_history": ','.join([str(round(random.uniform(0, 1), 1)) for _ in range(3)]),
        "nps": random.randint(1, 10),
        "application_type": random.choice(app_types),
        "latency_sensitivity": round(random.uniform(0.5, 1.0), 2),
        "current_task_load": round(random.uniform(0.1, 1.0), 2),
        "request_location": random.choice(zones),
        "request_time": arrival_time + 1.0,
        "required_bandwidth": random.randint(5, 100),
        "max_packet_loss": round(random.uniform(0.005, 0.05), 3),
        "max_delay": random.randint(1, 20),
        "battery_lifetime": random.randint(20, 100),
        "required_resources": {
            "cpu": round(random.uniform(0.5, 10.0), 1),
            "memory": random.randint(1, 16)
        },
        "data_size": random.randint(100, 2000)
    }
    processes.append(process)

config = {
    "scheduler": "ContextAware",
    "fog_nodes": fog_nodes,
    "processes": processes
}

with open('config.json', 'w') as f:
    json.dump(config, f, indent=4)