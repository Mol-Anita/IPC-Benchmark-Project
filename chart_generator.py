#!/usr/bin/python3
import matplotlib.pyplot as plt
import json
import numpy as np
import sys

if len(sys.argv) < 3:
    print("Usage: " + sys.argv[0] + " <json_data_file> <png_output_file> [<chart_title>]")
    sys.exit(-1)

with open(sys.argv[1], 'r') as file:
    data = json.load(file)

print("Loaded Data:")
print(json.dumps(data, indent=2))

server_labels = {
    "tcp_server": "TCP",
    "fifo_server": "FIFO",
    "usocket_server": "UNIX SOCKET",
    "msgqueue_server": "MESSAGE QUEUE"
}

bardata = {}
for server_type in data:
    for scenario in data[server_type]:
        if scenario not in bardata:
            bardata[scenario] = [[], [], []]
        indata = data[server_type][scenario]
        metric_ms = indata[1] * 1000
        stddev_ms = indata[4] * 1000
        bardata[scenario][0].append(metric_ms)
        bardata[scenario][1].append(metric_ms - stddev_ms)
        bardata[scenario][2].append(stddev_ms)

plt.figure(figsize=(12, 8))
if len(sys.argv) > 3:
    plt.title(sys.argv[3])

group_ids = np.arange(len(data))
offset_step = 0.8 / len(bardata)
offset = -(len(bardata) - 1) / 2

for scenario in bardata:
    print(f"Plotting data for scenario: {scenario}")
    plt.bar(group_ids + offset_step * offset,
            bardata[scenario][0],
            offset_step * 0.8,
            yerr=bardata[scenario][2],
            capsize=5,
            label=scenario)
    for i in range(len(group_ids)):
        plt.text(group_ids[i] + offset_step * offset,
                 bardata[scenario][0][i] + bardata[scenario][2][i] + 0.1,
                 f"{bardata[scenario][0][i]:.3f} ms",
                 ha='center', va='bottom', fontsize=8)
    offset += 1

plt.xticks(group_ids, [server_labels.get(server_type, server_type) for server_type in data.keys()], rotation=45)

plt.xlabel("Server Types")
plt.ylabel("Latency (ms)")

plt.legend()
plt.tight_layout()
plt.savefig(sys.argv[2])
print(f"Saved chart to {sys.argv[2]}")
