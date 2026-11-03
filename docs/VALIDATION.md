# Validation Status

## Current implementation

The current source uses POSIX `rand_r` with private `unsigned int` state. Seeds are range-checked against `UINT_MAX`; point counts remain long long.

The source and test interfaces have been updated together. The Makefile references the top-level `tests` and `scripts` directories. Python files are checked for syntax, and Makefile commands and local Markdown links are checked for consistency.

## Numeric type checks

Counts now use signed long long. Syntax checks and isolated Windows checks passed for strtoll parsing at LLONG_MAX, overflow rejection, unsigned seed limits, large work partitions, %lld CSV formatting, and benchmark argument bounds. These numeric checks exclude RNG execution and do not constitute an Ubuntu benchmark.

## Execution limitation

The revised rand_r implementation has not been built or executed on Ubuntu by the assistant. The VMware Ubuntu VM was started, but the requested run was stopped before compilation or benchmarking. No runtime result is claimed for this version.

MinGW's standard-library header can define `rand_r` as a macro that calls global `rand()`. That is unsuitable for validating reproducible private RNG state. The code intentionally rejects MinGW builds instead of silently running a different generator.

On Ubuntu, run the following from the repository root:

```bash
make
make bench
```

When the ignored local `tests/` files are present, run `make check` before benchmarking. A Git-only checkout does not contain those files; the target reports that they are missing instead of claiming that checks passed.

Do not treat syntax checks as successful Ubuntu execution.

## Historical measurements

The files outside the repository under `LAB3_personal/archive/validation_windows` were produced by the previous PCG-based version on Windows. They are retained as history and are not current benchmark results. Their timing and hit counts must not be copied into a report for the rand_r implementation.

## Expected checks

- Reproducibility and isolation of interleaved private rand_r states.
- Points inside, outside, and on the circle boundary.
- Floating-point ratio calculation.
- Complete distribution of uneven sample counts.
- Invalid arguments, long long point parsing, and unsigned seed limits.
- Headerless CSV and a broad deterministic estimator sanity check.
- At least five measurements with independent recomputation of summary statistics.

The checks are provided for the Ubuntu run; successful runtime results must be recorded after they are actually executed.
