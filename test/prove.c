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
 * @file prove.c
 * @author Craig Jacobson
 * @brief Demonstrate correctness.
 */
#include "bdd.h"
#include "priorityq.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>


priority_t *
new_priority(uint8_t priority)
{
    priority_t *p = (priority_t *)malloc(sizeof(priority_t));
    priority_init(p);
    priority_set(p, NULL, priority);
    return p;
}

priority_t *
new_urgent(void)
{
    return new_priority(PRIORITY_URGENT);
}

void
free_priority(priority_t *u)
{
    priority_destroy(u);
    free(u);
}

// Avoid having to allocate a priority queue for the tests. Makes them easier.
static priorityq_t _q;
static priorityq_t *q = &_q;
// Sometimes we just need one priority for basic tests. Here it be.
static priority_t _p;
static priority_t *p = &_p;

spec("priorityq library")
{
    describe("priorityq_t (priorityq manager) basics")
    {
        it("should init, size is zero, get nothing, and destroy")
        {
            priorityq_init(q);
            check(0 == priorityq_size(q));
            check(0 == priorityq_count_all(q));
            check(NULL == priorityq_dequeue(q));
            priorityq_destroy(q);
        }
    }

    describe("priority_t basics")
    {
        it("should init, set, and destroy")
        {
            priority_init(p);
            check(0 == priority_value(p));
            check(NULL == priority_data(p));
            check(!priority_is_active(p));
            priority_set(p, p, 1);
            check(1 == priority_value(p));
            check(p == priority_data(p));
            check(!priority_is_active(p));
            priority_destroy(p);
        }
    }

    describe("basic tests")
    {
        before_each()
        {
            priorityq_init(q);
            priority_init(p);
        }

        after_each()
        {
            priority_destroy(p);
            priorityq_destroy(q);
        }

        it("should increase/decrease in size when adding and removing")
        {
            check(0 == priorityq_size(q));
            check(0 == priorityq_count_all(q));
            priorityq_enqueue(q, p);
            check(1 == priorityq_size(q));
            check(1 == priorityq_count_all(q));
            priorityq_remove(q, p);
            check(0 == priorityq_size(q));
            check(0 == priorityq_count_all(q));
        }

        it("should be active if in the queue, and NOT otherwise")
        {
            priority_set(p, NULL, 1);
            check(!priority_is_active(p));
            priorityq_enqueue(q, p);
            check(priority_is_active(p));
            priorityq_remove(q, p);
            check(!priority_is_active(p));

            priorityq_enqueue(q, p);
            check(priority_is_active(p));
            check(p == priorityq_dequeue(q));
            check(!priority_is_active(p));
        }

        it("should return urgent when only item in the queue")
        {
            check(0 == priorityq_size(q));
            check(0 == priorityq_count_all(q));
            priority_set(p, NULL, PRIORITY_URGENT);
            priorityq_enqueue(q, p);
            check(1 == priorityq_size(q));
            check(1 == priorityq_count_all(q));
            check(1 == priorityq_count_done(q));
            check(p == priorityq_dequeue(q));
            check(0 == priorityq_count_done(q));
            check(0 == priorityq_size(q), "%u vs %u", 0, priorityq_size(q));
            check(0 == priorityq_count_all(q));
        }

        it("should return immediate/other when only item in the queue")
        {
            uint32_t actual;

            int i = 0;
            for (; i < PQ_CEILING; ++i)
            {
                priority_set(p, NULL, (uint8_t)i);
                priorityq_enqueue(q, p);
                check(1 == priorityq_size(q));
                actual = priorityq_count_q(q) + priorityq_count_immediate(q);
                check(1 == actual, "priority %d, %u vs %u", i, 1, actual);
                check(p == priorityq_dequeue(q));
                check(0 == priorityq_size(q));
                actual = priorityq_count_q(q) + priorityq_count_immediate(q);
                check(0 == actual, "priority %d, %u vs %u", i, 0, actual);
                check(NULL == priorityq_dequeue(q));
            }
        }
        
        it("should process priorities in order when added out of order")
        {
            // When added between priorityq_dequeue, priorities should be processed in order
            priority_t ps[PQ_CEILING];
            // p will be used for urgent

            int i = PQ_CEILING;
            while (i--)
            {
                priority_init(ps + i);
                priority_set(ps + i, NULL, (uint8_t)i);
                priorityq_enqueue(q, ps + i);
            }

            priority_set(p, NULL, PRIORITY_URGENT);
            priorityq_enqueue(q, p);

            // Check that everything gets removed in reverse order
            // (Order of priority, not order added)
            check(p == priorityq_dequeue(q));
            i = 0;
            for (; i < PQ_CEILING; ++i)
            {
                check((ps + i) == priorityq_dequeue(q), "priority: index(%d) priority(%d)", i, (int)priority_value(ps + i));
            }

            check(NULL == priorityq_dequeue(q));
            check(0 == priorityq_size(q));

            i = PQ_CEILING;
            while (i--)
            {
                priority_destroy(ps + i);
            }
        }

        it("should remove any item in the queue no matter the stage in the queue (also test double remove)")
        {
            priority_set(p, NULL, PRIORITY_URGENT);
            priorityq_enqueue(q, p);
            check(1 == priorityq_count_done(q));
            check(1 == priorityq_size_done(q));
            check(1 == priorityq_size(q));
            priorityq_remove(q, p);
            check(0 == priorityq_count_done(q));
            check(0 == priorityq_size_done(q));
            check(0 == priorityq_size(q));
            check(NULL == priorityq_dequeue(q));
            priorityq_remove(q, p);
            check(0 == priorityq_count_done(q));
            check(0 == priorityq_size_done(q));
            check(0 == priorityq_size(q));
            check(NULL == priorityq_dequeue(q));

            priority_set(p, NULL, 0);
            priorityq_enqueue(q, p);
            check(1 == priorityq_count_immediate(q));
            check(1 == priorityq_size_immediate(q));
            check(1 == priorityq_size(q));
            priorityq_remove(q, p);
            check(0 == priorityq_count_immediate(q));
            check(0 == priorityq_size_immediate(q));
            check(0 == priorityq_size(q));
            check(NULL == priorityq_dequeue(q));
            priorityq_remove(q, p);
            check(0 == priorityq_count_immediate(q));
            check(0 == priorityq_size_immediate(q));
            check(0 == priorityq_size(q));
            check(NULL == priorityq_dequeue(q));

            int i = 1;
            for (; i < PQ_CEILING; ++i)
            {
                int bindex = priorityq_upper_bit((uint8_t)i);
                priority_set(p, NULL, i);
                priorityq_enqueue(q, p);
                check(1 == priorityq_count_q(q));
                check(1 == priorityq_size_q(q));
                check(1 == priorityq_size(q));
                check(1 == priorityq_count_bin(q, bindex));
                priorityq_remove(q, p);
                check(0 == priorityq_count_q(q));
                check(0 == priorityq_size_q(q));
                check(0 == priorityq_size(q));
                check(0 == priorityq_count_bin(q, bindex));
                priorityq_remove(q, p);
                check(0 == priorityq_count_q(q));
                check(0 == priorityq_size_q(q));
                check(0 == priorityq_size(q));
                check(0 == priorityq_count_bin(q, bindex));
                check(NULL == priorityq_dequeue(q));
            }
        }

        it("should NOT re-prioritize an urgent in the queue if already in the queue")
        {
            priority_t _b;
            priority_t _a;
            priority_t *before = &_b;
            priority_t *after = &_a;

            priority_init(before);
            priority_init(after);

            priority_set(before, NULL, PRIORITY_URGENT);
            priority_set(p, NULL, PRIORITY_URGENT);
            priority_set(after, NULL, PRIORITY_URGENT);
            priorityq_enqueue(q, before);
            priorityq_enqueue(q, p);
            priorityq_enqueue(q, after);

            priorityq_enqueue(q, p); // Should not be removed, but kept in order.

            check(before == priorityq_dequeue(q));
            check(p == priorityq_dequeue(q));
            check(after == priorityq_dequeue(q));
            check(NULL == priorityq_dequeue(q));

            priority_destroy(after);
            priority_destroy(before);
        }

        it("should re-prioritize an item in the queue when bumped to urgent")
        {
            priority_t _lu;
            priority_t *lessurgent = &_lu;
            priority_init(lessurgent);

            priority_set(p, NULL, 3);
            priority_set(lessurgent, NULL, 12);

            priorityq_enqueue(q, lessurgent);
            priorityq_enqueue(q, p);
            check(p == priorityq_dequeue(q));
            check(lessurgent == priorityq_dequeue(q));
            check(NULL == priorityq_dequeue(q));

            priorityq_enqueue(q, lessurgent);
            priorityq_enqueue(q, p);
            priority_set(lessurgent, NULL, PRIORITY_URGENT);
            priorityq_enqueue(q, lessurgent);
            check(lessurgent == priorityq_dequeue(q));
            check(p == priorityq_dequeue(q));
            check(NULL == priorityq_dequeue(q));

            priority_destroy(lessurgent);
        }

        it("should re-prioritize any non-urgent to urgent when bumped")
        {
            priority_t _lu;
            priority_t *lu = &_lu;
            priority_init(lu);

            int i;
            for (i = 0; i < PQ_CEILING; ++i)
            {
                priority_set(p, NULL, 0);
                priority_set(lu, NULL, i);

                // Check orgininal ordering.
                priorityq_enqueue(q, p);
                priorityq_enqueue(q, lu);
                check(p == priorityq_dequeue(q));
                check(lu == priorityq_dequeue(q));
                check(NULL == priorityq_dequeue(q));

                // Check new ordering.
                priorityq_enqueue(q, p);
                priorityq_enqueue(q, lu);
                priority_set(lu, NULL, PRIORITY_URGENT);
                priorityq_enqueue(q, lu);
                check(lu == priorityq_dequeue(q));
                check(p == priorityq_dequeue(q));
                check(NULL == priorityq_dequeue(q));
            }

            priority_destroy(lu);
        }

        it("should re-prioritize an item")
        {
            priority_t ps[3];

            int i;
            for (i = 0; i < 3; ++i)
            {
                priority_init(ps + i);
                priority_set(ps + i, NULL, 32);
                priorityq_enqueue(q, ps + i);
            }

            priority_set(p, NULL, 64);
            priorityq_enqueue(q, p);

            for (i = 0; i < 3; ++i)
            {
                check((ps + i) == priorityq_dequeue(q));
            }
            check(p == priorityq_dequeue(q));
            check(NULL == priorityq_dequeue(q));

            for (i = 0; i < 3; ++i)
            {
                priority_set(ps + i, NULL, 29 + i);
                priorityq_enqueue(q, ps + i);
            }

            priority_set(ps, NULL, 0);
            priorityq_enqueue(q, ps);

            priority_set(p, NULL, 64);
            priorityq_enqueue(q, p);

            // Burn the first.
            check(ps == priorityq_dequeue(q));
            priority_set(p, NULL, 2);
            priorityq_enqueue(q, p);
            check(p == priorityq_dequeue(q));
            for (i = 1; i < 3; ++i)
            {
                check((ps + i) == priorityq_dequeue(q));
            }
            check(NULL == priorityq_dequeue(q));
        }
    }

    describe("brute force test")
    {
        before_each()
        {
            priorityq_init(q);
            priority_init(p);
        }

        after_each()
        {
            priority_destroy(p);
            priorityq_destroy(q);
        }

        it("should brute force every possible path of a priority")
        {
            uint8_t counter = 0;
            int counter_index = 0;
            for (; counter_index < 256; ++counter_index, ++counter)
            {
                int priority_index = 0;
                for (; priority_index < 128; ++priority_index)
                {
                    // Reset q
                    priorityq_init(q);

                    // Set internal relative priority to counter value.
                    int slow = 0;
                    priority_set(p, NULL, 1);
                    for (; slow < counter; ++slow)
                    {
                        priorityq_enqueue(q, p);
                        check(p == priorityq_dequeue(q));
                    }
                    check(priorityq_priority_counter(q) == counter);

                    // Should be empty.
                    check(NULL == priorityq_dequeue(q));

                    priority_set(p, NULL, priority_index);
                    priorityq_enqueue(q, p);
                    check(p == priorityq_dequeue(q));
                    check(NULL == priorityq_dequeue(q));

                    priorityq_destroy(q);
                }
            }
        }
    }

    describe("starvation-free properties")
    {
        before_each()
        {
            priorityq_init(q);
            priority_init(p);
        }

        after_each()
        {
            priority_destroy(p);
            priorityq_destroy(q);
        }

        it("should not starve an urgent with urgents")
        {
            priority_t *u = NULL;
            priority_set(p, NULL, PRIORITY_URGENT);

            priorityq_enqueue(q, new_urgent());
            priorityq_enqueue(q, new_urgent());

            priorityq_enqueue(q, p);

            bool found = false;
            const int max_count = 128;
            int count = 0;
            for (; count < max_count; ++count)
            {
                priorityq_enqueue(q, new_urgent());
                priorityq_enqueue(q, new_urgent());
                u = priorityq_dequeue(q);
                if (p == u)
                {
                    found = true;
                    break;
                }
                else
                {
                    free_priority(u);
                }
            }

            check(found);

            while ((u = priorityq_dequeue(q)))
            {
                free_priority(u);
            }
        }

        it("should not starve an immediate/other with urgents")
        {
            int i = 0;
            for (; i < PQ_CEILING; ++i)
            {
                priority_t *u = NULL;
                priority_set(p, NULL, (uint8_t)i);

                priorityq_enqueue(q, new_urgent());
                priorityq_enqueue(q, new_urgent());

                priorityq_enqueue(q, p);

                bool found = false;
                const int max_count = 128;
                int count = 0;
                for (; count < max_count; ++count)
                {
                    priorityq_enqueue(q, new_urgent());
                    priorityq_enqueue(q, new_urgent());
                    u = priorityq_dequeue(q);
                    if (p == u)
                    {
                        found = true;
                        break;
                    }
                    else
                    {
                        free_priority(u);
                    }
                }

                check(found);

                while ((u = priorityq_dequeue(q)))
                {
                    free_priority(u);
                }
            }
        }

        it("should not starve an immediate/other with immediates")
        {
            int i = 0;
            for (; i < PQ_CEILING; ++i)
            {
                priority_t *u = NULL;
                priority_set(p, NULL, (uint8_t)i);

                priorityq_enqueue(q, new_priority(0));
                priorityq_enqueue(q, new_priority(0));

                priorityq_enqueue(q, p);

                bool found = false;
                const int max_count = 128;
                int count = 0;
                for (; count < max_count; ++count)
                {
                    priorityq_enqueue(q, new_priority(0));
                    priorityq_enqueue(q, new_priority(0));
                    u = priorityq_dequeue(q);
                    if (p == u)
                    {
                        found = true;
                        break;
                    }
                    else
                    {
                        free_priority(u);
                    }
                }

                check(found);

                while ((u = priorityq_dequeue(q)))
                {
                    free_priority(u);
                }
            }
        }

        it("should not starve during constant re-insertions")
        {
            priority_t *u = NULL;
            priority_set(p, NULL, 64);

            priorityq_enqueue(q, new_urgent());
            priorityq_enqueue(q, new_urgent());

            priorityq_enqueue(q, p);

            bool found = false;
            const int max_count = 128;
            int count = 0;
            for (; count < max_count; ++count)
            {
                priorityq_enqueue(q, new_urgent());
                priorityq_enqueue(q, new_urgent());
                priority_set(p, NULL, 64);
                priorityq_enqueue(q, p); // Starvation through constant reset.
                u = priorityq_dequeue(q);
                if (p == u)
                {
                    found = true;
                    break;
                }
                else
                {
                    free_priority(u);
                }
            }

            check(found);

            while ((u = priorityq_dequeue(q)))
            {
                free_priority(u);
            }
        }
    }
}

