#!/usr/bin/env bash
set -e

POINTS=${1:-1000000000}
RUNS=${2:-5}
K_INTERVAL=${3:-1000}
SEED=42
THREADS=(1 2 4 8 16 32 64 128)
OUT="benchmark_results.csv"

# Compile all targets
echo "Compiling targets..."
gcc -O2 -o pi_single pi_single.c common.c
gcc -O2 -pthread -o multiThread multiThread.c
gcc -O2 -pthread -o approach3 approach3.c
echo "Build complete."
echo ""

# Initialize CSV log
echo "approach,threads,k,points,avg_time_sec,speedup,estimated_pi" > "$OUT"

# -------------------------------------------------------------
# Approach 1: Single Thread Baseline
# -------------------------------------------------------------
echo "=== APPROACH 1 SUMMARY TABLE ==="
printf "%-8s %-14s %-9s %-12s\n" "Threads" "Avg_Time_Sec" "Speedup" "Estimated_Pi"

total_time=0
last_pi=0
for ((r=1; r<=RUNS; r++)); do
    # Output: approach,threads,k,points,inside,pi,time_sec
    res=$(./pi_single --points "$POINTS" --seed "$SEED")
    t=$(echo "$res" | awk -F',' '{print $7}')
    last_pi=$(echo "$res" | awk -F',' '{printf "%.6f", $6}')
    total_time=$(awk -v t1="$total_time" -v t2="$t" 'BEGIN {print t1 + t2}')
done

T_SINGLE=$(awk -v tot="$total_time" -v r="$RUNS" 'BEGIN {printf "%.4f", tot / r}')
speedup="1.0000"

printf "%-8s %-14.4f %-9s %-12s\n" "1" "$T_SINGLE" "$speedup" "$last_pi"
echo "approach1,1,0,$POINTS,$T_SINGLE,$speedup,$last_pi" >> "$OUT"
echo ""

# -------------------------------------------------------------
# Approach 2: Multi-Thread Local Accumulation
# -------------------------------------------------------------
echo "=== APPROACH 2 SUMMARY TABLE ==="
printf "%-8s %-14s %-9s %-12s\n" "Threads" "Avg_Time_Sec" "Speedup" "Estimated_Pi"

for th in "${THREADS[@]}"; do
    total_time=0
    last_pi=0
    for ((r=1; r<=RUNS; r++)); do
        output=$(./multiThread "$th" "$POINTS")
        t=$(echo "$output" | grep "Time Taken" | awk '{print $4}')
        last_pi=$(echo "$output" | grep "Estimated Pi" | awk '{print $4}')
        total_time=$(awk -v t1="$total_time" -v t2="$t" 'BEGIN {print t1 + t2}')
    done

    avg_time=$(awk -v tot="$total_time" -v r="$RUNS" 'BEGIN {printf "%.4f", tot / r}')
    speedup=$(awk -v base="$T_SINGLE" -v cur="$avg_time" 'BEGIN {printf "%.4f", base / cur}')

    printf "%-8s %-14.4f %-9.4f %-12s\n" "$th" "$avg_time" "$speedup" "$last_pi"
    echo "approach2,$th,0,$POINTS,$avg_time,$speedup,$last_pi" >> "$OUT"
done
echo ""

# -------------------------------------------------------------
# Approach 3: Shared Variable with Mutex (Thread Sweep)
# -------------------------------------------------------------
echo "=== APPROACH 3 SUMMARY TABLE (k=$K_INTERVAL) ==="
printf "%-8s %-14s %-9s %-12s\n" "Threads" "Avg_Time_Sec" "Speedup" "Estimated_Pi"

for th in "${THREADS[@]}"; do
    total_time=0
    last_pi=0
    for ((r=1; r<=RUNS; r++)); do
        # Output: mode,threads,k,n,pi,time_sec
        res=$(./approach3 -n "$POINTS" -t "$th" -k "$K_INTERVAL" -s "$SEED")
        t=$(echo "$res" | awk -F',' '{print $6}')
        last_pi=$(echo "$res" | awk -F',' '{printf "%.6f", $5}')
        total_time=$(awk -v t1="$total_time" -v t2="$t" 'BEGIN {print t1 + t2}')
    done

    avg_time=$(awk -v tot="$total_time" -v r="$RUNS" 'BEGIN {printf "%.4f", tot / r}')
    speedup=$(awk -v base="$T_SINGLE" -v cur="$avg_time" 'BEGIN {printf "%.4f", base / cur}')

    printf "%-8s %-14.4f %-9.4f %-12s\n" "$th" "$avg_time" "$speedup" "$last_pi"
    echo "approach3,$th,$K_INTERVAL,$POINTS,$avg_time,$speedup,$last_pi" >> "$OUT"
done
echo ""

# -------------------------------------------------------------
# Trade-Off: Approach 3 Synchronization Frequency Sweep (k)
# -------------------------------------------------------------
echo "=== APPROACH 3 (k-Sweep, 4 Threads) ==="
printf "%-8s %-14s %-9s %-12s\n" "Sync_k" "Avg_Time_Sec" "Speedup" "Estimated_Pi"
K_VALUES=(1 10 100 1000 10000 100000)

for k in "${K_VALUES[@]}"; do
    total_time=0
    last_pi=0
    for ((r=1; r<=RUNS; r++)); do
        res=$(./approach3 -n "$POINTS" -t 4 -k "$k" -s "$SEED")
        t=$(echo "$res" | awk -F',' '{print $6}')
        last_pi=$(echo "$res" | awk -F',' '{printf "%.6f", $5}')
        total_time=$(awk -v t1="$total_time" -v t2="$t" 'BEGIN {print t1 + t2}')
    done

    avg_time=$(awk -v tot="$total_time" -v r="$RUNS" 'BEGIN {printf "%.4f", tot / r}')
    speedup=$(awk -v base="$T_SINGLE" -v cur="$avg_time" 'BEGIN {printf "%.4f", base / cur}')

    printf "%-8s %-14.4f %-9.4f %-12s\n" "$k" "$avg_time" "$speedup" "$last_pi"
    echo "approach3_ksweep,4,$k,$POINTS,$avg_time,$speedup,$last_pi" >> "$OUT"
done
echo ""

echo "All tests finished. Raw results stored in $OUT"