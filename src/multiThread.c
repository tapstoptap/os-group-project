#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

typedef struct {
    long long samples;
    long long circle_count;
    unsigned int seed; 
} thread_data_t;

void* monte_carlo_worker(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    long long local_circle_count = 0;
    unsigned int seed = data->seed;

    for (long long i = 0; i < data->samples; i++) {
        double x = ((double)rand_r(&seed) / RAND_MAX) * 2.0 - 1.0;
        double y = ((double)rand_r(&seed) / RAND_MAX) * 2.0 - 1.0;

        if (x * x + y * y <= 1.0) {
            local_circle_count++;
        }
    }

    data->circle_count = local_circle_count;
}


int main(int argc , char* argv [])
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <num-threads> <n>\n", argv[0]);
        return 1;
    }

    int num_threads = atoi(argv[1]);
    long long n = atoll(argv[2]);

    pthread_t threads[num_threads];
    thread_data_t thread_data[num_threads];

    long long base_samples = total_points / num_threads;
    long long remainder = total_points % num_threads;
    unsigned int base_seed = (unsigned int)time(NULL);
    struct timespec start_time, end_time;

    clock_gettime(CLOCK_MONOTONIC, &start_time);

    for (int i = 0; i < num_threads; i++) {
        thread_data[i].samples = base_samples + (i < remainder ? 1 : 0);
        thread_data[i].circle_count = 0;
        thread_data[i].seed = base_seed + (i * 1000) + 1;

        if (pthread_create(&threads[i], NULL, monte_carlo_worker, (void*)&thread_data[i]) != 0) {
            perror("pthread_create failed");
            return 1;
        }
    }

    long long total_circle_points = 0;
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
        total_circle_points += thread_data[i].circle_count;
    }

    clock_gettime(CLOCK_MONOTONIC, &end_time);

    double elapsed_time = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_nsec - start_time.tv_nsec) / 1e9;

    double estimated_pi = 4.0 * (double)total_circle_points / (double)total_points;

    printf("Estimated Pi : %.6f\n", estimated_pi);
    printf("Total Points : %lld\n", total_points);
    printf("Circle Hits  : %lld\n", total_circle_points);
    printf("Threads      : %d\n", num_threads);
    printf("Time Taken   : %.6f seconds\n", elapsed_time);

    return 0;
}