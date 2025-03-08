IPC Benchmarking – Performance Analysis of Inter-Process Communication Mechanisms

🚀 Overview:
This project benchmarks different Inter-Process Communication (IPC) methods—TCP, FIFO, Unix sockets, and message queues—by measuring round-trip time (RTT) for various data sizes.

🛠 Technologies Used:

    C – Client-server implementation for each IPC mechanism
    Bash – Automates benchmarking and aggregates results
    Python – Generates performance charts (matplotlib, JSON parsing)

📊 Features:
✔ Measures and compares IPC performance under different conditions
✔ Automated testing script runs 1000 iterations for accurate results
✔ Data visualization using candlestick bar charts
✔ Modular implementation with extensibility for other IPC mechanisms

🔗 Future Improvements:

    Support for multi-client benchmarking
    Implementation in Python or Java for cross-language performance comparison

📁 How to Run:

    bash run_benchmark.sh
    python chart_generator.py

