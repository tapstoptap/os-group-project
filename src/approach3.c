/*
 * Approach 3 - Shared variable with mutex (Monte Carlo pi estimation)
 *
 * Build:  gcc -O2 -pthread -o approach3 approach3.c
 *
 * Usage:  ./approach3 -n <points> -t <threads> -k <sync_interval> [-s seed] [-m mode] [-H] [-v]
 *
 *   -n  total number of random points (default 20,000,000)
 *   -t  number of threads              (default 4)
 *   -k  synchronization interval:
 *         k = 1  : lock + update the shared counter EVERY iteration (pure Approach 3)
 *         k > 1  : keep a local counter, flush into the shared counter every k iterations
 *         k = 0  : flush only once at the end (behaves like Approach 2, local accumulation)
 *   -s  base random seed (default 12345) -> fixed seed for reproducibility
 *   -m  shared (default) | single
 *         single = single-thread baseline (no pthread, no mutex), used as T_single
 *   -H  print CSV header first
 *   -v  human readable output instead of CSV
 *
 * Output (CSV): mode,threads,k,n,pi,time_sec
 * Only the compute phase is timed (thread creation -> all joined). No I/O inside loops.
 */
#define _GNU_SOURCE
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

typedef struct {
    long long n;      /* points for this thread */
    uint64_t  seed;   /* independent seed for this thread */
    long long k;      /* sync interval */
} task_t;

static long long global_count = 0;                         /* shared counter */
static pthread_mutex_t count_lock = PTHREAD_MUTEX_INITIALIZER;

/* ---- independent per-thread RNG (no shared state, unlike rand()) ---- */
static inline uint64_t splitmix64(uint64_t *s) {
    uint64_t z = (*s += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

static inline uint64_t xorshift64star(uint64_t *s) {
    uint64_t x = *s;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    *s = x;
    return x * 2685821657736338717ULL;
}

static inline double rand01(uint64_t *s) {
    return (double)(xorshift64star(s) >> 11) * (1.0 / 9007199254740992.0); /* [0,1) */
}

static uint64_t make_seed(uint64_t base, int tid) {
    uint64_t s = base ^ ((uint64_t)(tid + 1) * 0xD1B54A32D192ED03ULL);
    uint64_t r = splitmix64(&s);
    return r ? r : 0x1234567ULL; /* xorshift state must be non-zero */
}

static double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

/* ---- worker used by Approach 3 ---- */
static void *worker(void *arg) {
    task_t *t = (task_t *)arg;
    uint64_t st = t->seed;
    const long long k = t->k;
    long long local = 0;
    long long since = 0;

    for (long long i = 0; i < t->n; i++) {
        double x = 2.0 * rand01(&st) - 1.0;
        double y = 2.0 * rand01(&st) - 1.0;
        int hit = (x * x + y * y <= 1.0);

        if (k == 1) {
            /* critical section on every iteration: maximum contention */
            pthread_mutex_lock(&count_lock);
            global_count += hit;
            pthread_mutex_unlock(&count_lock);
        } else {
            local += hit;
            if (k > 1 && ++since == k) {
                pthread_mutex_lock(&count_lock);
                global_count += local;
                pthread_mutex_unlock(&count_lock);
                local = 0;
                since = 0;
            }
        }
    }

    /* flush leftovers (k == 0, or n not divisible by k) */
    if (k != 1 && local > 0) {
        pthread_mutex_lock(&count_lock);
        global_count += local;
        pthread_mutex_unlock(&count_lock);
    }
    return NULL;
}

/* ---- single-thread baseline (Approach 1) ---- */
static long long run_single(long long n, uint64_t base_seed) {
    uint64_t st = make_seed(base_seed, 0);
    long long inside = 0;
    for (long long i = 0; i < n; i++) {
        double x = 2.0 * rand01(&st) - 1.0;
        double y = 2.0 * rand01(&st) - 1.0;
        inside += (x * x + y * y <= 1.0);
    }
    return inside;
}

static void usage(const char *p) {
    fprintf(stderr,
            "Usage: %s -n <points> -t <threads> -k <sync_interval> [-s seed] "
            "[-m shared|single] [-H] [-v]\n",
            p);
}

int main(int argc, char **argv) {
    long long n = 20000000LL;
    int threads = 4;
    long long k = 1;
    uint64_t seed = 12345ULL;
    int single = 0, header = 0, verbose = 0;
    int opt;

    while ((opt = getopt(argc, argv, "n:t:k:s:m:Hvh")) != -1) {
        switch (opt) {
        case 'n': n = atoll(optarg); break;
        case 't': threads = atoi(optarg); break;
        case 'k': k = atoll(optarg); break;
        case 's': seed = strtoull(optarg, NULL, 10); break;
        case 'm': single = (strcmp(optarg, "single") == 0); break;
        case 'H': header = 1; break;
        case 'v': verbose = 1; break;
        default: usage(argv[0]); return 1;
        }
    }
    if (n <= 0 || threads <= 0 || k < 0) {
        usage(argv[0]);
        return 1;
    }

    long long inside = 0;
    double t0, t1;

    if (single) {
        threads = 1;
        t0 = now_sec();
        inside = run_single(n, seed);
        t1 = now_sec();
    } else {
        pthread_t *tid = malloc(sizeof(pthread_t) * threads);
        task_t *tasks = malloc(sizeof(task_t) * threads);
        if (!tid || !tasks) { perror("malloc"); return 1; }

        long long base = n / threads, rem = n % threads;
        for (int i = 0; i < threads; i++) {
            tasks[i].n = base + (i < rem ? 1 : 0);
            tasks[i].seed = make_seed(seed, i);
            tasks[i].k = k;
        }
        global_count = 0;

        t0 = now_sec();
        for (int i = 0; i < threads; i++) {
            if (pthread_create(&tid[i], NULL, worker, &tasks[i]) != 0) {
                perror("pthread_create");
                return 1;
            }
        }
        for (int i = 0; i < threads; i++) pthread_join(tid[i], NULL);
        t1 = now_sec();

        inside = global_count;
        free(tid);
        free(tasks);
    }

    double pi = 4.0 * (double)inside / (double)n;
    double elapsed = t1 - t0;

    if (verbose) {
        printf("mode=%s threads=%d k=%lld n=%lld inside=%lld pi=%.6f time=%.4fs\n",
               single ? "single" : "shared", threads, k, n, inside, pi, elapsed);
    } else {
        if (header) printf("mode,threads,k,n,pi,time_sec\n");
        printf("%s,%d,%lld,%lld,%.6f,%.6f\n",
               single ? "single" : "shared", threads, k, n, pi, elapsed);
    }
    return 0;
}
