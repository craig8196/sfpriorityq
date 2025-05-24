/********************************************************************************
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
 * @file q.h
 * @author Craig Jacobson
 * @brief Timeout manager interface.
 */
#ifndef PRIORITYQ_H_
#define PRIORITYQ_H_
#ifdef __cplusplus
extern "C" {
#endif


#include <stdbool.h>
#include <stdint.h>


/* Internal node */
struct priorityq_node_s
{
    struct priorityq_node_s *prev;
    struct priorityq_node_s *next;
};

#define PRIORITY_URGENT ((uint8_t)128)

/* Priority Struct*/
typedef struct priority_s
{
    struct priorityq_node_s node;
    void *data;
    uint8_t info[4]; // Internal data and flags.
} priority_t;

void
priority_init(priority_t *);
void
priority_destroy(priority_t *);
void
priority_set(priority_t *, void *, uint8_t);
uint8_t
priority_value(priority_t *); // Well, I didn't want to call it priority_priority...
void *
priority_data(priority_t *);
bool
priority_is_active(priority_t *);


/* Priority Manager */
#define PQ_CEILING  (128)
#define PQ_BINS (8)
typedef struct
{
    // The priority counter is a rotating value using masking to simulate overflow.
    uint8_t pc;
    uint32_t counter_imed;
    uint32_t size;
    uint32_t size_done;
    uint32_t size_imed;
    uint32_t size_q;
    // Process these first, urgent items go here.
    struct priorityq_node_s done;
    // Process these next.
    struct priorityq_node_s immediate;
    // Work on moving priorities up.
    struct priorityq_node_s processing;
    struct priorityq_node_s bins[PQ_BINS];
} priorityq_t;

void
priorityq_init(priorityq_t *);
void
priorityq_destroy(priorityq_t *);
uint32_t
priorityq_size(priorityq_t *);

void
priorityq_enqueue(priorityq_t *, priority_t *);
priority_t *
priorityq_dequeue(priorityq_t *);
void
priorityq_remove(priorityq_t *, priority_t *);


/* Exports for testing. */
uint8_t
priorityq_priority_counter(priorityq_t *);
uint32_t
priorityq_count_bin(priorityq_t *, uint32_t);
uint32_t
priorityq_count_all(priorityq_t *);
uint32_t
priorityq_count_immediate(priorityq_t *);
uint32_t
priorityq_count_done(priorityq_t *);
uint32_t
priorityq_count_q(priorityq_t *);
uint32_t
priorityq_size_immediate(priorityq_t *);
uint32_t
priorityq_size_done(priorityq_t *);
uint32_t
priorityq_size_q(priorityq_t *);
uint32_t
priorityq_upper_bit(uint8_t n);
#ifdef DEBUG_OUTPUT_FUNCTIONS
void
priorityq_dump_stats(priorityq_t *);
void
priority_dump_stats(priority_t *p);
#endif


/* Useful macros. */
#ifndef LIKELY
#   ifdef __GNUC__
#       define LIKELY(x)   __builtin_expect(!!(x), 1)
#   else
#       define LIKELY(x) (x)
#   endif
#endif

#ifndef UNLIKELY
#   ifdef __GNUC__
#       define UNLIKELY(x) __builtin_expect(!!(x), 0)
#   else
#       define UNLIKELY(x) (x)
#   endif
#endif

#ifndef INLINE
#   ifdef __GNUC__
#       define INLINE __attribute__((always_inline)) inline
#   else
#       define INLINE
#   endif
#endif

#ifndef NOINLINE
#   ifdef __GNUC__
#       define NOINLINE __attribute__((noinline))
#   else
#       define NOINLINE
#   endif
#endif


#ifdef __cplusplus
}
#endif
#endif /* PRIORITYQ_H_ */

