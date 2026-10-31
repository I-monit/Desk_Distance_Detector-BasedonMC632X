/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author        Notes
 * 2019-02-01    qin&songyh     first version
 */

#ifndef __LRBTREE_H__
#define __LRBTREE_H__

#include <rtthread.h>

struct rb_node
{
    unsigned long rb_parent_color;
#define RB_RED 0
#define RB_BLACK 1
    struct rb_node *rb_right;
    struct rb_node *rb_left;
} __attribute__((aligned(sizeof(long))));
/* The alignment might seem pointless, but allegedly CRIS needs it */

struct rb_root
{
    struct rb_node *rb_node;
};

#define rb_parent(r) ((struct rb_node *)((r)->rb_parent_color & ~3))
#define rb_color(r) ((r)->rb_parent_color & 1)
#define rb_is_red(r) (!rb_color(r))
#define rb_is_black(r) rb_color(r)
#define rb_set_red(r)               \
    do                              \
    {                               \
        (r)->rb_parent_color &= ~1; \
    } while (0)
#define rb_set_black(r)            \
    do                             \
    {                              \
        (r)->rb_parent_color |= 1; \
    } while (0)

static inline void lrb_set_parent(struct rb_node *rb, struct rb_node *p)
{
    rb->rb_parent_color = (rb->rb_parent_color & 3) | (unsigned long)p;
}
static inline void lrb_set_color(struct rb_node *rb, int color)
{
    rb->rb_parent_color = (rb->rb_parent_color & ~1) | color;
}

#define RB_ROOT \
    (struct rb_root) { RT_NULL, }
#define rb_entry(ptr, type, member) rt_container_of(ptr, type, member)

#define RB_EMPTY_ROOT(root) ((root)->rb_node == RT_NULL)
#define RB_EMPTY_NODE(node) (rb_parent(node) == node)
#define RB_CLEAR_NODE(node) (lrb_set_parent(node, node))

static inline void lrb_init_node(struct rb_node *rb)
{
    rb->rb_parent_color = 0;
    rb->rb_right        = RT_NULL;
    rb->rb_left         = RT_NULL;
    RB_CLEAR_NODE(rb);
}

extern void lrb_insert_color(struct rb_node *, struct rb_root *);
extern void lrb_erase(struct rb_node *, struct rb_root *);

typedef void (*rb_augment_f)(struct rb_node *node, void *data);

extern void lrb_augment_insert(struct rb_node *node, rb_augment_f func,
                              void *data);
extern struct rb_node *lrb_augment_erase_begin(struct rb_node *node);
extern void lrb_augment_erase_end(struct rb_node *node, rb_augment_f func,
                                 void *data);

/* Find logical next and previous nodes in a tree */
extern struct rb_node *lrb_next(const struct rb_node *);
extern struct rb_node *lrb_prev(const struct rb_node *);
extern struct rb_node *lrb_first(const struct rb_root *);
extern struct rb_node *lrb_last(const struct rb_root *);

/* Fast replacement of a single node without remove/rebalance/add/rebalance */
extern void lrb_replace_node(struct rb_node *victim, struct rb_node *new,
                            struct rb_root *root);

static inline void lrb_link_node(struct rb_node *node, struct rb_node *parent,
                                struct rb_node **rb_link)
{
    node->rb_parent_color = (unsigned long)parent;
    node->rb_left = node->rb_right = RT_NULL;

    *rb_link = node;
}

#endif /* RBTREE_H_ */
