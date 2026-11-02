#define _POSIX_C_SOURCE 200809L
#include "common.h"

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* MinGW's rand_r can be a macro calling global rand(), not POSIX rand_r. */
#ifdef __MINGW32__
#error "Build this rand_r version on Ubuntu or another POSIX libc, not MinGW."
#endif

void mc_seed(mc_rng *rng, unsigned int seed)
{
    rng->seed = seed;
}

double rand01(mc_rng *rng)
{
    return (double)rand_r(&rng->seed) / (double)RAND_MAX;
}

static int parse_decimal(const char *text, uint64_t *value)
{
    if (text[0] == '\0')
        return -1;
    for (const char *p = text; *p != '\0'; ++p)
        if (*p < '0' || *p > '9')
            return -1;

    errno = 0;
    char *end;
    uintmax_t parsed = strtoumax(text, &end, 10);
    if (errno == ERANGE || *end != '\0' || parsed > UINT64_MAX)
        return -1;
    *value = (uint64_t)parsed;
    return 0;
}

void mc_usage(const char *program)
{
    printf("Usage: %s [--points M] [--seed S]\n"
           "       %s M S\n"
           "Defaults: M=10000000, S=42; M > 0; S >= 0.\n"
           "M: uint64 decimal; S: unsigned int decimal (max %u).\n"
           "This is Approach 1 (one main thread).\n"
           "CSV: approach,threads,k,points,inside,pi,time_sec\n",
           program, program, UINT_MAX);
}

int mc_parse_config(int argc, char **argv, mc_config *config)
{
    config->points = MC_DEFAULT_POINTS;
    config->seed = MC_DEFAULT_SEED;
    uint64_t parsed_seed = MC_DEFAULT_SEED;
    if (argc == 2 && strcmp(argv[1], "--help") == 0)
        return 1;

    /* Compatibility with the reference group's positional invocation. */
    if (argc == 3 && argv[1][0] != '-') {
        if (parse_decimal(argv[1], &config->points) != 0 ||
            parse_decimal(argv[2], &parsed_seed) != 0)
            return -1;
    } else {
        int seen_points = 0, seen_seed = 0;
        for (int i = 1; i < argc; i += 2) {
            if (i + 1 >= argc)
                return -1;
            if (strcmp(argv[i], "--points") == 0 && !seen_points) {
                seen_points = 1;
                if (parse_decimal(argv[i + 1], &config->points) != 0)
                    return -1;
            } else if (strcmp(argv[i], "--seed") == 0 && !seen_seed) {
                seen_seed = 1;
                if (parse_decimal(argv[i + 1], &parsed_seed) != 0)
                    return -1;
            } else {
                return -1;
            }
        }
    }
    if (config->points == 0 || parsed_seed > UINT_MAX)
        return -1;
    config->seed = (unsigned int)parsed_seed;
    return 0;
}

double now_sec(void)
{
    struct timespec timestamp;
    if (clock_gettime(CLOCK_MONOTONIC, &timestamp) != 0)
        return -1.0;
    return (double)timestamp.tv_sec + (double)timestamp.tv_nsec * 1e-9;
}

uint64_t mc_count_inside(uint64_t points, mc_rng *rng)
{
    uint64_t inside = 0;
    for (uint64_t i = 0; i < points; ++i) {
        double x = 2.0 * rand01(rng) - 1.0;
        double y = 2.0 * rand01(rng) - 1.0;
        if (mc_is_inside(x, y))
            ++inside;
    }
    return inside;
}

double mc_estimate_pi(uint64_t inside, uint64_t points)
{
    return 4.0 * (double)inside / (double)points;
}

uint64_t mc_points_for_thread(uint64_t total, uint64_t threads, uint64_t id)
{
    return total / threads + (id < total % threads ? UINT64_C(1) : UINT64_C(0));
}

void print_result(const char *approach, uint64_t threads, uint64_t batch,
                  uint64_t points, uint64_t inside, double seconds)
{
    printf("%s,%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%.12f,%.9f\n",
           approach, threads, batch, points, inside,
           mc_estimate_pi(inside, points), seconds);
}
