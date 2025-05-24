/*******************************************************************************
 * Copyright (c) 2025 Craig Jacobson
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 ******************************************************************************/
/**
 * @file complexity.c
 * @author Craig Jacobson
 * @brief Test the time complexity of the data structure.
 */
#include <assert.h>
#include <errno.h>
#include <float.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "priorityq.h"

#ifndef FORCESEED
#define FORCESEED (0)
#endif


typedef struct
{
    struct timespec start;
    struct timespec end;
} stopwatch_t;

void
stopwatch_reset(stopwatch_t *sw)
{
    sw->start = (struct timespec){ 0 };
    sw->end = (struct timespec){ 0 };
}

void
stopwatch_start(stopwatch_t *sw)
{
    if (clock_gettime(CLOCK_REALTIME, &sw->start))
    {
        printf("Error getting start time: %d, %s\n", errno, strerror(errno));
        abort();
    }
}

void
stopwatch_stop(stopwatch_t *sw)
{
    if (clock_gettime(CLOCK_REALTIME, &sw->end))
    {
        printf("Error getting end time: %d, %s\n", errno, strerror(errno));
        abort();
    }
}

double
stopwatch_elapsed(stopwatch_t *sw)
{
    double seconds =
        (double)(sw->end.tv_sec - sw->start.tv_sec)
        + ((double)sw->end.tv_nsec - (double)sw->start.tv_nsec)/1000000000.0;
    return seconds;
}

typedef struct
{
    uint32_t size;
    size_t alen;
    priority_t **ps;
} minheap_t;

void
minheap_init(minheap_t *h, int n)
{
    h->size = 0;
    h->alen = n;
    h->ps = (priority_t **)calloc(h->alen, sizeof(priority_t *));
}

void
minheap_precache(minheap_t *h)
{
    size_t i;
    for (i = 0; i < h->alen; ++i)
    {
        if (NULL != h->ps[i])
        {
            asm("");
        }
    }
}

void
minheap_realloc(minheap_t *)
{
    // Reallocation ignored for now.
    return;
}

void
minheap_nq(minheap_t *h, priority_t *p)
{
    if (h->size == h->alen) { minheap_realloc(h); }

    uint8_t priority = priority_value(p);
    uint32_t previndex = h->size;
    uint32_t nextindex = previndex ? (previndex - 1) >> 1 : 0;
    priority_t *next = NULL;

    while (previndex != nextindex)
    {
        next = h->ps[nextindex];
        if (priority_value(next) > priority)
        {
            h->ps[previndex] = next;
            previndex = nextindex;
            nextindex = previndex ? (previndex - 1) >> 1 : 0;
        }
        else
        {
            nextindex = previndex;
        }
    }

    h->ps[nextindex] = p;

    ++h->size;
}

priority_t *
minheap_dq(minheap_t *h)
{
    priority_t *p = NULL;

    if (h->size)
    {
        --h->size;
        p = h->ps[0];
        priority_t *last = h->ps[h->size];
        h->ps[h->size] = NULL;

        uint32_t rootindex = 0;
        uint32_t lastindex = h->size;
        while (rootindex != lastindex)
        {
            uint32_t leftindex = (rootindex + 1) << 1;
            uint32_t riteindex = leftindex + 1;
            if (riteindex < lastindex)
            {
                priority_t *left = h->ps[leftindex];
                priority_t *rite = h->ps[riteindex];
                if (priority_value(left) <= priority_value(rite))
                {
                    h->ps[rootindex] = left;
                    rootindex = leftindex;
                }
                else
                {
                    h->ps[rootindex] = rite;
                    rootindex = riteindex;
                }
            }
            else
            {
                if (leftindex >= lastindex)
                {
                    lastindex = rootindex;
                }
                else
                {
                    h->ps[rootindex] = h->ps[leftindex];
                    rootindex = leftindex;
                    lastindex = rootindex;
                }
            }
        }

        h->ps[lastindex] = last;
    }

    return p;
}

void
minheap_destroy(minheap_t *h)
{
    free(h->ps);
}

uint8_t
random8(void)
{
    return 127 & ((uint32_t)random());
}

int
random_seed(void)
{
    int seed = FORCESEED;
    if (!seed)
    {
        seed = (int)time(0);
    }
    srand(seed);
    return seed;
}

priority_t *
random_priorities(int n)
{
    priority_t *ps = (priority_t *)malloc(n * sizeof(priority_t));
    int i;
    for (i = 0; i < n; ++i)
    {
        priority_init(ps + i);
        priority_set(ps + i, NULL, random8());
    }
    return ps;
}

int
get_iterations(int argc, const char *argv[])
{
    int iterations = 128;
    if (argc >= 3)
    {
        iterations = atoi(argv[2]);
    }
    return iterations;
}

int
get_n(int argc, const char *argv[])
{
    int n = 10;
    if (argc >= 2)
    {
        n = atoi(argv[1]);
    }
    return n;
}

double
time_loop(int outer, int inner)
{
    stopwatch_t sw;
    stopwatch_start(&sw);
    int o, i;
    for (o = 0; o < outer; ++o)
    {
        for (i = 0; i < inner; ++i) asm("");
        for (i = 0; i < inner; ++i) asm("");
    }
    stopwatch_stop(&sw);
    return stopwatch_elapsed(&sw);
}

double
time_min_heap(minheap_t *heap, int iterations, int n, priority_t *ps)
{
    stopwatch_t sw;
    stopwatch_start(&sw);
    int iter, index;
    for (iter = 0; iter < iterations; ++iter)
    {
        for (index = 0; index < n; ++index)
        {
            priority_t *p = ps + index;
            minheap_nq(heap, p);
        }

        for (index = 0; index < n; ++index)
        {
            minheap_dq(heap);
        }
    }
    stopwatch_stop(&sw);

    return stopwatch_elapsed(&sw);
}

double
time_priorityq(priorityq_t *q, int iterations, int n, priority_t *ps)
{
    stopwatch_t sw;
    stopwatch_start(&sw);
    int iter, index;
    for (iter = 0; iter < iterations; ++iter)
    {
        for (index = 0; index < n; ++index)
        {
            priority_t *p = ps + index;
            priorityq_enqueue(q, p);
        }

        for (index = 0; index < n; ++index)
        {
            priorityq_dequeue(q);
        }
    }
    stopwatch_stop(&sw);

    return stopwatch_elapsed(&sw);
}

void
print_results(const char *name, double rawtime, double overhead, int iterations, int n)
{
    double total = rawtime - overhead;
    double avgiter = total / (double)iterations;
    double avgn = avgiter / (double)n;
    printf("{name:\"%s\","
            "raw_time: %.*g,"
            "overhead_time: %.*g,"
            "total_time: %.*g,"
            "average_iteration_time: %.*g,"
            "average_n_time: %.*g,"
            "iterations: %d,"
            "n: %d}\n",
            name,
            DBL_DECIMAL_DIG, rawtime,
            DBL_DECIMAL_DIG, overhead,
            DBL_DECIMAL_DIG, total,
            DBL_DECIMAL_DIG, avgiter,
            DBL_DECIMAL_DIG, avgn,
            iterations,
            n);
#if 0
    printf("{name:\"%s\","
            "raw_time: %lf,"
            "overhead_time: %lf,"
            "total_time: %lf,"
            "average_iteration_time: %lf,"
            "average_n_time: %lf,"
            "iterations: %d,"
            "n: %d}\n",
            name,
            rawtime,
            overhead,
            total,
            avgiter,
            avgn,
            iterations,
            n);
#endif
}

int
main(int argc, const char *argv[])
{
    minheap_t _heap;
    minheap_t *h = &_heap;
    priorityq_t _q;
    priorityq_t *q = &_q;

    int seed = random_seed();
    printf("Seed: %d\n", seed);

    int iterations = get_iterations(argc, argv);
    int n = get_n(argc, argv);

    minheap_init(h, n);
    minheap_precache(h);
    priorityq_init(q);

    priority_t *ps = random_priorities(n);

    double tloop = time_loop(iterations, n);
    double theap = time_min_heap(h, iterations, n, ps);
    double tq = time_priorityq(q, iterations, n, ps);

    print_results("min__heap", theap, tloop, iterations, n);
    print_results("priorityq", tq, tloop, iterations, n);

    priorityq_destroy(q);
    minheap_destroy(h);
    free(ps);

    return 0;
}
