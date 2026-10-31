/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author        Notes
 * 2019-02-01    qin&songyh     first version
 */

#ifndef __TIMERQUEUE_H__
#define __TIMERQUEUE_H__

#include "lrbtree.h"
#include "ktime.h"

struct timerqueue_node
{
    struct rb_node node;
    ktime_t expires;
};

struct timerqueue_head
{
    struct rb_root head;
    struct timerqueue_node *next;
};

extern void timerqueue_add(struct timerqueue_head *head,
                           struct timerqueue_node *node);
extern void timerqueue_del(struct timerqueue_head *head,
                           struct timerqueue_node *node);
extern struct timerqueue_node *timerqueue_iterate_next(
    struct timerqueue_node *node);

/**
 * timerqueue_getnext - Returns the timer with the earliest expiration time
 *
 * @head: head of timerqueue
 *
 * Returns a pointer to the timer node that has the
 * earliest expiration time.
 */
static inline struct timerqueue_node *timerqueue_getnext(
    struct timerqueue_head *head)
{
    return head->next;
}

static inline void timerqueue_init(struct timerqueue_node *node)
{
    lrb_init_node(&node->node);
}

static inline void timerqueue_init_head(struct timerqueue_head *head)
{
    head->head = RB_ROOT;
    head->next = NULL;
}

#endif /* TIMERQUEUE_H_ */
