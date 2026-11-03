#!/usr/bin/env python3
"""Plot the results of approach3_bench.sh.   Needs: python3 + matplotlib.

Run from anywhere:  python3 scripts/approach3_plot.py

Reads:
  results/approach3_results.csv, results/approach3_sysinfo.txt
Writes:
  results/approach3_summary.csv            averaged table (mean, stdev, speedup, efficiency)
  figures/approach3_time_vs_threads.png    execution time vs threads
  figures/approach3_speedup_vs_threads.png speedup vs threads (ideal line included)
  figures/approach3_efficiency_vs_threads.png  efficiency = speedup / threads
  figures/approach3_sync_frequency.png     experiment 6.1: time vs sync interval k
"""
import csv
import statistics as st
from collections import defaultdict
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

ROOT = Path(__file__).resolve().parent.parent
RES = ROOT / "results"
FIG = ROOT / "figures"
FIG.mkdir(exist_ok=True)

rows = list(csv.DictReader(open(RES / "approach3_results.csv")))
data = defaultdict(list)  # (mode, threads, k) -> [times]
pis = defaultdict(list)
for r in rows:
    key = (r["mode"], int(r["threads"]), int(r["k"]))
    data[key].append(float(r["time_sec"]))
    pis[key].append(float(r["pi"]))

mean = {k: st.mean(v) for k, v in data.items()}
sd = {k: (st.stdev(v) if len(v) > 1 else 0.0) for k, v in data.items()}

t_single = mean[("single", 1, 0)]
print(f"T_single = {t_single:.4f} s")

# number of cores (for the vertical marker), read from the sysinfo file
cores = None
try:
    for line in open(RES / "approach3_sysinfo.txt"):
        if line.startswith("cores (nproc):"):
            cores = int(line.split(":")[1])
except OSError:
    pass


def series(k):
    ts = sorted(t for (m, t, kk) in mean if m == "shared" and kk == k)
    return ts, [mean[("shared", t, k)] for t in ts], [sd[("shared", t, k)] for t in ts]


# ---- summary table ----
with open(RES / "approach3_summary.csv", "w", newline="") as f:
    w = csv.writer(f)
    w.writerow(["mode", "threads", "k", "avg_time_s", "stdev_s", "avg_pi", "speedup", "efficiency"])
    for (m, t, k) in sorted(mean):
        avg = mean[(m, t, k)]
        sp = t_single / avg
        w.writerow([m, t, k, f"{avg:.4f}", f"{sd[(m, t, k)]:.4f}",
                    f"{st.mean(pis[(m, t, k)]):.6f}", f"{sp:.3f}", f"{sp / t:.3f}"])

curves = [(1, "Approach 3 (mutex every iteration, k=1)", "tab:red"),
          (0, "Local accumulation (k=0, ~Approach 2)", "tab:blue")]


def mark_cores(ax):
    if cores:
        ax.axvline(cores, color="gray", ls="--", lw=1)
        ax.text(cores, ax.get_ylim()[0], f" {cores} cores", va="bottom", color="gray")


def save(fig, name):
    fig.tight_layout()
    fig.savefig(FIG / name, dpi=150)
    plt.close(fig)


# ---- 1. time vs threads ----
fig, ax = plt.subplots(figsize=(7, 4.5))
for k, label, c in curves:
    ts, ys, es = series(k)
    if ts:
        ax.errorbar(ts, ys, yerr=es, marker="o", label=label, color=c, capsize=3)
ax.axhline(t_single, color="green", ls=":", label=f"Single thread ({t_single:.2f}s)")
ax.set_xscale("log", base=2); ax.set_yscale("log")
ax.set_xlabel("Number of threads"); ax.set_ylabel("Execution time (s)")
ax.set_title("Execution time vs number of threads"); ax.grid(True, which="both", alpha=.3)
mark_cores(ax); ax.legend(fontsize=8, loc="best")
save(fig, "approach3_time_vs_threads.png")

# ---- 2. speedup vs threads ----
fig, ax = plt.subplots(figsize=(7, 4.5))
for k, label, c in curves:
    ts, ys, _ = series(k)
    if ts:
        ax.plot(ts, [t_single / y for y in ys], marker="o", label=label, color=c)
ts_all = sorted({t for (m, t, kk) in mean if m == "shared"})
ax.plot(ts_all, ts_all, "k--", lw=1, label="Ideal (linear)")
ax.set_xscale("log", base=2); ax.set_yscale("log", base=2)
ax.set_xlabel("Number of threads"); ax.set_ylabel("Speedup = T_single / T_parallel")
ax.set_title("Speedup vs number of threads"); ax.grid(True, which="both", alpha=.3)
mark_cores(ax); ax.legend(fontsize=8, loc="upper left")
save(fig, "approach3_speedup_vs_threads.png")

# ---- 3. efficiency ----
fig, ax = plt.subplots(figsize=(7, 4.5))
for k, label, c in curves:
    ts, ys, _ = series(k)
    if ts:
        ax.plot(ts, [t_single / y / t for t, y in zip(ts, ys)], marker="o", label=label, color=c)
ax.set_xscale("log", base=2)
ax.set_xlabel("Number of threads"); ax.set_ylabel("Efficiency = speedup / threads")
ax.set_title("Parallel efficiency"); ax.grid(True, which="both", alpha=.3)
mark_cores(ax); ax.legend(fontsize=8, loc="upper right")
save(fig, "approach3_efficiency_vs_threads.png")

# ---- 4. experiment 6.1: synchronization frequency ----
# thread count used by the k sweep = the one that has runs for k=100
kt = sorted({t for (m, t, k) in mean if m == "shared" and (m, t, 100) in mean})
if kt:
    T = kt[0]
    ks_pos = sorted({k for (m, t, k) in mean if m == "shared" and k > 0 and t == T})
    pts = [(k, mean[("shared", T, k)], sd[("shared", T, k)]) for k in ks_pos]
    fig, ax = plt.subplots(figsize=(7, 4.5))
    ax.errorbar([p[0] for p in pts], [p[1] for p in pts], yerr=[p[2] for p in pts],
                marker="o", color="tab:red", capsize=3, label=f"{T} threads, shared counter")
    if ("shared", T, 0) in mean:
        ax.axhline(mean[("shared", T, 0)], color="tab:blue", ls="--", label="k=0 (flush once at end)")
    ax.axhline(t_single, color="green", ls=":", label="Single thread")
    ax.set_xscale("log"); ax.set_yscale("log")
    ax.set_xlabel("Sync interval k (update shared counter every k iterations)")
    ax.set_ylabel("Execution time (s)")
    ax.set_title("Experiment 6.1: synchronization frequency"); ax.grid(True, which="both", alpha=.3)
    ax.legend(fontsize=8)
    save(fig, "approach3_sync_frequency.png")

print(f"Wrote {RES / 'approach3_summary.csv'} and {FIG}/approach3_*.png")
