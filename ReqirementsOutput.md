Fog Computing Scheduler Performance Comparison Guide
6 Key Performance Metrics for Research Paper
1. Average Waiting Time (Lower is Better)

Definition: Average time processes wait before execution begins
Formula: Σ(Start_Time - Arrival_Time) / Number_of_Scheduled_Processes
Unit: Time units
Why Important: Measures user experience and system responsiveness

2. Average Turnaround Time (Lower is Better)

Definition: Average total time from process arrival to completion
Formula: Σ(Completion_Time - Arrival_Time) / Number_of_Scheduled_Processes
Unit: Time units
Why Important: Indicates overall system efficiency

3. Throughput (Higher is Better)

Definition: Number of processes completed per unit time
Formula: Number_of_Scheduled_Processes / Total_Execution_Time
Unit: Processes per time unit
Why Important: Measures system productivity

4. Resource Utilization (Higher is Better)

Definition: Percentage of system resources effectively used
Components: CPU, Memory, Bandwidth utilization
Formula: (Total_Resource_Used / (Total_Resource_Capacity × Total_Execution_Time)) × 100
Unit: Percentage
Why Important: Shows how efficiently the scheduler uses available resources

5. Success Rate (Higher is Better)

Definition: Percentage of processes successfully scheduled
Formula: (Scheduled_Processes / Total_Processes) × 100
Unit: Percentage
Why Important: Measures scheduler reliability and effectiveness

6. Scheduling Overhead (Lower is Better)

Definition: Computational cost of the scheduling algorithm
