
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <errno.h>
#include <unistd.h>

#include "config.h"
#include "benchmark.h"

/* The ratio of the largest known output to input
 * e.g.: p_conv_f32 -> nr+nh-1 -> ratio 2
 */
static const size_t max_output = 3;

/* ifdef posix
 * this prototype is posix specific
 */

#if defined(HAVE_TIME_H)
#include <time.h>
#endif

#if defined(HAVE_E_LIB_H)
#include <e-lib.h>
#endif

typedef uint64_t platform_clock_t;

/* Arrays */
#define MAX_OUTPUTS 1 /* Points to same memory */
#define MAX_INPUTS 3
#define MAX_PARAMS (MAX_OUTPUTS + MAX_INPUTS)

/* Output args point to same mem, input args point to same mem */
#ifdef __epiphany__
volatile uint32_t *epiphany_done = (uint32_t *) 0x8f200000;
char *epiphany_stdout = (char *) 0x8f300000;
#define MAX_ELEMS 512
int8_t RAW_MEM[MAX_PARAMS * MAX_ELEMS * sizeof(uintmax_t)];
#define bench_printf(...)\
    do {\
        int __tmp;\
        __tmp = sprintf(epiphany_stdout, __VA_ARGS__);\
        if (__tmp > 0) {\
            epiphany_stdout = (char *) (((uintptr_t) epiphany_stdout) + \
                ((uintptr_t) __tmp));\
        }\
    } while(0)
#else
#define bench_printf printf
/* We don't want to be memory bound. So fit inside last-level cache (assume
 * 512kb). This might need tweaking, e.g., for x86_64 data will likely fit in
 * L1$ whereas on ARM (Cortex A9) probably not. */
#define MAX_ELEMS 16384
#endif

#if defined(HAVE_CLOCK_GETTIME)
static platform_clock_t platform_clock(void)
{
    struct timespec ts;
    uint64_t nanosec;

    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts);
    nanosec = (uint64_t) ts.tv_sec * 1000000000UL + (uint64_t) ts.tv_nsec;

    return nanosec;
}
#elif defined(HAVE_MACH_TIME)
#include <mach/mach_time.h>
static platform_clock_t platform_clock(void)
{
    static mach_timebase_info_data_t tb_info = {
        .numer = 0,
        .denom = 0,
    };
    uint64_t abs_time, nanosec;

    abs_time = mach_absolute_time();
    if (tb_info.denom == 0) {
        (void) mach_timebase_info(&tb_info);
    }
    nanosec = abs_time;
    nanosec /= tb_info.denom;
    nanosec *= tb_info.numer;

    return nanosec;
}
#elif defined(HAVE_E_LIB_H)
static platform_clock_t platform_clock(void)
{
    // Assuming 600MHz clock 10^9 / (600 * 10^6)
    const float factor = 1.6666666666666666666f;

    static platform_clock_t clock = 0;
    platform_clock_t diff;

    static bool initialized = false;
    uint64_t now;

    if (!initialized) {
        e_ctimer_stop(E_CTIMER_0);
        e_ctimer_set(E_CTIMER_0, E_CTIMER_MAX);
        e_ctimer_start(E_CTIMER_0, E_CTIMER_CLK);
        initialized = true;
        return 0;
    }

    /* Clock will drift here */
    diff = E_CTIMER_MAX - e_ctimer_get(E_CTIMER_0);

    e_ctimer_stop(E_CTIMER_0);
    e_ctimer_set(E_CTIMER_0, E_CTIMER_MAX);
    e_ctimer_start(E_CTIMER_0, E_CTIMER_CLK);

    clock += diff;

    return (platform_clock_t) ((float) clock * factor);
}
#else
#error "No timing function"
#endif

static void platform_print_duration(platform_clock_t start,
                                    platform_clock_t end)
{
    if (end < start)
        bench_printf("%" PRIu64, 0xffffffffffffffffUL);
    else
        bench_printf("%" PRIu64, end - start);
}

/* end of platform specific section */

struct item_data
{
    platform_clock_t start;
};

static void item_preface(struct item_data *, const struct p_bench_item *);
static void item_done(struct item_data *, const struct p_bench_specification *,
                      const char *);
static void setup_memory(struct p_bench_raw_memory *, char **raw, size_t);

int main(void)
{
    struct p_bench_specification spec;
    char *raw_mem = NULL;
    spec.current_size = MAX_ELEMS;

    setup_memory(&spec.mem, &raw_mem, spec.current_size);
    bench_printf(";name, size, duration (ns)\n");
    for (const struct p_bench_item *item = benchmark_items; item->name != NULL;
         ++item) {
        struct item_data data;

#ifndef __epiphany__
        /* Warm up cache. We don't want to be memory-bound. */
        item->benchmark(&spec);
#endif

        item_preface(&data, item);
        item->benchmark(&spec);
        item_done(&data, &spec, item->name);
    }
#ifdef __epiphany__
    *epiphany_done = 1;
#endif
    return EXIT_SUCCESS;
}

static void setup_output_pointers(struct p_bench_raw_memory *mem, void *p)
{
    /* Assume largest type is 64 bits */

    /* TODO: All pointers point to same memory region so output will be bogus */
    mem->o1.p_u64 = p;
    mem->o2.p_u64 = p;
    mem->o3.p_u64 = p;
    mem->o4.p_u64 = p;
}

static void setup_prandom_chars(char *p, size_t size, unsigned r,
                                bool skip_zero)
{
    /* It is probably not necessary, but this way the same prandom values
     * are used everytime, everywhere
     */

    while (size > 0) {
        r = 7559 * r + 5;
        /* not the best prng method in the universe, but good enough */

        if (skip_zero && ((char)r) == 0) {
            continue;
        }
        *p = (char)r;
        ++p;
        --size;
    }
}

static void setup_input_pointers(struct p_bench_raw_memory *mem, char *p,
                                 size_t size)
{
    unsigned seed = 0;

    /* Assume uint64_t is largest type */

    setup_prandom_chars(p, size * sizeof(uint64_t), seed, false);
    mem->i1_w.p_void = p;
    p += size * sizeof(uint64_t);

    setup_prandom_chars(p, size * sizeof(uint64_t), seed, false);
    mem->i2_w.p_void = p;
    p += size * sizeof(uint64_t);

    setup_prandom_chars(p, size * sizeof(uint64_t), seed, false);
    mem->i3_w.p_void = p;
    p += size * sizeof(uint64_t);

#if 0
    /* TODO: Do we really need 4 inputs? */
    setup_prandom_chars(p, size * sizeof(uint64_t), seed, false);
    mem->i4_w.p_void = p;
    p += size * sizeof(uint64_t);
#endif
}

static void setup_memory(struct p_bench_raw_memory *mem, char **raw,
                         size_t size)
{
    assert(mem != NULL);
    assert(size > 0);
    assert(raw != NULL);

    size_t raw_output_size = MAX_OUTPUTS * MAX_ELEMS * sizeof(uintmax_t);
    size_t raw_size =
        raw_output_size + MAX_INPUTS * MAX_ELEMS * (sizeof(uintmax_t));

#ifdef __epiphany__
    *raw = RAW_MEM;
#else
    if (*raw == NULL) {
        *raw = malloc(raw_size);
    } else {
        *raw = realloc(*raw, raw_size);
    }
    if (*raw == NULL) {
        (void)fprintf(stderr, "Unable to allocate memory: %zu\n", size);
        exit(EXIT_FAILURE);
    }
#endif

    setup_output_pointers(mem, *raw);
    setup_input_pointers(mem, *raw + raw_output_size, size);
}

static void item_preface(struct item_data *data,
                         const struct p_bench_item *item)
{
    data->start = platform_clock();
}

static void item_done(struct item_data *data,
                      const struct p_bench_specification *spec,
                      const char *name)
{
    assert(name != NULL);
    assert(name[0] != 0);

    platform_clock_t now = platform_clock();
    bench_printf("%s, %" PRIu64 ", ", name, spec->current_size);
    platform_print_duration(data->start, now);
    bench_printf("\n");
}
