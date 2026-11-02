#ifndef MC_COMMON_H
#define MC_COMMON_H

#include <stdint.h>
#include <limits.h>

typedef struct {
    unsigned int seed;
} mc_rng;

/* The seed is also the private state updated by POSIX rand_r(). */
void mc_seed(mc_rng *rng, unsigned int seed);
double rand01(mc_rng *rng);

#define MC_DEFAULT_POINTS UINT64_C(10000000)
#define MC_DEFAULT_SEED 42U

typedef struct {
    uint64_t points;
    unsigned int seed;
} mc_config;

/* Future worker arguments: each worker owns one instance and its RNG. */
typedef struct {
    uint64_t points;
    mc_rng rng;
    uint64_t inside;
} mc_thread_data;

/* Return 0 on success, 1 for --help, -1 on invalid input. */
int mc_parse_config(int argc, char **argv, mc_config *config);
void mc_usage(const char *program);

/* Return monotonic wall-clock seconds, or -1 on clock failure. */
double now_sec(void);
static inline int mc_is_inside(double x, double y)
{
    return x * x + y * y <= 1.0;
}
uint64_t mc_count_inside(uint64_t points, mc_rng *rng);
double mc_estimate_pi(uint64_t inside, uint64_t points);

/* No division by zero: caller must supply threads > 0, id < threads. */
uint64_t mc_points_for_thread(uint64_t total, uint64_t threads, uint64_t id);

/* CSV contract: approach,threads,k,points,inside,pi,time_sec; no header. */
void print_result(const char *approach, uint64_t threads, uint64_t batch,
                  uint64_t points, uint64_t inside, double seconds);

#endif
