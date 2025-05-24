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
 * @file q.c
 * @author Craig Jacobson
 * @brief Lazy, starvation-free priority queue implementation.
 */

#include "priorityq.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


/*******************************************************************************
 * Helper Functions
*******************************************************************************/

/**
 * @warn Do NOT input zero.
 * @return The index of the highest set bit.
 */
INLINE static int
get_high_index32(uint32_t n)
{
    return 31 - __builtin_clz(n);
}


/*******************************************************************************
 * Pointer Conversion Functions
*******************************************************************************/

#ifndef recover_ptr
#define recover_ptr(p, type, field) \
    ((type *)((char *)(p) - offsetof(type, field)))
#endif

INLINE static struct priorityq_node_s *
to_node(priority_t *p)
{
    return &p->node;
}

INLINE static priority_t *
to_priority(struct priorityq_node_s *p)
{
    return recover_ptr(p, priority_t, node);
}


/*******************************************************************************
 * Node Functions
*******************************************************************************/

INLINE static bool
node_in_list(struct priorityq_node_s *n)
{
    return !!n->next;
}

INLINE static void
node_clear(struct priorityq_node_s *n)
{
    (*n) = (const struct priorityq_node_s){ 0 };
}

INLINE static void
node_unlink_only(struct priorityq_node_s *n)
{
    n->next->prev = n->prev;
    n->prev->next = n->next;
}

INLINE static void
node_unlink(struct priorityq_node_s *n)
{
    node_unlink_only(n);
    node_clear(n);
}


/*******************************************************************************
 * List Functions
*******************************************************************************/

/**
 * You MUST be sure the list isn't empty!!!
 */
INLINE static struct priorityq_node_s *
list_dq_quick(struct priorityq_node_s *l)
{
    struct priorityq_node_s *n = l->next;
    node_unlink_only(n);
    return n;
}

INLINE static struct priorityq_node_s *
list_dq(struct priorityq_node_s *l)
{
    struct priorityq_node_s *n = NULL;

    if (l != l->next)
    {
        n = l->next;
        node_unlink(n);
    }

    return n;
}

INLINE static void
list_nq(struct priorityq_node_s *l, struct priorityq_node_s *n)
{
    n->next = l;
    n->prev = l->prev;
    l->prev->next = n;
    l->prev = n;
}

INLINE static bool
list_has(struct priorityq_node_s *l)
{
    return (l != l->next);
}

INLINE static void
list_clear(struct priorityq_node_s *l)
{
    l->next = l;
    l->prev = l;
}

INLINE static void
lists_clear(struct priorityq_node_s *l, size_t num)
{
    size_t i;
    for (i = 0; i < num; ++i)
    {
        list_clear(l + i);
    }
}

/**
 * @brief Append items from l2 to l1.
 */
INLINE static void
list_append(struct priorityq_node_s *l1, struct priorityq_node_s *l2)
{
    if (list_has(l2))
    {
        l2->next->prev = l1->prev;
        l2->prev->next = l1;
        l1->prev->next = l2->next;
        l1->prev = l2->prev;
        list_clear(l2);
    }
}

INLINE static int
list_count(struct priorityq_node_s *l)
{
    int count = 0;
    struct priorityq_node_s *next = l->next;

    while (next != l)
    {
        ++count;
        next = next->next;
    }

    return count;
}


/*******************************************************************************
 * Priority Functions
*******************************************************************************/

#define PQ_MASK (PQ_CEILING - 1)

// The original priority that an item was given (external).
#define PRIORITY_ABS (0)
// Relative to the current queue priority counter (internal).
#define PRIORITY_REL (1)
// The current queue an item is in.
#define PRIORITY_LOC (2)
// The urgent field.
#define PRIORITY_URG (3)

void
priority_init(priority_t *p)
{
    (*p) = (const priority_t){ 0 };
}

void
priority_destroy(priority_t *p)
{
    (*p) = (const priority_t){ 0 };
}

void
priority_set(priority_t *p, void *data, uint8_t priority)
{
    p->data = data;
    if (LIKELY(PRIORITY_URGENT != priority))
    {
        p->info[PRIORITY_ABS] = priority;
        p->info[PRIORITY_URG] = 0;
    }
    else
    {
        p->info[PRIORITY_ABS] = 0;
        p->info[PRIORITY_URG] = 1;
    }
}

uint8_t
priority_value(priority_t *p)
{
    return p->info[PRIORITY_ABS];
}

void *
priority_data(priority_t *p)
{
    return p->data;
}

bool
priority_is_active(priority_t *p)
{
    return node_in_list(to_node(p));
}


/*******************************************************************************
 * Priority Queue Functions (Internal)
*******************************************************************************/

enum PRIORITY_LOC_ENUM
{
    PRIORITY_LOC_NONE = 0,
    PRIORITY_LOC_DONE = 1,
    PRIORITY_LOC_IMED = 2,
    PRIORITY_LOC_Q    = 3,
};

/**
 * The priority MUST be in a queue in this data structure!!!
 */
INLINE static void
priorityq_decrement_queue(priorityq_t *q, priority_t *p)
{
    int loc = p->info[PRIORITY_LOC];
    if (PRIORITY_LOC_DONE == loc)
    {
        --q->size_done;
    }
    else if (PRIORITY_LOC_IMED == loc)
    {
        --q->size_imed;
    }
    else // PRIORITY_LOC_Q == loc
    {
        --q->size_q;
    }
}

INLINE static void
priorityq_nq_only(priorityq_t *q, priority_t *p)
{
    // This block adds overflow detection to better distribute priorities.
    // Without this the priorities crossing the overflow boundary all
    // get placed in the last bucket.
    // While that behavior doesn't hurst overall complexity, it can cause
    // a bit of a pause when advancing the queue and the urgent and immediate
    // queues are empty.
    // WARNING: This only really works in conjunction with the changes
    // in priorityq_advance_priority_counter!!!
    //
    // pc = priority counter
    uint8_t pc = q->pc;
    // rp = relative priority of p
    uint8_t rp = p->info[PRIORITY_REL];
    // nrp = rp less one
    uint8_t nrp = rp - 1;
    // The condition is that the upper bit of pc is a '1' and rp is a '0'.
    // Also, rp CANNOT be zero.
    // This doesn't work because rp can be zero: !((~PQ_MASK) & (pc & ~rp))
    int index;
    if (nrp >= pc)
    {
        // Not an overflow situation.
        // Original code (fall back to just this if this doesn't work).
        //
        // HOW THIS WORKS:
        // Since a priority is greater than the priority counter
        // we have a know fact that must be true:
        // the priority's leading differing bit is a '1', and the counter's
        // in the same position is a '0'.
        // When the counter flips that bit, we do so precisely.
        // This means that eventually that bit is the same:
        // pc = nnnn 1zzz
        // rp = nnnn 1mmm
        // So the n's are identical and the bits to the right in the counter
        // are guaranteed to be zero.
        // This means that we can just count the '1' bits to the right of
        // a differing bit of the relative priority to know how many different
        // bins it will be sorted into over the course of it's life.
        // If z = m, then we're to be prioritized and sent out of the queue.
        // If z != m, then the next bin is the next differing bit.
        // Because of the progression of the counter, we know that it is
        // less than or equal to any active priority.
        // This is how we progress priorities up through the queues.
        index = get_high_index32(rp ^ pc);
    }
    else
    {
        // We are wrapping around! This is the overflow situation.
        //
        // HOW THIS WORKS:
        // Preconditions: pc > rp, rp != 0
        // pc = 1nnn zmmm
        // rp = 0xxx yyyy
        // Let z be the FIRST zero somewhere after the leading '1' (or no zeros).
        // n are the bits to the left of z.
        // m are bits we don't care about.
        // y are bits we don't care about to be masked out by zmmm.
        // By repeated ANDing and bit shifting we turn everything to zeros
        // after the FIRST zero z.
        // This gives ones left of z and zeros to the right:
        // pc = 1111 z000
        // The bin to place p into is then the highest bit in the x's in rp
        // that overlaps the ones in pc.
        // This ONLY WORKS because we don't trigger bins corresponding to bits
        // flipping from '1' to '0', just bits flipping from '0' to '1'.
        // With the exception of the leading bit so we can take advantage of
        // overflow to keep this queue going indefinitely.
        //
        // apc = AND'ed priority counter
        uint8_t apc = pc & (pc >> 1);
        apc &= apc >> 2;
        apc &= apc >> 4;
        index = get_high_index32(rp & pc);
    }
    list_nq(q->bins + index, to_node(p));
}

INLINE static void
priorityq_advance_priority_counter(priorityq_t *q)
{
    uint8_t pc = q->pc;

    int index = 0;
    uint8_t msb = 1;
    while ((index < (PQ_BINS - 1)) && !(list_has((q->bins) + index) && (~pc & msb)))
    {
        ++index;
        msb <<= 1;
    }

    uint8_t newpc = (pc | (msb - 1)) + 1;

    // No matter how we advance, this bin is getting triggered.
    // DO NOT move directly to the immediate queue.
    list_append(&q->processing, q->bins + index);
    ++index;

    // WARNING: The following line of code are the changes referenced in
    // priorityq_nq!!!!
    // Since relative priorities are always greater than the priority counter,
    // the leading bit of ANY relative priority is set to 1 and the counter to 0.
    // The EXCEPTION is when we have wrap around, the leading bit can be a 1
    // in the counter and a zero in anything else.
    // Given eight bits: zxxx xxxx, we can ignore 1 -> 0 transitions in x's
    // as the counter's progression will NEVER trigger a bin; the z bit is the
    // exception.
    // Previous code (also works, just trying to reduce pointer checks):
    // uint8_t bits = (newpc ^ q->pc) >> 1;
    //
    // index tells us the first bin to check, so we should start there.
    //index = 1;
    uint8_t bits = ((~PQ_MASK & (pc ^ newpc)) | (PQ_MASK & (~pc & newpc)));
    bits >>= index;
    while (bits)
    {
        if (1 & bits)
        {
            list_append(&q->processing, q->bins + index);
        }
        ++index;
        bits >>= 1;
    }

    q->pc = newpc;
}

INLINE static void
priorityq_advance_immediates(priorityq_t *q)
{
    if (q->size_imed)
    {
        if (q->counter_imed)
        {
            list_nq(&q->done, list_dq_quick(&q->immediate));
            --q->size_imed;
            ++q->size_done;
            if (q->size_done < q->size_imed)
            {
                switch (q->size_imed & 1)
                {
                    case 0:
                        // Sometimes we add a second item.
                        // But make up for it by dividing by 2.
                        list_nq(&q->done, list_dq_quick(&q->immediate));
                        --q->size_imed;
                        ++q->size_done;
                        q->counter_imed >>= 1;
                        break;
                    case 1:
                        // Sometimes we just add one item.
                        --q->counter_imed;
                        break;
                }
            }
            else
            {
                // Sometimes we significantly reduce the counter.
                q->counter_imed >>= 2;
            }
        }
        else
        {
            // Sometimes we do no work in moving immediates to done.
            q->counter_imed = get_high_index32(q->size_imed) + 1;
        }
    }
}

INLINE static void
priorityq_advance_priority_queue(priorityq_t *q)
{
    if (q->size_q)
    {
        if (list_has(&q->processing))
        {
            // Advance priority queue.
            uint32_t limit_q = get_high_index32(q->size_q) + 1;
            do
            {
                priority_t *p = to_priority(list_dq_quick(&q->processing));
                if (LIKELY(p->info[PRIORITY_REL] != q->pc))
                {
                    priorityq_nq_only(q, p);
                }
                else
                {
                    p->info[PRIORITY_LOC] = PRIORITY_LOC_IMED;
                    --q->size_q;
                    ++q->size_imed;
                    list_nq(&q->immediate, to_node(p));
                }
            } while (--limit_q && list_has(&q->processing));
        }
        else
        {
            // Advance priority counter.
            priorityq_advance_priority_counter(q);
        }
    }
}


/*******************************************************************************
 * Priority Queue Functions
*******************************************************************************/

void
priorityq_init(priorityq_t *q)
{
    (*q) = (const priorityq_t){ 0 };
    list_clear(&q->done);
    list_clear(&q->immediate);
    list_clear(&q->processing);
    lists_clear(q->bins, PQ_BINS);
}

void
priorityq_destroy(priorityq_t *q)
{
    (*q) = (const priorityq_t){ 0 };
}

/**
 * @return The number of priorities in the data structure.
 */
uint32_t
priorityq_size(priorityq_t *q)
{
    return q->size;
}

/**
 * @brief Add the priority to the manager.
 *        Respects reprioritization upwards, but NOT downwards.
 * @warn Remember to maintain referential stability! 'priority_t' is a node internally!
 *       If you're using stack values, they cannot go out of scope.
 *       Move values to the heap prior to calling, not after.
 * @param p - The priority to add.
 *
 * If the priority appears to be a part of a list already, reprioritize.
 * If the priority is urgent, add to final queue.
 * If the priority is zero, add to immediate queue.
 * Otherwise, add priority to the internal data structure for future processing.
 */
void
priorityq_enqueue(priorityq_t *q, priority_t *p)
{
    if (UNLIKELY(p->info[PRIORITY_LOC] == PRIORITY_LOC_DONE)) { return; }

    // Checking in queue should be logical equiv to LOC != NONE.
    if (UNLIKELY(node_in_list(to_node(p))))
    {
        // If the new priority is less than the existing priority we must unlink.
        // Otherwise, we leave it in the queue.
        if (p->info[PRIORITY_URG])
        {
            // If the new priority is urgent, put into done q.
            node_unlink_only(to_node(p));
            list_nq(&q->done, to_node(p));
            // Can only be in immediate or regular queue
            if (PRIORITY_LOC_IMED == p->info[PRIORITY_LOC])
            {
                --q->size_imed;
            }
            else
            {
                --q->size_q;
            }
            ++q->size_done;
            p->info[PRIORITY_LOC] = PRIORITY_LOC_DONE;
            return;
        }
        else if (p->info[PRIORITY_LOC] == PRIORITY_LOC_IMED ||
                 p->info[PRIORITY_ABS] >= (p->info[PRIORITY_REL] - (uint8_t)q->pc))
        {
            // Either we're on our way out or the new priority is greater/eq
            // than our current location in the q.
            // Note that if the node is in processing and the absolute
            // priority is equal to the relative priority, we could
            // unlink and re-nq, but if we keep re-inserting the priority
            // we can get into a state where we don't make progress.
            // While that should be considered user error, it is something a
            // starvation-free q should consider.
            return;
        }
        else
        {
            node_unlink_only(to_node(p));
            // Can only be in regular queue.
            --q->size_q;
            --q->size;
        }
    }

    if (LIKELY(p->info[PRIORITY_ABS]))
    {
        p->info[PRIORITY_REL] = p->info[PRIORITY_ABS] + q->pc;
        p->info[PRIORITY_LOC] = PRIORITY_LOC_Q;
        ++q->size_q;
        priorityq_nq_only(q, p);
    }
    else if (p->info[PRIORITY_URG])
    {
        p->info[PRIORITY_REL] = q->pc;
        p->info[PRIORITY_LOC] = PRIORITY_LOC_DONE;
        ++q->size_done;
        list_nq(&q->done, to_node(p));
    }
    else
    {
        p->info[PRIORITY_REL] = q->pc;
        p->info[PRIORITY_LOC] = PRIORITY_LOC_IMED;
        ++q->size_imed;
        list_nq(&q->immediate, to_node(p));
    }
    ++q->size;
}

/**
 * @return The next expired priority; NULL if none.
 */
priority_t *
priorityq_dequeue(priorityq_t *q)
{
    if (LIKELY(q->size))
    {
        struct priorityq_node_s *n;
        do
        {
            priorityq_advance_immediates(q);
            priorityq_advance_priority_queue(q);
            n = list_dq(&q->done);
        } while (!n);
        --q->size_done;
        --q->size;
        priority_t *p = to_priority(n);
        p->info[PRIORITY_LOC] = PRIORITY_LOC_NONE;
        return p;
    }
    else
    {
        return NULL;
    }
}

/**
 * @param p - The priority to remove from the queue.
 * @brief Stop the priority by removing it from the queue.
 */
void
priorityq_remove(priorityq_t *q, priority_t *p)
{
    if (LIKELY(node_in_list(to_node(p))))
    {
        node_unlink(to_node(p));
        priorityq_decrement_queue(q, p);
        --q->size;
        p->info[PRIORITY_LOC] = PRIORITY_LOC_NONE;
    }
}


/*******************************************************************************
 * Priority Queue Functions (Testing)
*******************************************************************************/

uint8_t
priorityq_priority_counter(priorityq_t *q)
{
    return q->pc;
}

/**
 * @brief Counts the number of items in a bin. Mostly used for testing.
 * @param index - The index of the bin to count.
 * @return The count of items in the specified bin; zero on invalid bin.
 */
uint32_t
priorityq_count_bin(priorityq_t *q, uint32_t index)
{
    return list_count((q->bins) + ((PQ_BINS - 1) & index));
}

/**
 * @return The count of all priorities in the datastructure, excluding expired.
 */
uint32_t
priorityq_count_all(priorityq_t *q)
{
    uint32_t count = 0;

    count += list_count(&q->done);
    count += list_count(&q->immediate);
    count += list_count(&q->processing);

    int i;
    for (i = 0; i < PQ_BINS; ++i)
    {
        count += list_count((q->bins) + i);
    }

    return count;
}

/**
 * @return The count of all items in the immediate list.
 */
uint32_t
priorityq_count_immediate(priorityq_t *q)
{
    return list_count(&q->immediate);
}

/**
 * @return The count of all items in the done list.
 */
uint32_t
priorityq_count_done(priorityq_t *q)
{
    return list_count(&q->done);
}

/**
 * @return The count of all items in the queue.
 */
uint32_t
priorityq_count_q(priorityq_t *q)
{
    uint32_t count = 0;

    count += list_count(&q->processing);

    int i;
    for (i = 0; i < PQ_BINS; ++i)
    {
        count += list_count((q->bins) + i);
    }

    return count;
}

uint32_t
priorityq_size_immediate(priorityq_t *q)
{
    return q->size_imed;
}

uint32_t
priorityq_size_done(priorityq_t *q)
{
    return q->size_done;
}

uint32_t
priorityq_size_q(priorityq_t *q)
{
    return q->size_q;
}

uint32_t
priorityq_upper_bit(uint8_t n)
{
    return get_high_index32(n);
}

#ifdef DEBUG_OUTPUT_FUNCTIONS
/**
 * Dumps the bin counts to stdout.
 */
void
priorityq_dump_stats(priorityq_t *q)
{
    printf("PRIORITY COUNTER: %u\n"
           "TOTAL COUNT==SIZE: %u==%u\n"
           "DONE COUNT==SIZE: %u==%u\n"
           "IMMEDIATE COUNT==SIZE: %u==%u\n"
           "QUEUE COUNT==SIZE: %u==%u\n"
           "PROCESSING COUNT: %u\n"
           "BIN COUNTS:\n",
           (uint32_t)q->pc,
           priorityq_count_all(q), q->size,
           priorityq_count_done(q), q->size_done,
           priorityq_count_immediate(q), q->size_imed,
           priorityq_count_q(q), q->size_q,
           list_count(&q->processing));

    int i;
    for (i = 0; i < PQ_BINS; ++i)
    {
        printf("%u: %u\n", i, list_count((q->bins) + i));
    }
}

INLINE static const char *
priority_loc_str(priority_t *p)
{
    switch (p->info[PRIORITY_LOC])
    {
        case PRIORITY_LOC_NONE: return "NONE";
        case PRIORITY_LOC_DONE: return "DONE/URGENT";
        case PRIORITY_LOC_IMED: return "IMMEDIATE";
        case PRIORITY_LOC_Q: return "QUEUE";
        default: return "ERROR";
    }
}

/**
 * Dumps the priority data to stdout.
 */
void
priority_dump_stats(priority_t *p)
{
    printf("PRIORITY: %u\n"
           "RELATIVE PRIORITY: %u\n"
           "LOCATION: %s\n"
           "URGENT? %s\n"
           "IN QUEUE? %s\n",
           (uint32_t)p->info[PRIORITY_ABS],
           (uint32_t)p->info[PRIORITY_REL],
           priority_loc_str(p),
           p->info[PRIORITY_URG] ? "yes" : "no",
           node_in_list(to_node(p)) ? "yes" : "no");
}
#endif

