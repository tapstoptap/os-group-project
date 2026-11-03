# Approach 3 – Shared Variable with Synchronization (mutex)

Monte Carlo π estimation where **all threads update one global counter protected by a `pthread_mutex_t`**.
The result is correct, but the lock serializes the work, so it is slow. This part of the project measures how
slow, and how the cost shrinks when threads synchronize less often.

Other approaches (single thread = Approach 1, thread-local counters = Approach 2) are documented by their owners.
Here, `k = 0` (local accumulation) is only a *reference* that behaves like Approach 2, and `-m single` is only a
baseline for the speedup.

## 1. Requirements

- Linux (we used Ubuntu in VirtualBox, 5 virtual cores) and `gcc`; no `make` target is needed
- `python3` and `matplotlib` for the plots: `sudo apt install -y python3-matplotlib`

## 2. Files

| Path | What it is |
| --- | --- |
| `src/approach3.c` | The program (pthread, mutex, sync interval `-k`, single-thread baseline `-m single`) |
| `scripts/approach3_bench.sh` | Builds the program and runs every benchmark, writes the raw CSV |
| `scripts/approach3_plot.py` | Reads the raw CSV, writes the summary table and the 4 figures |
| `results/approach3_results.csv` | Raw data: one row per run (`mode,threads,k,n,run,pi,time_sec`) |
| `results/approach3_summary.csv` | Mean time, standard deviation, speedup and efficiency per configuration |
| `results/approach3_sysinfo.txt` | Machine and parameters used for the run |
| `figures/approach3_*.png` | Time, speedup, efficiency vs threads; time vs sync interval `k` |
| `docs/` | Written analysis of Approach 3 (`Approach 3 – Shared Variable with Synchronization: Analysis`) |

`build/` (the compiled binary) is created by the script and must not be committed.

## 3. Quick start

Run everything from the repository root.

```bash
# 1. compile
mkdir -p build
gcc -O2 -Wall -pthread -o build/approach3 src/approach3.c

# 2. one run (human-readable output)
./build/approach3 -n 20000000 -t 5 -k 1 -v
# prints: mode=shared threads=5 k=1 n=20000000 inside=<count> pi=3.141648 time=<seconds>s

# 3. full benchmark (about 1 minute at the default N)
./scripts/approach3_bench.sh

# 4. tables and figures
python3 scripts/approach3_plot.py
```

Close heavy applications first; the benchmark is sensitive to other load on the machine.

### Program options

```
./build/approach3 -n <points> -t <threads> -k <sync_interval> [-s seed] [-m shared|single] [-H] [-v]
```

| Option | Meaning | Default |
| --- | --- | --- |
| `-n` | Total number of random points (split evenly over the threads) | 20,000,000 |
| `-t` | Number of threads | 4 |
| `-k` | Synchronization interval (see below) | 1 |
| `-s` | Base random seed; each thread derives its own independent seed | 12345 |
| `-m` | `shared` = pthreads with a mutex, `single` = single-thread baseline without mutex | `shared` |
| `-H` | Print the CSV header first | off |
| `-v` | Human-readable output instead of CSV | off |

CSV output columns: `mode,threads,k,n,pi,time_sec`. Only the compute phase is timed, from the first
`pthread_create` to the last `pthread_join`; nothing is printed inside the timed region.

### What `-k` means

| `-k` | Behaviour | Used for |
| --- | --- | --- |
| `1` | Lock, add the result of the point (0 or 1), unlock, **on every iteration** | **Approach 3** as specified |
| `10` … `100000` | Count in a local variable; lock and add it to the shared counter every `k` iterations | Experiment 6.1 (sync frequency) |
| `0` | Count locally; one locked update when the thread finishes | Reference equal to Approach 2 |

Each thread has its own xorshift64\* random generator (`rand()` would add a hidden shared state).

### Benchmark settings

Override with environment variables, for example `N=100000000 RUNS=5 ./scripts/approach3_bench.sh`.

| Variable | Default | Meaning |
| --- | --- | --- |
| `N` | `20000000` | Total points, the same for every configuration |
| `RUNS` | `5` | Runs per configuration (mean is reported) |
| `SEED` | `12345` | Base seed (fixed, so π values are reproducible) |
| `THREADS` | `1 2 4 <cores> 2×<cores> 16 32 64 100` | Thread counts for the sweep |
| `KS` | `1 10 100 1000 10000 100000 0` | Sync intervals for experiment 6.1 |
| `KTHREADS` | `<cores>` | Thread count used in experiment 6.1 |

The script first measures the single-thread baseline (`T_single`), then sweeps the thread counts with `k = 1`
(Approach 3) and `k = 0` (reference), then sweeps `k` with `KTHREADS` threads.

## 4. Results on our machine

Ubuntu VM (VirtualBox), 5 virtual cores, N = 20,000,000, mean of 5 runs (10 runs for 5 threads with k = 1), `T_single` = 0.0742 s.
Speedup = `T_single / T_parallel`; efficiency = speedup / threads.

| Threads | Time k=1 (s) | Speedup k=1 | Time k=0 (s) | Speedup k=0 |
| --- | --- | --- | --- | --- |
| 1 | 0.1076 | 0.69 | 0.0672 | 1.10 |
| 2 | 0.4443 | 0.17 | 0.0367 | 2.03 |
| 4 | 0.2462 | 0.30 | 0.0179 | 4.14 |
| 5 | 0.3085 | 0.24 | 0.0253 | 2.94 |
| 10 | 0.5014 | 0.15 | 0.0194 | 3.84 |
| 100 | 0.5286 | 0.14 | 0.0248 | 3.00 |

Synchronization interval (5 threads): k = 1 → 0.309 s, k = 10 → 0.082 s, k = 100 → 0.020 s;
from k = 100 up the differences are within the run-to-run noise.

Take-aways:

- With a mutex on every update, no thread count beats the single-thread baseline (speedup ≤ 0.69).
- Updating the shared counter every 100 iterations instead of every iteration is about 15× faster and
  reaches the performance of local accumulation.
- π is identical for every `k` at a given thread count, so the mutex costs time but never correctness.

Caveats: this is a VM with 5 virtual cores shared with the host, runs with `k = 0` last only about 20 ms (so the
spread is 10–20 %), and the 1-thread speedup above 1 is measurement noise in the baseline, not a real gain.
The full discussion is in the analysis document in `docs/`.

## 5. Reproducibility checklist

- Same `N`, seed and thread counts for all approaches when comparing them.
- Record the machine in `results/approach3_sysinfo.txt` (the script does this automatically).
- Do not run other heavy programs during the benchmark.
- If a run is much slower than the numbers above, rerun it; one noisy run should not be used for the report.
