/*
 * Copyright (c) 2024 Silicon Laboratories Inc. (www.silabs.com)
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of the Silicon Labs Master Software License
 * Agreement (MSLA) available at [1].  This software is distributed to you in
 * Object Code format and/or Source Code format and is governed by the sections
 * of the MSLA applicable to Object Code, Source Code and Modified Open Source
 * Code. By using this software, you agree to the terms of the MSLA.
 *
 * [1]: https://www.silabs.com/about-us/legal/master-software-license-agreement
 */
#ifndef COMMON_TIMER_H
#define COMMON_TIMER_H

#include <sys/queue.h>
#include <stdint.h>

/*
 * Timer module backed by a single timerfd. A sorted list of timers is
 * maintained and the timerfd is always set to expire at the shortest timeout.
 *
 * The module is initialized by calling timer_ctxt_init(), and updated by
 * calling timer_ctxt_process() when timer_ctxt.fd is ready, which is typically
 * queried using select() or poll().
 *
 * To use a timer, set timer.callback and call one of the timer_start_xxx()
 * functions. Timer structures are typically included as a member of a bigger
 * structure, which can be retrieved using container_of() when timer.callback()
 * is invoked.
 *
 * Timers are generally used in different independent modules, which each have
 * their own context. Retrieving the module context from the struct timer is not
 * always possible using container_of(), typically when the module has a list
 * of entries with each their own timer. To handle this issue, modules must
 * register themselves with timer_group_init(), which allows to retrieve module
 * context using container_of() on the struct timer_group from the callback.
 *
 * Periodic timers can be implemented by explicitly calling timer_start_rel()
 * from the callback function, but for convenience timer.period_ms provides an
 * automatic restart mechanism when set.
 */

// Declare struct timer_list
SLIST_HEAD(timer_list, timer_entry);

struct timer_group {
    struct timer_ctxt *ctxt;
    struct timer_list timers;
    SLIST_ENTRY(timer_group) link;
};

// Declare struct timer_group_list
SLIST_HEAD(timer_group_list, timer_group);

struct timer_ctxt {
    int fd;
    struct timer_group_list groups;
};

struct timer_entry {
    uint64_t period_ms;
    void (*callback)(struct timer_group *group, struct timer_entry *timer);

    // Internal fields
    uint64_t expire_ms;
    SLIST_ENTRY(timer_entry) link;
};

// Should be called once at the start of the program before anything else.
void timer_ctxt_init(struct timer_ctxt *ctxt);

// Should be called when ctxt->fd is ready.
void timer_ctxt_process(struct timer_ctxt *ctxt);

// Should be called once per project submodule to register a new timer group.
void timer_group_init(struct timer_ctxt *ctxt, struct timer_group *group);

// Start a timer using an absolute monotonic time.
void timer_start_abs(struct timer_group *group, struct timer_entry *timer, uint64_t expire_ms);

// Start a timer relative to the current time.
void timer_start_rel(struct timer_group *group, struct timer_entry *timer, uint64_t offset_ms);

// Stop a timer.
void timer_stop(struct timer_group *group, struct timer_entry *timer);

#endif
