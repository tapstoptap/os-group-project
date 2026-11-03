#ifndef MC_COMMON_H
#define MC_COMMON_H

#include <limits.h>

typedef struct {
    unsigned int seed;
} mc_rng;

/* The seed is also the private state updated by POSIX rand_r(). */
void mc_seed(mc_rng *rng, unsigned int seed);
double rand01(mc_rng *rng);

#define MC_DEFAULT_POINTS 1000000000LL
#define MC_DEFAULT_SEED 42U

typedef struct {
    long long points;
    unsigned int seed;
} mc_config;

/* Future worker arguments: each worker owns one instance and its RNG. */
typedef struct {
    long long points;
    mc_rng rng;
    long long inside;
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
long long mc_count_inside(long long points, mc_rng *rng);
double mc_estimate_pi(long long inside, long long points);

/* No division by zero: caller must supply total >= 0, threads > 0, 0 <= id < threads. */
long long mc_points_for_thread(long long total, long long threads, long long id);

/* CSV contract: approach,threads,k,points,inside,pi,time_sec; no header. */
void print_result(const char *approach, long long threads, long long batch,
                  long long points, long long inside, double seconds);

#endif
