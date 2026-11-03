#!/usr/bin/env bash
# Benchmark for Approach 3 (shared counter + mutex), with a single-thread baseline
# and a local-accumulation reference (k=0, equivalent to Approach 2) for comparison.
#
# Run from anywhere:   ./scripts/approach3_bench.sh
# Override defaults:   N=100000000 RUNS=5 ./scripts/approach3_bench.sh
#
# Reads:   src/approach3.c
# Builds:  build/approach3                      (not committed; see .gitignore)
# Writes:  results/approach3_results.csv        (mode,threads,k,n,run,pi,time_sec)
#          results/approach3_sysinfo.txt        (machine + parameters of this run)
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
SRC="$ROOT/src/approach3.c"
BIN_DIR="$ROOT/build"
BIN="$BIN_DIR/approach3"
RES_DIR="$ROOT/results"
mkdir -p "$BIN_DIR" "$RES_DIR"

N=${N:-20000000}          # total points, identical for every configuration
RUNS=${RUNS:-5}           # >= 5 runs per configuration
SEED=${SEED:-12345}       # fixed seed -> reproducible pi values
CORES=$(nproc)
# thread counts: below / equal / above the number of cores
THREADS=${THREADS:-"1 2 4 $CORES $((CORES*2)) 16 32 64 100"}
THREADS=$(echo $THREADS | tr ' ' '\n' | sort -n -u | tr '\n' ' ')
# sync intervals for experiment 6.1 (thread count fixed to KTHREADS)
KS=${KS:-"1 10 100 1000 10000 100000 0"}
KTHREADS=${KTHREADS:-$CORES}

echo "== Compile =="
gcc -O2 -Wall -pthread -o "$BIN" "$SRC"

{
  echo "date: $(date)"
  echo "cores (nproc): $CORES"
  lscpu | grep -E 'Model name|^CPU\(s\)|Thread|Core|MHz' || true
  uname -srmo
  gcc --version | head -1
  echo "N=$N RUNS=$RUNS SEED=$SEED THREADS=[$THREADS] KS=[$KS] KTHREADS=$KTHREADS"
} > "$RES_DIR/approach3_sysinfo.txt"

OUT="$RES_DIR/approach3_results.csv"
echo "mode,threads,k,n,run,pi,time_sec" > "$OUT"

run_cfg () {  # args: mode threads k
  local mode=$1 t=$2 k=$3
  for r in $(seq 1 "$RUNS"); do
    # program prints: mode,threads,k,n,pi,time_sec  -> insert the run index
    line=$("$BIN" -n "$N" -t "$t" -k "$k" -s "$SEED" -m "$mode")
    IFS=, read -r m th kk nn pi tm <<< "$line"
    echo "$m,$th,$kk,$nn,$r,$pi,$tm" >> "$OUT"
  done
  echo "  done: mode=$mode threads=$t k=$k"
}

echo "== Baseline: single thread (T_single) =="
run_cfg single 1 0

echo "== Thread sweep: Approach 3 (k=1, mutex on every iteration) =="
for t in $THREADS; do run_cfg shared "$t" 1; done

echo "== Thread sweep: local accumulation (k=0, ~Approach 2) for comparison =="
for t in $THREADS; do run_cfg shared "$t" 0; done

echo "== Experiment 6.1: synchronization frequency (threads=$KTHREADS) =="
for k in $KS; do run_cfg shared "$KTHREADS" "$k"; done

echo "Finished -> $OUT"
echo "Next: python3 scripts/approach3_plot.py"
