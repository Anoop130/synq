#!/bin/bash
# Quick performance benchmark for Synq collector

echo "=== Synq Collector Performance Benchmark ==="
echo ""

# Check if collector is running
PID=$(pgrep -f "build/binaries/collector")

if [ -z "$PID" ]; then
    echo "Collector is not running. Starting it in the background..."
    echo "Please run: ./build/binaries/collector &"
    exit 1
fi

echo "Found collector process: PID $PID"
echo ""

# Get initial memory usage
echo "=== Memory Usage ==="
MEM_KB=$(ps -p $PID -o rss= | tr -d ' ')
MEM_MB=$(echo "scale=2; $MEM_KB / 1024" | bc)
echo "Memory (RSS): ${MEM_MB} MB"
echo ""

# Monitor CPU usage over 30 seconds (6 samples at 5-second intervals)
echo "=== CPU Usage (monitoring for 30 seconds) ==="
echo "Sampling every 5 seconds..."
echo ""

CPU_SAMPLES=()
for i in {1..6}; do
    CPU=$(ps -p $PID -o %cpu= | tr -d ' ')
    echo "Sample $i: ${CPU}%"
    CPU_SAMPLES+=($CPU)
    sleep 5
done

echo ""
echo "=== Summary ==="
echo "Memory Usage: ${MEM_MB} MB"
echo ""
echo "CPU Usage Samples:"
TOTAL=0
for cpu in "${CPU_SAMPLES[@]}"; do
    TOTAL=$(echo "$TOTAL + $cpu" | bc)
done
AVG=$(echo "scale=3; $TOTAL / ${#CPU_SAMPLES[@]}" | bc)
echo "  Average: ${AVG}%"
echo "  Min: $(printf '%s\n' "${CPU_SAMPLES[@]}" | sort -n | head -1)%"
echo "  Max: $(printf '%s\n' "${CPU_SAMPLES[@]}" | sort -n | tail -1)%"
echo ""
echo "=== Results for Resume ==="
echo "CPU Usage: ~${AVG}% average"
echo "Memory Footprint: ${MEM_MB} MB"
