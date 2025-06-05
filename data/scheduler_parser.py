import csv

def parse_process_details(text):
    processes = {}
    lines = text.splitlines()
    for i, line in enumerate(lines):
        if line.startswith("=== ") and " Scheduled Processes Summary ===" in line:
            for j in range(i + 1, len(lines)):
                if lines[j].strip().startswith("Process_ID"):
                    header_line = lines[j].strip()
                    separator = "\t" if "\t" in header_line else ","
                    headers = [h.strip() for h in header_line.split(separator)]
                    data_lines = []
                    for k in range(j + 1, len(lines)):
                        if lines[k].strip() and not lines[k].startswith("="):
                            data_lines.append(lines[k].strip())
                        else:
                            break
                    for data_line in data_lines:
                        parts = [p.strip() for p in data_line.split(separator)]
                        if len(parts) == len(headers):
                            pid = int(parts[0])
                            process_data = {}
                            for header, value in zip(headers[1:], parts[1:]):
                                key = header.replace("_", " ")
                                if key in ["Arrival Time", "Burst Time"]:
                                    process_data[key] = int(value)
                                elif key in ["Start Time", "Completion Time", "Waiting Time", "Turnaround Time"]:
                                    process_data[key] = float(value)
                                elif key == "Assigned Node" or key == "Assigned Nodes":
                                    process_data["Assigned Nodes"] = value
                                else:
                                    process_data[key] = value
                            processes[pid] = process_data
                    return processes
    return {}

def parse_summary_metrics(text):
    metrics = {}
    in_summary = False
    for line in text.splitlines():
        line = line.strip()
        if "SUMMARY METRICS (CSV FORMAT)" in line:
            in_summary = True
            continue
        if in_summary:
            if line.startswith("==="):
                in_summary = False
                continue
            if "\t" in line:
                key, value_str = line.split("\t", 1)
            elif "," in line:
                key, value_str = line.split(",", 1)
            elif ":" in line:
                key, value_str = line.split(":", 1)
            else:
                continue
            key = key.strip()
            value_str = value_str.strip()
            if "units" in value_str:
                value = float(value_str.split()[0])
            elif "%" in value_str:
                value = float(value_str.split("%")[0])
            elif "processes/unit" in value_str:
                value = float(value_str.split()[0])
            else:
                try:
                    value = float(value_str)
                except ValueError:
                    value = value_str
            metrics[key] = value
    return metrics

def parse_node_utilization(text):
    nodes = []
    lines = text.splitlines()
    for i, line in enumerate(lines):
        if line.startswith("=== NODE UTILIZATION ==="):
            for j in range(i + 1, len(lines)):
                if lines[j].strip().startswith("Node_ID"):
                    header_line = lines[j].strip()
                    separator = "\t" if "\t" in header_line else ","
                    headers = [h.strip() for h in header_line.split(separator)]
                    data_lines = []
                    for k in range(j + 1, len(lines)):
                        if lines[k].strip() and not lines[k].startswith("="):
                            data_lines.append(lines[k].strip())
                        else:
                            break
                    for data_line in data_lines:
                        parts = [p.strip() for p in data_line.split(separator)]
                        if len(parts) == len(headers):
                            node_data = {}
                            for header, value in zip(headers, parts):
                                key = header.replace("_", " ")
                                if key == "Node ID":
                                    node_data[key] = value
                                elif key in ["Utilization Time", "Utilization Percent"]:
                                    node_data[key] = float(value)
                            nodes.append(node_data)
                    return nodes
    return []

def main():
    input_files = {
        "ContextAware": "data/Input_ContextAware.txt",
        "FCFS": "data/input_FCFS.txt",
        "SJF": "data/input_SJF.txt"
    }

    all_rows = []

    for algorithm, filename in input_files.items():
        try:
            with open(filename, "r") as file:
                text = file.read()
        except FileNotFoundError:
            print(f"Warning: {filename} not found. Skipping.")
            continue

        processes = parse_process_details(text)
        summary = parse_summary_metrics(text)
        nodes = parse_node_utilization(text)

        # Process rows
        for pid in processes:
            proc = processes[pid]
            row = {
                'Scheduling Algorithm': algorithm,
                'Type': 'Process',
                'Process ID': str(pid),
                'Arrival Time': str(proc.get('Arrival Time', '')),
                'Burst Time': str(proc.get('Burst Time', '')),
                'Start Time': str(proc.get('Start Time', '')),
                'Completion Time': str(proc.get('Completion Time', '')),
                'Waiting Time': str(proc.get('Waiting Time', '')),
                'Turnaround Time': str(proc.get('Turnaround Time', '')),
                'Assigned Nodes': str(proc.get('Assigned Nodes', '')),
                'Status': proc.get('Status', 'Scheduled'),
                'Metric': '',
                'Value': ''
            }
            all_rows.append(row)

        # Summary rows
        for metric, value in summary.items():
            row = {
                'Scheduling Algorithm': algorithm,
                'Type': 'Summary',
                'Process ID': '',
                'Arrival Time': '',
                'Burst Time': '',
                'Start Time': '',
                'Completion Time': '',
                'Waiting Time': '',
                'Turnaround Time': '',
                'Assigned Nodes': '',
                'Status': '',
                'Metric': metric,
                'Value': str(value)
            }
            all_rows.append(row)

        # Node utilization rows
        for node in nodes:
            row = {
                'Scheduling Algorithm': algorithm,
                'Type': 'Node Utilization',
                'Node ID': node['Node ID'],
                'Utilization Time': str(node['Utilization Time']),
                'Utilization Percent': str(node['Utilization Percent']),
            }
            all_rows.append(row)

    fieldnames = [
        'Scheduling Algorithm', 'Type', 'Process ID', 'Arrival Time', 'Burst Time',
        'Start Time', 'Completion Time', 'Waiting Time', 'Turnaround Time',
        'Assigned Nodes', 'Status', 'Metric', 'Value',
        'Node ID', 'Utilization Time', 'Utilization Percent'
    ]

    with open("data/scheduling_data.csv", "w", newline='') as csvfile:
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        writer.writeheader()
        for row in all_rows:
            writer.writerow(row)

if __name__ == "__main__":
    main()