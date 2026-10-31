
#include "osd_llist.h"

/** 返回后向节点 */
node_t *node_getNext(node_t *node) {
    if (node != NODE_NULL)
        return node->next;
    else
        return NODE_NULL;
}

/** 设置后向节点 */
void node_setNext(node_t *node, node_t *next) {
    if (node != NODE_NULL)
        node->next = next;
}

/** 返回前向节点 */
node_t *node_getPrev(node_t *node) {
    if (node != NODE_NULL)
        return node->prev;
    else
        return NODE_NULL;
}

/** 设置前向节点 */
void node_setPrev(node_t *node, node_t *pre) {
    if (node != NODE_NULL)
        node->prev = pre;
}

/** 返回子节点（上层节点） */
node_t *node_getChild(node_t *node) {
    if (node != NODE_NULL)
        return node->child;
    else
        return NODE_NULL;
}

/** 设置子节点（上层节点） */
void node_setChild(node_t *node, node_t *child) {
    if (node != NODE_NULL)
        node->child = child;
}

/** 返回父节点（上层节点） */
node_t *node_getParent(node_t *node) {
    if (node != NODE_NULL)
        return node->parent;
    else
        return NODE_NULL;
}

/** 设置父节点（上层节点） */
void node_setParent(node_t *node, node_t *parent) {
    if (node != NODE_NULL)
        node->parent = parent;
}

/** 当前节点有子节点 */
int node_hasChild(node_t *node) {
    if (node->child)
        return 1;
    else
        return 0;
}

/** 当前节点是头节点 */
int node_isHead(node_t *node) {
    if (node_getPrev(node) == 0)
        return 1;
    else
        return NODE_NULL;
}

/** 返回当前节点链的头结点 */
node_t *node_getHead(node_t *node) {
    int     cnt;
    node_t *c = node;

    for (cnt = 0; cnt < NODE_MAX_NUM; cnt++) {
        if (node_isHead(c) == 1)
            return c;
        else
            c = c->prev;
    }
    return NODE_NULL;
}

/** 当前节点是尾节点 */
int node_isTail(node_t *node) {
    if (node_getNext(node) == 0)
        return 1;
    else
        return NODE_NULL;
}

/** 返回当前层尾节点 */
node_t *node_getTail(node_t *node) {
    int     cnt;
    node_t *c = node;

    for (cnt = 0; cnt < NODE_MAX_NUM; cnt++) {
        if (node_isTail(c) == 1)
            return c;
        else
            c = c->next;
    }
    return NODE_NULL;
}

void node_append(node_t *node, node_t *next)  // next add after node
{
    node_t *temp;
    if (node_isTail(node)) {
        temp = NODE_NULL;
        node_setNext(node, next);
        node_setNext(next, temp);
        node_setPrev(next, node);
    } else  // not foot
    {
        temp = node_getNext(node);
        node_setNext(node, next);
        node_setPrev(temp, next);
        node_setNext(next, temp);
        node_setPrev(next, node);
    }
}

void node_insert(node_t *node, node_t *prev)  // prev add before node
{
    node_t *temp;
    if (node_isHead(node))  // head
    {
        temp = NODE_NULL;
        node_setNext(prev, node);
        node_setPrev(prev, temp);
        node_setPrev(node, prev);
    } else  // not head
    {
        temp = node_getPrev(node);
        node_setNext(prev, node);
        node_setPrev(prev, temp);
        node_setNext(temp, prev);
        node_setPrev(node, prev);
    }
}

void node_adopt(node_t *parent, node_t *child) {
    node_setChild(parent, child);
    node_setParent(child, parent);
}

void node_delete(node_t *node) {
    node_t *pre;
    node_t *next;
    if (node_isHead(node))  // head
    {
        pre  = NODE_NULL;
        next = node_getNext(node);
        node_setPrev(next, pre);
    } else if (node_isTail(node))  // foot
    {
        pre  = node_getPrev(node);
        next = NODE_NULL;
        node_setNext(pre, next);
    } else {
        pre  = node_getPrev(node);
        next = node_getNext(node);
        node_setNext(pre, next);
        node_setPrev(next, pre);
    }
}

void node_init(node_t *node) {
    node->next   = NODE_NULL;
    node->prev   = NODE_NULL;
    node->parent = NODE_NULL;
    node->child  = NODE_NULL;
}
