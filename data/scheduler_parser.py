import csv

def parse_process_details(text):
    processes = {}
    current_pid = None
    current_data = {}
    for line in text.splitlines():
        if line.startswith("Process ID:"):
            if current_pid is not None:
                processes[current_pid] = current_data
            current_pid = int(line.split(":")[1].strip())
            current_data = {}
        elif ":" in line:
            key, value = line.split(":", 1)
            key = key.strip()
            value = value.strip()
            if key in ["Arrival Time", "Burst Time", "Priority"]:
                current_data[key] = int(value)
    if current_pid is not None:
        processes[current_pid] = current_data
    return processes

def parse_scheduling_details(text):
    scheduling = {}
    current_pid = None
    current_data = {}
    in_timing_details = False

    for line in text.splitlines():
        line = line.strip()

        # Detect the start of the Per-Process Timing Details section
        if line.startswith("Per-Process Timing Details:"):
            in_timing_details = True
            continue
        # Detect the end of the section
        elif line.startswith("Aggregate Metrics:"):
            in_timing_details = False
            continue

        # Process lines only within the timing details section
        if in_timing_details:
            if line.startswith("Process "):
                if current_pid is not None:
                    scheduling[current_pid] = current_data
                # Extract PID from "Process X:"
                current_pid = int(line.split()[1].strip(":"))
                current_data = {}
            elif line.startswith("Start Time:"):
                value = line.split(":")[1].strip().split()[0]
                current_data['Start Time'] = float(value)
            elif line.startswith("Completion Time:"):
                value = line.split(":")[1].strip().split()[0]
                current_data['Completion Time'] = float(value)
            elif line.startswith("Waiting Time:"):
                value = line.split(":")[1].strip().split()[0]
                current_data['Waiting Time'] = float(value)
            elif line.startswith("Assigned to Node(s):"):
                nodes = line.split(":")[1].strip()
                current_data['Assigned Nodes'] = nodes
            elif line.startswith("Status:"):
                status = line.split(":")[1].strip()
                current_data['Status'] = status

    if current_pid is not None:
        scheduling[current_pid] = current_data
    return scheduling

def parse_summary_metrics(text):
    metrics = {}
    in_aggregate = False
    in_calculation = False
    for line in text.splitlines():
        if line.startswith("Aggregate Metrics:"):
            in_aggregate = True
            in_calculation = False
        elif line.startswith("Calculation Counts:"):
            in_aggregate = False
            in_calculation = True
        elif in_aggregate and ":" in line:
            parts = line.split(":", 1)
            key = parts[0].strip()
            value_str = parts[1].strip()
            if "units" in value_str:
                value = float(value_str.split()[0])
            elif "%" in value_str:
                value = float(value_str.split("%")[0])
            elif "processes/unit" in value_str:
                value = float(value_str.split()[0])
            else:
                value = value_str
            metrics[key] = value
        elif in_calculation and ":" in line:
            key, value = line.split(":", 1)
            key = key.strip()
            value = int(value.strip())
            metrics[key] = value
    return metrics

def main():
    with open("input.txt", "r") as file:
        text = file.read()
    
    processes = parse_process_details(text)
    scheduling = parse_scheduling_details(text)
    summary = parse_summary_metrics(text)
    
    # Combine process data
    process_data = []
    for pid in processes:
        initial = processes[pid]
        sched = scheduling.get(pid, {})
        row = {
            'Process ID': pid,
            'Arrival Time': initial['Arrival Time'],
            'Burst Time': initial['Burst Time'],
            'Priority': initial['Priority'],
            'Start Time': sched.get('Start Time', ''),
            'Completion Time': sched.get('Completion Time', ''),
            'Waiting Time': sched.get('Waiting Time', ''),
            'Assigned Nodes': sched.get('Assigned Nodes', ''),
            'Status': sched.get('Status', 'Not Scheduled')
        }
        process_data.append(row)
    
    # Write processes.csv
    with open("processes.csv", "w", newline='') as csvfile:
        fieldnames = ['Process ID', 'Arrival Time', 'Burst Time', 'Priority', 'Start Time', 'Completion Time', 'Waiting Time', 'Assigned Nodes', 'Status']
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        writer.writeheader()
        for row in process_data:
            writer.writerow(row)
    
    # Write summary.csv
    with open("summary.csv", "w", newline='') as csvfile:
        fieldnames = ['Metric', 'Value']
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        writer.writeheader()
        for metric, value in summary.items():
            writer.writerow({'Metric': metric, 'Value': value})

if __name__ == "__main__":
    main()