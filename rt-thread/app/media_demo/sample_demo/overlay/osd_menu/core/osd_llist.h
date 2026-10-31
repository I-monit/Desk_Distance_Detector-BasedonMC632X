/**
 * OSD 链表结构
 *
 * A0(HT)
 *  ||
 * A1(H) <=> B1 <=> C1 <=> D1 <=> E1(T)
 *           ||                    ||
 *          A2(H) <=> B2(T)       C2(H) <=> D2(T)
 *           ||
 *          A3(H) <=> B3 <=> C3 <=> D3(T)
 *
 * 采用多级双向链表，每个节点可以有0或1个前向节点，后向节点，父节点与子节点
 */

#ifndef _OSD_LLIST_
#define _OSD_LLIST_

#define NODE_NULL 0
#define NODE_MAX_NUM 0xffff

typedef struct _node {
    struct _node *next;
    struct _node *prev;
    struct _node *parent;
    struct _node *child;
} node_t;

void node_init(node_t *node);

/** 链表操作 */
node_t *node_getNext(node_t *node);
node_t *node_getPrev(node_t *node);
node_t *node_getChild(node_t *node);
node_t *node_getParent(node_t *node);
void node_setNext(node_t *node, node_t *next);
void node_setPrev(node_t *node, node_t *pre);
void node_setChild(node_t *node, node_t *child);
void node_setParent(node_t *node, node_t *parent);

int node_isHead(node_t *node);
int node_isTail(node_t *node);
int node_hasChild(node_t *node);
node_t *node_getHead(node_t *node);
node_t *node_getTail(node_t *node);
void node_append(node_t *node, node_t *next);
void node_insert(node_t *node, node_t *prev);
void node_adopt(node_t *node, node_t *child);
void node_delete(node_t *node);

#endif  // !_OSD_LLIST_