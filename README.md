
# SFPriorityQ (Starvation-Free Priority Queue)
This is a lazy, starvation-free priority queue implemented using a spinoff of the ideas in craig8196/hitime.

Some features are:
1. Laziness, we defer work in organizing the priority queue.
1. Priorities, 129 priorities: 0-127 and urgent, where 0 is immediate (but not urgent).
1. Scalable, tractable runtime complexity that doesn't grow complexity in terms of the number of priorities (besides the fact that having more means more work).
1. Convenient, easy and constant time start and stop operations.
1. Compact, memory consumption is strictly dependent on how many items are in the queue.
1. Starvation free, items are incrementally advanced to prevent starvation.
1. Orderedness, items of similar priority will be executed in order relative to each other.
1. Coverage, high code coverage.
For more details, read on.

I've never seen a data-structure like this before, I'm hoping it is original and useful.

Enjoy!


## Abstract Referential Orientation (ARO)
This is what I'm calling this class of data-structures.
I don't know if there is already a word for this classification.
Basically, instead of comparing data with each other you create an abstract reference that everything orients around (hence the name).
These structures already exist and we use them often.
Some examples of existing data-structures/concepts that I'm aware of are as follows:
- This project ;)
- Cryptography (most, if not all)
    - Privacy
    - Identity
    - HMAC
    - etc.
- JSON Web Tokens
- Radix Sort
- [HiTime](https://github.com/craig8196/hitime) (another project of mine)

The following are some non-computer science examples:
- Money (ignoring corruption, just the idea people)
- Ethical Foundations (again, the type isn't as important as the idea in this context)


## Contents
- [Features](#features)
- [Code Examples](#code-examples)
- [Build](#build)
- [Time Complexity](#time-complexity)
- [Space Complexity](#space-complexity)
- [Design](#design)
- [Musings](#musings)
- [TODO](#todo)


## Features
To expand on the above...

1. Laziness, we defer work in organizing the priority queue.
    By doing a little work every time you get the next prioritized item the queue progresses.
1. Priorities, 129 priorities: 0-127 and urgent, where 0 is immediate (but not urgent).
    With 129 priorities, most tasks that need prioritizing can be completed.
1. Scalable, tractable runtime complexity that doesn't depend on the number of priorities.
    Efficiency of the queue depends on the maximum priority you use.
    For example, if you only use 16 priorities then the number of updates will be around 4 per priority (instead of 8).
    Since the number of times any priority moves queues if fixed, we get excelent complexity.
    Each moving of a queue is like performing comparisons and swaps in a min heap.
    However, unlike a min heap, the complexity of a single priority doesn't depend on the number of total priorities in the queue.
    This is because the behavior is like a fixed radix sort, but on a moving value (not a moving window).
1. Convenient, easy and constant time start and stop operations.
    Since the queue is lazy, the cost of adding an item to the queue is a single operation.
    Since we're using a doubly linked list, we just unlink the item to remove it from the queue.
1. Compact, memory consumption is strictly dependent on how many items are in the queue.
    As far as space is concerned, the memory is managed in a more fine-grained way.
    Min heaps, and similar data structures, typically will over allocate when expanding.
    A common reallocation scheme is to increase by 50-100% of the current size.
    This can create much wasted space.
    Also, if the memory isn't free'd or reduced, then the implementation will continue to use too much memory.
    With nodes that can be embedded, the memory is fine-grained and we don't over-allocate.
    Also, the memory lives and dies with the item that the priority concerns.
1. Starvation free, items are incrementally advanced to prevent starvation.
    This is tunable. The default is to do the minimum to progress items in the queue.
1. Orderedness, items of similar priority will be executed in order relative to each other.
    This is just a nice consequence of queues.
1. Coverage, high code coverage.


## Code Examples
I've always found learning by example to be my favorite, so...
1. Include:

        #include "pq.h"

1. Create:

        pq_t pq;
        pq_init(&pq);

1. Insert/Prioritize:  
    **Warning: priorities greater than 127 will result in undefined behavior!**

        priority_t p;
        priority_init(&p);
        priority_set(&p, NULL, 0); // Set your data and desired priority (0-127).
        pq_insert(&pq, &p);

        // Re-prioritizaton is allowed (and starvation-free).
        priority_set(&p, NULL, PRIORITY_URGENT); // Set urgent.
        pq_insert(&pq, &p); // Re-prioritize.


1. Remove/Cancel:

        // If you initialized, safe to call anytime; even if not in structure.
        pq_remove(&pq, &p);

1. Prioritizing:

        priority_t *task;
        while ((task = pq_next(&pq)))
        {
            // Do something with task. Insert more priorities.
        }

1. Destroy:

        priority_t *t;
        while ((t = pq_next(&pq)))
        {
            // Clean up.
        }
        pq_destroy(&pq);


## Build
Use the meson build system.

To build:

        meson setup build && cd build/ && meson compile

To run tests:

        meson test

To run code coverage:

        meson configure --Db_coverage=true
        meson compile
        meson test
        ninja coverage-html

To build release (turn on optimizations):

        meson configure -Dbuildtype=release

Note that the default build creates a static library.


## Time Complexity
The following will need to be vetted, but...
* O(1) for `pq_insert`, max of 7 operations (see below).
* O(1) for `pq_remove`.
* O(1) for `pq_next`, incrementally performs 2 other operations (more if the processing queue is empty).
* O(n) for n priorities carried to completion.

We are cheating a little bit. Each priority is actually about O(ceiling(log2(p))) where p is the priority.
Since we only have 128 priorities, we only have about 7 operations we'll perform before the priority is kicked into
the out-going queue.
Since the number is low I brute-forced the amortized complexity, so Amortized O(3.39) is the estimated cost of a priority
(assuming a uniform distribution of priorities over the life of the queue).
The advantage here is that the complexity of any given priority is not dependent on the total number of priorities in the queue.
This indicates that we should have O(n) behavior over the life of the queue and beat a min-heap implementation for sufficiently large n.

The `complexity.c` program was written to demonstrate the complexity.
On my machine, around 5,000 is when the Priority Queue starts to beat out a simple Min-Heap implementation.
While I tried to keep the comparison fair, there are some differences to consider.
The Min-Heap is very straight forward, is NOT starvation-free, and can be optimized by using a different internal priority value.
There is code to try and pre-cache the Min-Heap array to help a fair comparison.
Additionally, this data-structure benefits from better reference of locality.
On the other hand, the Priority Queue has more features and is starvation-free.
The Priority Queue has more computational overhead, leading to a larger constant value to overcome;
it has NOT been optimized and does NOT have the same benefits of reference of locality.


## Space Complexity
Each struct is fixed and space complexity only grows linearly with the number of priorities.
The values are for a 64-bit system with 4 octet alignment.
The `pq_t` struct is 192 octets.
Each `priority_t` is 28 octets.

The size of `pq_t` can be adjusted by customizing the internal data-structure according to constraints that you can enforce.
The size of each `priority_t` can be reduced by making the data implicit by embedding the struct and recovering the pointer to your data type later.


## Design
The the following is the design and description of functionality.

#### Urgent/Immediate Handling and Starvation Prevention
Urgent items are placed directly into the done queue to be processed.
Immediates (priority of 0) are added to their own queue to prevent flooding the done queue.
Once a priority reaches immediate status, it is placed on the immediate queue.

Each time the next priority is obtained the queue is progressed by the `max_processing` setting.
We add one (1) to the `max_processing` setting to avoid starving non-immediate priorities.
This ensures that two (2) is the lowest processing value.
Half of `max_processing` will advance the immediates and the other half advances other priorities.
While 50/50 split may not be the most principled approach, it does avoid starvation without progressing lower priorities too quickly.

The default value for `max_processing` is zero (0).
This setting uses a dynamic value determined from the current size of the queue.
Also, the user can adjust `max_processing` at any time to create custom behavior.

#### The Processing Queue
When new items are prioritized, they are inserted directly into the priority queue.
The processing queue are items that need to be updated and re-inserted into the priority queue.
By keeping them separate we ensure that the queue is progressing.
If the processing queue is empty, then we step up the priority counter and select items to make progress.

#### The Core Idea
The over-arching idea is that we're going to use the addition operation, and the fact that bits flip, as triggers.
Every time a bit flips, we will process the items in that bin.
By placing items in larger bins (lower priorities/higher priority values), we wait for them to be processed longer.
Any item with a non-zero priority has a differing bit, and the largest two differing bit indicate more than half of the life/priority of an item's wait.

When a priority is started we add the priority given by the user to the priority counter.
The priority counter and all priorities are `uint32_t` types and we will simulate overflow.
(The overflow is simulated by masking.)
The priority counter + the priority = the relative priority.
Since 127 is the largest priority value, we get two possible cases:
1. The relative priority is greater than the current counter value and the largest differing bit is the optimal bin to place it in.
1. The relative priority is less than the counter, but the highest bit differs.
    This comes as a consequence of having 127 as the highest priority value, despite `uint8_t` having 255 as a maximum.
    Since the highest bit differs. By the time the counter flips the highest bit, we then have either expired or we are now in the situation of the first case above.
The second case is what allows us to use overflow so we never have to reset.

The following are the edge-cases to test my claim that 127 being the highest value results in optimal bin placement.
The first case is the PC (priority counter) is zero; 0 + 127 = b0000 0000 + b0111 1111 = b0111 1111; the highest bit is zero and the order is preserved as 127 > 0.
The second case is PC = 128, as it is at the boundary; 128 + 127 = b1111 1111 = 255; 255 > 128; the relative priority is greater than the priority counter and so the properties of (>) are preserved.
The third case is more interesting; PC = 129; 129 + 127 = b0000 0000; now we've wrapped around, but the upper-most bit differs which makes sure that it is placed in the correct bin.
The fourth case is PC=255; 255 + 127 = b0111 1110; which also differs in the uppermost bit.
The case we don't want, and have avoided, is to have a relative priority that is less than the priority counterand the uppermost bit is the same.

The counter adds only enough to trigger the bit of the next lowest bin.
Since we're adding at least one (1) each time we advance, we get the guarantee that all bits will eventually trigger.
Adding only enough to trigger the next lowest bit doesn't mean that only the lowest bin will get triggered.


## Musings
This data structure demonstrates a principle that is new to me and quite important.
Data doesn't have to be organized direct and relative to its peers.
In this structure we organize everything relative to a moving point.
That moving point then re-organizes data, the priorities (being fixed points), through the process of its own updates.
By comparison, most data structures organize data relative to the other data in the structure.

Can we beat previous complexities of existing algorithms by using an external organizing mechanism that removes the interdependency of data itself?


## TODO
- [ ] Peer review.
- [ ] Optimize code.

