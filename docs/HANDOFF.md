# Integration Contract

## Scope

Person 1 implements assignment Sections 1, 2, and 3.1: the Monte Carlo model, single-thread baseline, and shared helpers. Parallel workers and the mutex implementation belong to Persons 2 and 3.

## Shared conventions

| Item | Convention |
|---|---|
| Coordinates | `x = 2.0 * rand01(&rng) - 1.0`; same for y; range `[-1,1]` |
| Hit predicate | `mc_is_inside(x,y)`, equivalent to `x*x + y*y <= 1.0` |
| RNG | POSIX `rand_r`, with a private `unsigned int` state |
| Baseline initialization | `mc_seed(&rng, base_seed)` |
| Future worker initialization | `mc_seed(&rng, base_seed + thread_id)`, after range checks |
| Work division | `mc_points_for_thread(M,p,id)`, requiring `p > 0` and `id < p` |
| Counters | `uint64_t` |
| CSV | `approach,threads,k,points,inside,pi,time_sec`; no header |
| Build settings | C11, `-O2 -Wall -Wextra -Wpedantic -pthread` |
| Timing | `now_sec()`; check for a negative failure result |

Before assigning worker seeds, reject configurations where `base_seed + (p - 1)` exceeds `UINT_MAX`. Perform this check without overflowing the addition, for example by comparing `p - 1` with `UINT_MAX - base_seed`. Different seeds do not by themselves prove statistical independence. Keep the seed convention identical in both parallel implementations.

Do not share an RNG object between workers. Do not mix `rand()`, another generator, or a different coordinate transformation into just one approach.

## Person 2

1. Allocate a separate `mc_thread_data` for each worker.
2. Assign points and initialize each private RNG before the measured calculation.
3. Have each worker call `mc_count_inside(data->points, &data->rng)`.
4. Write the worker's result once, then let main combine results after completion synchronization.
5. Create all workers before waiting for them; handle create/join failures without reporting incomplete work as a complete estimate.

The work splitter distributes the remainder among the first workers. For M=10 and p=3, the counts are 4, 3, and 3.

## Person 3

Reuse the same sample distribution and worker seeds as Person 2. Call `rand01` and `mc_is_inside` within the worker, then protect shared-counter updates with a mutex.

Do not replace the default mutex experiment with one final update per worker: that changes synchronization frequency. Batch updates are a separately labeled experiment under assignment Section 6.1. Flush the final partial batch.

For the same M, p, seed, libc, and sample mapping, local and mutex implementations should have the same hit count. With p=1 and worker seed equal to the baseline seed, the hit count should match the baseline. Changing p can change the samples, so equality with the baseline is not generally required.

## Measurement

The baseline measures sample generation, circle checks, and counting. Initialization and I/O are excluded.

For parallel compute-only measurements, coordinate worker readiness before starting the timer. Document any boundary synchronization and final reduction included in the measured interval. Keep these conventions consistent across approaches. If an additional end-to-end series includes thread creation and joining, label it separately.

The reference report includes create/join overhead. Do not combine its times with this baseline or reuse its results as the team's own measurements.

## Files and integration

Core code is in `src` at the repository root. Benchmark scripts are tracked in `scripts`. Verification files in `tests` are local only and intentionally ignored by Git; `make check` requires those files, while `make` and `make bench` do not. Invoke these targets through the root Makefile. The team report will be written separately by its assigned author.

Merge shared files deliberately rather than overwriting an existing repository's Makefile or headers. No push or merge is performed by this local handoff. The implementation uses libc's `rand_r` and no longer bundles an external RNG header.
