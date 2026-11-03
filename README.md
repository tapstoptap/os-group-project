# Monte Carlo Pi Estimation

Single-thread baseline for the Week 3 Operating Systems lab, covering **Section 1 — Objectives**, **Section 2 — Problem Description**, and **Section 3.1 — Approach 1**.

The program estimates pi by sampling points uniformly in a square and counting those inside its inscribed unit circle. It provides the reference computation time for the team's parallel implementations.

## Requirements

- Ubuntu or a compatible POSIX environment with `rand_r` and `clock_gettime`.
- GCC with C11 support and Make.
- Python 3 for optional checks and benchmarks; no third-party Python packages.

```bash
sudo apt update
sudo apt install build-essential python3
```

Run the following commands from this directory, which contains the Makefile.

## Quick start

```bash
make
./pi_single --points 1000000000 --seed 42
```

The default configuration uses one billion points and seed 42. Only the main thread performs the calculation; no worker threads are created. See [Running on Ubuntu](docs/GUIDE.md) for the complete walkthrough.

## Method

Each sample has coordinates in `[-1, 1]`. A point is counted when `x*x + y*y <= 1.0`. Since the unit circle has area pi and the square has area 4:

```text
pi_estimate = 4.0 * points_inside / total_points
```

Both counters use `long long`; the final ratio uses floating-point arithmetic. A larger sample count reduces typical statistical error, but does not guarantee that every estimate becomes closer to pi.

## Usage

```bash
./pi_single
./pi_single 1000000000 42
./pi_single --points 1000000000 --seed 42
./pi_single --help
```

| Argument | Default | Accepted values |
|---|---|---|
| `--points` | `1000000000` | Decimal integers from 1 to `9223372036854775807` |
| `--seed` | `42` | Decimal integers from 0 to `UINT_MAX`, normally `4294967295` on Ubuntu |

Invalid input produces an error on stderr and a nonzero exit status without a result row. Signs, whitespace, scientific notation, duplicate options, and unsupported options are rejected.

## Output

A successful run writes one CSV row without a header:

```text
approach,threads,k,points,inside,pi,time_sec
```

| Field | Meaning |
|---|---|
| `approach` | `single` |
| `threads` | `1` |
| `k` | `0`; batching is not used |
| `points` | Total number of sampled points |
| `inside` | Number of points inside or on the circle |
| `pi` | Estimated value of pi |
| `time_sec` | Elapsed computation time in seconds |

## Timing and reproducibility

The timed interval includes point generation, the circle test, and local counting. Argument parsing, seed initialization, final ratio calculation, and output are excluded. Timing uses `clock_gettime(CLOCK_MONOTONIC)`.

POSIX `rand_r` updates a private `unsigned int` seed. Reusing the same initial seed reproduces samples within the same libc implementation. Identical sequences are not guaranteed across different libc implementations. Each future worker must own a separate seed.

## Optional checks and benchmarks

Run the baseline benchmark from the repository root:

```bash
make bench
```

Local verification files in `tests/` are intentionally excluded from Git. If those files are available on your machine, run `make check` before benchmarking. A Git-only checkout can build and benchmark without them; `make check` reports a clear error if the local test files are missing.

The benchmark discards one warm-up and records five runs. It creates a timestamped directory under `results` containing:

- `raw.csv`: individual measurements, run numbers, and seeds.
- `summary.csv`: mean, sample standard deviation, minimum, maximum, and estimate.
- `machine.json`: environment, command, RNG, declared build flags, and timing scope.

Running `./pi_single` directly only prints to the terminal; it does not save files. Each benchmark invocation creates a separate results directory. Generated results are ignored by Git by default. Force-add only the reviewed Ubuntu measurement directory when preparing the team's official results; leave trial and historical measurements untracked.

Use `mean_seconds` as `T_single`. Compare approaches using the same machine, point count, RNG, compiler settings, and timing convention. Fixed-seed repetitions assess timing variability, not independent accuracy samples.

## Repository layout

```text
os-group-project/
├── src/
│   ├── common.h                  Shared types and declarations
│   ├── common.c                  Parsing, rand_r, geometry, timer, and CSV
│   └── pi_single.c               Approach 1 entry point
├── scripts/
│   └── benchmark_baseline.py     Repeated baseline measurements
├── tests/                        Local only; excluded from Git
│   ├── check_common.c            Shared-helper checks
│   └── verify_baseline.py        CLI and integration checks
├── docs/
│   ├── GUIDE.md                  Build and execution guide
│   ├── HANDOFF.md                Contract for the parallel approaches
│   └── VALIDATION.md             Current verification status
├── results/                      Generated measurements
├── figures/                      Reserved for the team's measured plots
├── .gitattributes
├── .gitignore
├── Makefile
└── README.md
```

This follows the reference project's top-level separation of source, scripts, documentation, measurements, and figures. The tree also shows the optional local tests, which are not part of a Git-only checkout. Approach 2 and 3 source files will be added by their owners. The team report will be prepared separately by its assigned author.

Personal learning notes and historical Windows measurements are stored outside this repository in the sibling `LAB3_personal` directory and are excluded from the source release.

## Documentation

- [Ubuntu guide](docs/GUIDE.md)
- [Integration handoff](docs/HANDOFF.md)
- [Validation status](docs/VALIDATION.md)

## Validation status

The current `rand_r` implementation has not been executed on Ubuntu in the authoring environment. Earlier Windows measurements used a different RNG and are archived separately. MinGW builds are intentionally rejected because its `rand_r` compatibility macro may call global `rand()`.

## References

The assignment PDF and team work allocation define the scope. The [reference project](https://github.com/PTAxHVA/os-week3-monte-carlo-pi/tree/1693779e4ac4698e3ccf36c109747e04d8b7e709) and supplied report informed the CSV layout and baseline organization. This implementation samples the full circle in `[-1,1]²`; the reference samples a quarter circle in `[0,1]²`. Both have area ratio pi/4, but their sample results are not expected to match. Reference measurements are not presented as this project's results.
