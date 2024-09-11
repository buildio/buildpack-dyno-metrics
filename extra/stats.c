#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>

void log_system_stats(const char *stats_format, ...) {
    const char *dyno_formation = getenv("DYNO_FORMATION");
    const char *dyno_uuid = getenv("DYNO_UUID");

    if (!dyno_formation) dyno_formation = "unknown_formation";
    if (!dyno_uuid) dyno_uuid = "unknown_dyno";

    va_list args;
    va_start(args, stats_format);

    printf("sample#source=%s sample#dyno=%s ", dyno_formation, dyno_uuid);
    vprintf(stats_format, args); // Print the formatted string with the passed arguments
    printf("\n");

    va_end(args);
}

/* ------------------------     Load Averages        ------------------------ */

#define SAMPLE_INTERVAL 20  // Interval in seconds
#define PERIOD_1M 60        // 1-minute period in seconds
#define PERIOD_5M 300       // 5-minute period in seconds
#define PERIOD_15M 900      // 15-minute period in seconds

double load_avg_1m = 0.0;
double load_avg_5m = 0.0;
double load_avg_15m = 0.0;
time_t last_sample_time = 0;
pid_t agent_pid;  // Store the agent's PID

// Function to count the number of runnable processes in the container
int count_runnable_tasks() {
    FILE *file = fopen("/sys/fs/cgroup/cgroup.procs", "r");
    if (!file) {
        perror("Error opening cgroup.procs");
        exit(1);
    }

    int runnable_tasks = 0;
    char line[256];
    pid_t pid;

    // Read each PID in the cgroup.procs file
    while (fgets(line, sizeof(line), file)) {
        sscanf(line, "%d", &pid);
        if (pid != agent_pid) {  // Exclude the agent's PID
            runnable_tasks++;
        }
    }

    fclose(file);
    return runnable_tasks;
}

// Function to calculate the exponential moving average (EMA)
double calculate_ema(double current_runnable_count, double previous_avg, int period, double interval_sec) {
    double expterm = exp(-interval_sec / period);
    return (1 - expterm) * current_runnable_count + expterm * previous_avg;
}

// Function to sample the number of runnable tasks and update the load averages
void sample_load_average() {
    time_t current_time = time(NULL);
    double interval_sec = difftime(current_time, last_sample_time);

    if (last_sample_time == 0) {
        interval_sec = SAMPLE_INTERVAL;  // Assume first interval as SAMPLE_INTERVAL
    }

    // Get the current number of runnable tasks
    int runnable_tasks = count_runnable_tasks();

    // Update load averages using EMA
    load_avg_1m = calculate_ema(runnable_tasks, load_avg_1m, PERIOD_1M, interval_sec);
    load_avg_5m = calculate_ema(runnable_tasks, load_avg_5m, PERIOD_5M, interval_sec);
    load_avg_15m = calculate_ema(runnable_tasks, load_avg_15m, PERIOD_15M, interval_sec);

    // Update the last sample time
    last_sample_time = current_time;

    // Print the load averages in Heroku-style format
    log_system_stats( "sample#load_avg_1m=%.2f sample#load_avg_5m=%.2f sample#load_avg_15m=%.2f",
                      load_avg_1m, load_avg_5m, load_avg_15m);
}

/* ------------------------     Process statistics   ------------------------ */

#define MAX_SAMPLES_1M (60 / SAMPLE_INTERVAL)  // Samples for 1-minute window
#define MAX_SAMPLES_5M (300 / SAMPLE_INTERVAL)  // Samples for 5-minute window
#define MAX_SAMPLES_15M (900 / SAMPLE_INTERVAL)  // Samples for 15-minute window

// Arrays to store CPU usage deltas
long long cpu_samples_1m[MAX_SAMPLES_1M] = {0};
long long cpu_samples_5m[MAX_SAMPLES_5M] = {0};
long long cpu_samples_15m[MAX_SAMPLES_15M] = {0};

int sample_count_1m = 0;
int sample_count_5m = 0;
int sample_count_15m = 0;

long long previous_cpu_usage = 0;

// Function to read CPU usage from /sys/fs/cgroup/cpu.stat
long long get_cpu_usage() {
    FILE *file = fopen("/sys/fs/cgroup/cpu.stat", "r");
    if (!file) {
        perror("Error opening cpu.stat");
        exit(1);
    }

    char line[256];
    long long usage_usec = 0;

    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "usage_usec", 10) == 0) {
            sscanf(line, "usage_usec %lld", &usage_usec);
            break;
        }
    }

    fclose(file);
    return usage_usec;
}

// Function to calculate the average from an array of samples
long long calculate_average(long long *samples, int count) {
    if (count == 0) {
        return 0;
    }

    long long sum = 0;
    for (int i = 0; i < count; i++) {
        sum += samples[i];
    }

    return sum / count;
}

// Function to sample CPU usage and calculate averages
void sample_cpu_usage(int interval_sec) {
    const char *dyno_formation = getenv("DYNO_FORMATION");
    const char *dyno_uuid = getenv("DYNO_UUID");
    long long current_cpu_usage = get_cpu_usage();
    long long cpu_usage_delta = 0;

    // If this is not the first sample, calculate the delta
    if (previous_cpu_usage > 0) {
        cpu_usage_delta = (current_cpu_usage - previous_cpu_usage) / interval_sec;
    }

    // Update the previous CPU usage for the next sample
    previous_cpu_usage = current_cpu_usage;

    // Add the delta to the samples arrays, scaled to the appropriate bucket size
    cpu_samples_1m[sample_count_1m % MAX_SAMPLES_1M] = cpu_usage_delta;
    cpu_samples_5m[sample_count_5m % MAX_SAMPLES_5M] = cpu_usage_delta;
    cpu_samples_15m[sample_count_15m % MAX_SAMPLES_15M] = cpu_usage_delta;

    // Increment the sample counters
    sample_count_1m++;
    sample_count_5m++;
    sample_count_15m++;

    // Calculate averages in milliseconds (divide microseconds by 1000)
    long long avg_cpu_1m = calculate_average(cpu_samples_1m, sample_count_1m > MAX_SAMPLES_1M ? MAX_SAMPLES_1M : sample_count_1m) / 1000;
    long long avg_cpu_5m = calculate_average(cpu_samples_5m, sample_count_5m > MAX_SAMPLES_5M ? MAX_SAMPLES_5M : sample_count_5m) / 1000;
    long long avg_cpu_15m = calculate_average(cpu_samples_15m, sample_count_15m > MAX_SAMPLES_15M ? MAX_SAMPLES_15M : sample_count_15m) / 1000;

    // Output the CPU averages in Heroku-style log line
    log_system_stats("sample#cpu_avg_1m=%lldms sample#cpu_avg_5m=%lldms sample#cpu_avg_15m=%lldms",
           avg_cpu_1m, avg_cpu_5m, avg_cpu_15m);
}

/* ------------------------     Memory statistics   ------------------------ */

// Function to read memory usage from a given file path
long long read_memory_usage(const char *filepath) {
    FILE *file = fopen(filepath, "r");
    if (!file) {
        perror("Error opening memory file");
        exit(1);
    }

    long long memory_usage = 0;
    fscanf(file, "%lld", &memory_usage);

    fclose(file);
    return memory_usage;
}

// Function to read memory statistics and output them in Heroku-style log
void sample_memory_usage() {
    // Get the source (DYNO_FORMATION) and dyno (DYNO_UUID) from environment variables
    const char *dyno_formation = getenv("DYNO_FORMATION");
    const char *dyno_uuid = getenv("DYNO_UUID");

    // Ensure that default values are provided if the environment variables are not set
    if (!dyno_formation) {
        dyno_formation = "unknown_formation";
    }
    if (!dyno_uuid) {
        dyno_uuid = "unknown_dyno";
    }

    // Get total memory usage from memory.current
    long long memory_total = read_memory_usage("/sys/fs/cgroup/memory.current");

    // For RSS (anon) and cache (file) memory, we will use memory.stat file
    long long memory_rss = 0;
    long long memory_cache = 0;
    long long memory_pgpgin = 0;
    long long memory_pgpgout = 0;

    FILE *file = fopen("/sys/fs/cgroup/memory.stat", "r");
    if (!file) {
        perror("Error opening memory.stat");
        exit(1);
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        // Look for "anon" for RSS, "file" for cache, and "pgpgin"/"pgpgout" for page statistics
        if (strncmp(line, "anon", 4) == 0) {
            sscanf(line, "anon %lld", &memory_rss);
        } else if (strncmp(line, "file", 4) == 0) {
            sscanf(line, "file %lld", &memory_cache);
        } else if (strncmp(line, "pgpgin", 6) == 0) {
            sscanf(line, "pgpgin %lld", &memory_pgpgin);
        } else if (strncmp(line, "pgpgout", 7) == 0) {
            sscanf(line, "pgpgout %lld", &memory_pgpgout);
        }
    }

    fclose(file);

    // Get swap memory usage from memory.swap.current
    long long memory_swap = read_memory_usage("/sys/fs/cgroup/memory.swap.current");

    // Get memory quota (max memory usage) from memory.max
    long long memory_quota = read_memory_usage("/sys/fs/cgroup/memory.max");

    // If the memory quota is set to "max", set it to a large number to represent no limit
    if (memory_quota == -1) {
        memory_quota = 999999999;  // Example large number for no limit
    }

    // Calculate total memory used (resident + cache + swap)
    long long memory_total_combined = memory_rss + memory_cache + memory_swap;

    // Convert all memory values from bytes to megabytes with 2 decimal places
    double memory_rss_mb = memory_rss / (1024.0 * 1024.0);
    double memory_cache_mb = memory_cache / (1024.0 * 1024.0);
    double memory_swap_mb = memory_swap / (1024.0 * 1024.0);
    double memory_total_combined_mb = memory_total_combined / (1024.0 * 1024.0);
    double memory_quota_mb = memory_quota / (1024.0 * 1024.0);

    // Output the memory statistics with source and dyno information
    log_system_stats(
           "sample#memory_total=%.2fMB sample#memory_rss=%.2fMB sample#memory_cache=%.2fMB "
           "sample#memory_swap=%.2fMB sample#memory_quota=%.2fMB sample#memory_pgpgin=%lld sample#memory_pgpgout=%lld",
           memory_total_combined_mb, memory_rss_mb, memory_cache_mb,
           memory_swap_mb, memory_quota_mb, memory_pgpgin, memory_pgpgout);
}

int main() {
    while (1) {
        sample_load_average();  // Report CPU load averages
        sample_cpu_usage(SAMPLE_INTERVAL);  // Report CPU load averages
        sample_memory_usage();  // Report memory usage
        sleep(SAMPLE_INTERVAL);  // Sleep for the specified interval (20 seconds)
    }

    return 0;
}

