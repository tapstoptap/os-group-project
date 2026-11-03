#define _POSIX_C_SOURCE 200809L
#include "common.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    mc_config config;
    int parsed = mc_parse_config(argc, argv, &config);
    if (parsed == 1) {
        mc_usage(argv[0]);
        return EXIT_SUCCESS;
    }
    if (parsed != 0) {
        fprintf(stderr, "Invalid arguments. Use --help for usage.\n");
        return EXIT_FAILURE;
    }

    /* Initialization is OUTSIDE the timed interval. No pthread is created. */
    mc_rng rng;
    mc_seed(&rng, config.seed);

    double start = now_sec();
    if (start < 0.0) {
        perror("clock_gettime start");
        return EXIT_FAILURE;
    }

    /* The measured work: generate points, test the circle, count hits. */
    long long inside = mc_count_inside(config.points, &rng);

    double finish = now_sec();
    if (finish < 0.0) {
        perror("clock_gettime finish");
        return EXIT_FAILURE;
    }
    if (finish < start) {
        fprintf(stderr, "Invalid monotonic clock interval.\n");
        return EXIT_FAILURE;
    }

    /* Formula conversion and output are OUTSIDE the timed interval. */
    print_result("single", 1, 0, config.points, inside, finish - start);
    return EXIT_SUCCESS;
}
