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
 * @file calculate.c
 * @author Craig Jacobson
 * @brief Compute amortized time complexity.
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>


static int
get_high_index32(uint32_t n)
{
    return 31 - __builtin_clz(n);
}

static int
get_pop_count32(uint32_t n)
{
    return __builtin_popcount(n);
}

int
main(void)
{
    uint8_t priorities_max = 128;

    double total_updates = 0;
    double total_states = 128 * 256;

    printf("PRIORITY: AMORTIZED_COMPLEXITY\n");
    uint8_t counter = 0;
    int counter_index = 0;
    for (; counter_index < 256; ++counter_index, ++counter)
    {
        double local_updates = 0;
        uint8_t priority = 1;
        for (; priority < priorities_max; ++priority)
        {
            uint8_t relative = counter + priority;
            if (relative > counter)
            {
                int i = get_high_index32(relative ^ counter);
                uint8_t mask = (((uint8_t)1) << i) - 1;
                total_updates += (1 + get_pop_count32((mask & relative)));
                local_updates += (1 + get_pop_count32((mask & relative)));
            }
            else
            {
                uint8_t pc = counter;
                uint8_t apc = pc & pc >> 1;
                apc &= apc >> 2;
                apc &= apc >> 4;
                int i = get_high_index32(relative & counter);
                uint8_t mask = (((uint8_t)1) << i) - 1;
                total_updates += (1 + get_pop_count32((mask & relative)));
                local_updates += (1 + get_pop_count32((mask & relative)));
            }
        }
        double local_amort = local_updates / 128;
        printf("%u: %lf\n", (uint32_t)counter, local_amort);
    }

    double amort = total_updates / total_states;
    printf("Amortized complexity is %lf updates per priority.\n", amort);

    return 0;
}
