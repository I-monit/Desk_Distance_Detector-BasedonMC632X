/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author        Notes
 * 2019-02-01    qin&songyh     first version
 */

#include "timerqueue.h"
#include "lrbtree.h"

/**
 * timerqueue_add - Adds timer to timerqueue.
 *
 * @head: head of timerqueue
 * @node: timer node to be added
 *
 * Adds the timer node to the timerqueue, sorted by the
 * node's expires value.
 */
void timerqueue_add(struct timerqueue_head *head, struct timerqueue_node *node)
{
    struct rb_node **p     = &head->head.rb_node;
    struct rb_node *parent = NULL;
    struct timerqueue_node *ptr;

    while (*p)
    {
        parent = *p;
        ptr    = rb_entry(parent, struct timerqueue_node, node);
        if (node->expires.tv64 < ptr->expires.tv64)
            p = &(*p)->rb_left;
        else
            p = &(*p)->rb_right;
    }
    lrb_link_node(&node->node, parent, p);
    lrb_insert_color(&node->node, &head->head);

    if (!head->next || node->expires.tv64 < head->next->expires.tv64)
        head->next = node;
}

/**
 * timerqueue_del - Removes a timer from the timerqueue.
 *
 * @head: head of timerqueue
 * @node: timer node to be removed
 *
 * Removes the timer node from the timerqueue.
 */
void timerqueue_del(struct timerqueue_head *head, struct timerqueue_node *node)
{
    /* update next pointer */
    if (head->next == node)
    {
        struct rb_node *rbn = lrb_next(&node->node);

        head->next = rbn ? rb_entry(rbn, struct timerqueue_node, node) : NULL;
    }
    lrb_erase(&node->node, &head->head);
    RB_CLEAR_NODE(&node->node);
}

/**
 * timerqueue_iterate_next - Returns the timer after the provided timer
 *
 * @node: Pointer to a timer.
 *
 * Provides the timer that is after the given node. This is used, when
 * necessary, to iterate through the list of timers in a timer list
 * without modifying the list.
 */
struct timerqueue_node *timerqueue_iterate_next(struct timerqueue_node *node)
{
    struct rb_node *next;

    if (!node)
        return NULL;
    next = lrb_next(&node->node);
    if (!next)
        return NULL;
    return rt_container_of(next, struct timerqueue_node, node);
}
