# Running the Baseline on Ubuntu

Run these commands in an Ubuntu terminal. The baseline needs GCC and Make; Python is only used by the optional checks and benchmark runner.

## 1 Install the tools

```bash
sudo apt update
sudo apt install build-essential python3 unzip
gcc --version
make --version
python3 --version
```

No third-party Python packages are required.

## 2 Open the source directory

If using the release archive, copy `os-week3-person1.zip` into Ubuntu's Downloads directory:

```bash
mkdir -p ~/os-week3
unzip ~/Downloads/os-week3-person1.zip -d ~/os-week3
cd ~/os-week3/LAB3
ls
```

You should see `Makefile`, `src`, `docs`, and `README.md`. Scripts, documentation, results, and figures are grouped in top-level directories. The local source archive may also include `tests/`; this directory is intentionally ignored by Git and is absent from a Git-only checkout.

If the source is already available from a checkout or shared folder, open the directory containing this baseline's Makefile. On Ubuntu WSL, this workspace may be available at:

```bash
cd /mnt/d/261/OS/LAB/LAB3
```

The `/mnt/d` path is specific to WSL. For VMware or VirtualBox, copy or extract the source into Ubuntu first.

## 3 Build

```bash
make
```

Equivalent GCC command:

```bash
gcc -D_POSIX_C_SOURCE=200809L -O2 -std=c11 -Wall -Wextra -Wpedantic \
    src/pi_single.c src/common.c -pthread -o pi_single
```

Use the same optimization settings for all approaches. The baseline does not create worker threads, but retains `-pthread` as a shared build convention. Build with a POSIX libc; MinGW's `rand_r` compatibility macro is not supported.

## 4 Run

```bash
./pi_single 1000000000 42
./pi_single --points 1000000000 --seed 42
./pi_single --help
```

The point count must be a positive decimal long long integer. The seed must fit `unsigned int`: normally 0 to 4294967295 on Ubuntu. With no arguments, the program uses one billion points and seed 42.

The output is one headerless CSV row:

```text
approach,threads,k,points,inside,pi,time_sec
```

The first three values are always `single,1,0`. The other fields contain the point count, circle-hit count, pi estimate, and elapsed computation time in seconds. The estimate is not expected to equal pi exactly.

## 5 Run the optional correctness checks

Run this step only if the local `tests/` files are available:

```bash
make check
```

Expect two `PASS` messages, covering shared helpers and CLI/integration behavior. Resolve failures before collecting performance data. If tests are absent, `make check` stops with an explanatory message. Building and benchmarking do not depend on those local test files.

To check invalid input:

```bash
./pi_single --points 0
echo $?
```

This should print an error and return a nonzero status. Seed zero is valid; a zero point count is not.

## 6 Record at least five measurements

```bash
make bench
```

For explicit settings:

```bash
python3 scripts/benchmark_baseline.py \
    --binary ./pi_single --points 1000000000 --seed 42 --runs 5
```

The script discards one warm-up, then records five runs with the same initial seed. A new directory under `results` contains:

| File | Contents |
|---|---|
| `raw.csv` | Individual measurements with seed and run number |
| `summary.csv` | Mean, sample standard deviation, minimum, maximum, and estimate |
| `machine.json` | Platform, CPU information where available, command, RNG, flags, and timing scope |

Use `mean_seconds` as `T_single`. The measured work is point generation, circle testing, and counting. Parsing, initialization, final ratio calculation, and printing are outside the interval.

Direct runs of `./pi_single` print to the terminal without saving files. Each benchmark invocation creates a new results directory and does not overwrite earlier measurements. Results are ignored by Git by default; force-add only the reviewed Ubuntu result directory if it needs to be published.

Build flags in metadata are declared rather than extracted from the binary. If you compile with different settings, pass the actual string using `--build-flags`.

## 7 Keep comparisons fair

Collect official results on the same Ubuntu machine. Close heavy background tasks and run configurations sequentially. Record:

```bash
uname -a
lscpu
nproc
gcc --version
```

For a VM, record its assigned vCPU count separately from host hardware. Keep total points, RNG, compiler settings, and timing scope consistent.

To record ten runs of the same one-billion-point workload:

```bash
python3 scripts/benchmark_baseline.py --points 1000000000 --seed 42 --runs 10
```

After changing the point count, remeasure all approaches in the comparison. Fixed-seed timing repetitions reuse samples; they do not generate independent accuracy observations.

## 8 Troubleshoot

| Symptom | Action |
|---|---|
| GCC or Make is missing | Install `build-essential` |
| No Makefile is found | Change into the repository root containing Makefile |
| `./pi_single` does not exist | Run `make`; do not use an old Windows executable |
| `make check` reports missing local tests | Restore the ignored `tests/` files if verification is needed; build and benchmark remain available |
| Make reports `missing separator` | Restore recipe tabs or use the GCC command above |
| `Invalid arguments` | Use decimal integers such as `1000000000`, not `1e9` |
| Seed is out of range | Use a value no larger than `UINT_MAX` |
| Benchmark time is zero | Increase the sample count |
| Another machine gives different hits | Check libc; `rand_r` sequences can differ across implementations |

## 9 Rebuild and share

```bash
make clean
make
```

If the local tests are available, also run `make check`. Cleaning removes generated executables, not measurements. Share source and reviewed Ubuntu measurements. Read `docs/HANDOFF.md` before integrating the parallel approaches.
