#ifndef _FH_OSAL_LIST_H
#define _FH_OSAL_LIST_H

/*
 * Simple doubly linked list implementation.
 *
 * Some of the internal functions ("__xxx") are useful when
 * manipulating whole lists rather than single entries, as
 * sometimes we already know the next/prev entries and we can
 * generate better code by using them directly rather than
 * using the generic single-entry routines.
 */

struct fh_osal_list_head {
	struct fh_osal_list_head *next, *prev;
};
#define FH_OSAL_LIST_HEAD_INIT(name) { &(name), &(name) }

#define FH_OSAL_LIST_HEAD(name) \
	struct fh_osal_list_head name = FH_OSAL_LIST_HEAD_INIT(name)

#define fh_osal_list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

#define fh_osal_offsetof(TYPE, MEMBER) ((int) &((TYPE *)0)->MEMBER)

#define fh_osal_container_of(ptr, type, member) ({		  \
	const __typeof__( ((type *)0)->member ) *__mptr = (ptr);	\
	(type *)( (char *)__mptr - fh_osal_offsetof(type,member) );})

#define fh_osal_list_entry(ptr, type, member) \
	fh_osal_container_of(ptr, type, member)

#define fh_osal_list_first_entry(ptr, type, member) \
	fh_osal_list_entry((ptr)->next, type, member)

#define fh_osal_list_next_entry(pos, member) \
	fh_osal_list_entry((pos)->member.next, typeof(*(pos)), member)

#define fh_osal_list_for_each_entry(pos, head, member)				\
	for (pos = fh_osal_list_first_entry(head, typeof(*pos), member);	\
		 &pos->member != (head);					\
		 pos = fh_osal_list_next_entry(pos, member))

#define fh_osal_list_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); \
		pos = n, n = pos->next)

#define fh_osal_list_for_each_entry_safe(pos, n, head, member)			\
	for (pos = fh_osal_list_first_entry(head, typeof(*pos), member),	\
		n = fh_osal_list_next_entry(pos, member);			\
		 &pos->member != (head); 					\
		 pos = n, n = fh_osal_list_next_entry(n, member))

static inline void FH_OSAL_INIT_LIST_HEAD(struct fh_osal_list_head *list)
{
	list->next = list;
	list->prev = list;
}

/*
 * Insert a new entry between two known consecutive entries.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __fh_osal_list_add(struct fh_osal_list_head *new,
				  struct fh_osal_list_head *prev,
				  struct fh_osal_list_head *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

/**
 * list_add - add a new entry
 * @new: new entry to be added
 * @head: list head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
static inline void fh_osal_list_add(struct fh_osal_list_head *new, struct fh_osal_list_head *head)
{
	__fh_osal_list_add(new, head, head->next);
}

/**
 * list_add_tail - add a new entry
 * @new: new entry to be added
 * @head: list head to add it before
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */
static inline void fh_osal_list_add_tail(struct fh_osal_list_head *new, struct fh_osal_list_head *head)
{
	__fh_osal_list_add(new, head->prev, head);
}

/**
 * list_empty - tests whether a list is empty
 * @head: the list to test.
 */
static inline int fh_osal_list_empty(const struct fh_osal_list_head *head)
{
	if (head == NULL) {
        return -1;
    }

	return head->next == head;
}

/*
 * Delete a list entry by making the prev/next entries
 * point to each other.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __fh_osal_list_del(struct fh_osal_list_head * prev, struct fh_osal_list_head * next)
{
	next->prev = prev;
	prev->next = next;
}

/**
 * list_del - deletes entry from list.
 * @entry: the element to delete from the list.
 * Note: list_empty() on entry does not return true after this, the entry is
 * in an undefined state.
 */

#define FH_OSAL_LIST_POISON1  ((void *) 0x00100100)
#define FH_OSAL_LIST_POISON2  ((void *) 0x00200200)

static inline void fh_osal_list_del(struct fh_osal_list_head *entry)
{
	__fh_osal_list_del(entry->prev, entry->next);
	entry->next = FH_OSAL_LIST_POISON1;
	entry->prev = FH_OSAL_LIST_POISON2;
}

static inline int fh_osal_list_is_last(const struct fh_osal_list_head *list,
				const struct fh_osal_list_head *head)
{
	return list->next == head;
}

/**
 * list_move_tail - delete from one list and add as another's tail
 * @list: the entry to move
 * @head: the head that will follow our entry
 */
static inline void fh_osal_list_move_tail(struct fh_osal_list_head *list,
				  struct fh_osal_list_head *head)
{
	__fh_osal_list_del(list->prev, list->next);
	fh_osal_list_add_tail(list, head);
}

/**
 * list_bulk_move_tail - move a subsection of a list to its tail
 * @head: the head that will follow our entry
 * @first: first entry to move
 * @last: last entry to move, can be the same as first
 *
 * Move all entries between @first and including @last before @head.
 * All three entries must belong to the same linked list.
 */
static inline void fh_osal_list_bulk_move_tail(struct fh_osal_list_head *head,
				       struct fh_osal_list_head *first,
				       struct fh_osal_list_head *last)
{
	first->prev->next = last->next;
	last->next->prev = first->prev;

	head->prev->next = first;
	first->prev = head->prev;

	last->next = head;
	head->prev = last;
}


#endif
