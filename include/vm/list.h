#ifndef __LIST_H
#define __LIST_H

#include "vm/system.h"

#include <stdbool.h>

/* 
 * This doubly-linked list is shamelessly stolen from Linux kernel.
 */

struct list_head {
	struct list_head *next, *prev;
};

#define LIST_HEAD_INIT(name) { .next = &name, .prev = &name }

#define INIT_LIST_HEAD(ptr) do { \
        (ptr)->next = (ptr); (ptr)->prev = (ptr); \
} while (0)

/*
 * Insert a new entry between two known consecutive entries.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __list_add(struct list_head *new, struct list_head *prev,
			      struct list_head *next)
{
	next->prev = new;
	new->prev = prev;
	new->next = next;
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
static inline void list_add(struct list_head *new, struct list_head *head)
{
	__list_add(new, head, head->next);
}

/**
 * list_add_tail - add a new entry
 * @new: new entry to be added
 * @head: list head to add it before
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */
static inline void list_add_tail(struct list_head *new, struct list_head *head)
{
	__list_add(new, head->prev, head);
}

/*
 * Delete a list entry by making the prev/next entries
 * point to each other.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __list_del(struct list_head * prev, struct list_head * next)
{
	next->prev = prev;
	prev->next = next;
}

#define LIST_POISON1  ((void *) 0x00100100)
#define LIST_POISON2  ((void *) 0x00200200)

/**
 * list_del - deletes entry from list.
 * @entry: the element to delete from the list.
 * Note: list_empty on entry does not return true after this, the entry is
 * in an undefined state.
 */
static inline void list_del(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
	entry->next = LIST_POISON1;
	entry->prev = LIST_POISON2;
}

/**
 * list_move - delete from one list and add as another's head
 * @list: the entry to move
 * @head: the head that will precede our entry
 */
static inline void list_move(struct list_head *list, struct list_head *head)
{
	__list_del(list->prev, list->next);
	list_add(list, head);
}

static inline bool list_is_empty(struct list_head *head)
{
	return head->next == head;
}

/**
 * list_entry - get the struct for this entry
 * @ptr:        the &struct list_head pointer.
 * @type:       the type of the struct this is embedded in.
 * @member:     the name of the list_struct within the struct.
 */
#define list_entry(ptr, type, member) \
        container_of(ptr, type, member)

/**
 * list_for_each_entry  -       iterate over list of given type
 * @pos:        the type * to use as a loop counter.
 * @head:       the head for your list.
 * @member:     the name of the list_struct within the struct.
 */
#define list_for_each_entry(pos, head, member)                          \
        for (pos = list_entry((head)->next, typeof(*pos), member);      \
             &pos->member != (head);					\
             pos = list_entry(pos->member.next, typeof(*pos), member))

/**
 * list_for_each_entry_reverse - iterate backwards over list of given type.
 * @pos:        the type * to use as a loop cursor.
 * @head:       the head for your list.
 *  @member:     the name of the list_struct within the struct.
 */
#define list_for_each_entry_reverse(pos, head, member)                  \
	for (pos = list_entry((head)->prev, typeof(*pos), member);      \
	     &pos->member != (head);                                    \
	     pos = list_entry(pos->member.prev, typeof(*pos), member))

/**
 * list_for_each_entry_safe - iterate over list of given type safe against removal of list entry
 * @pos:        the type * to use as a loop counter.
 * @n:          another type * to use as temporary storage
 * @head:       the head for your list.
 * @member:     the name of the list_struct within the struct.
 */
#define list_for_each_entry_safe(pos, n, head, member)                  \
        for (pos = list_entry((head)->next, typeof(*pos), member),      \
                n = list_entry(pos->member.next, typeof(*pos), member); \
             &pos->member != (head);                                    \
             pos = n, n = list_entry(n->member.next, typeof(*n), member))

/**
 * list_first_entry - get the struct for the first entry
 * @head:      the &struct list_head pointer.
 * @type:      the type of the struct this is embedded in.
 * @member:    the name of the list_struct within the struct.
 */
#define list_first_entry(head, type, member) \
	list_entry((head)->next, type, member)
#define list_next_entry list_first_entry

/**
 * list_last_entry - get the struct for the last entry
 * @head:      the &struct list_head pointer.
 * @type:      the type of the struct this is embedded in.
 * @member:    the name of the list_struct within the struct.
 */
#define list_last_entry(head, type, member) \
	list_entry((head)->prev, type, member)
#define list_prev_entry list_last_entry

/**
 * list_first - get first element in a list
 * @head: the head of your list
 */
#define list_next list_first
static inline struct list_head *list_first(struct list_head *head)
{
	return head->next;
}

/**
 * list_last - get last element in a list
 * @head: the head of your list
 */
#define list_prev list_last
static inline struct list_head *list_last(struct list_head *head)
{
	return head->prev;
}

/**
 * list_for_each - iterate over list nodes
 * @node - the iterator of type struct list_head *
 * @head - the head of your list
 */
#define list_for_each(node, head) \
	for (node = head->next; node != head; node = node->next)

typedef int (*list_cmp_fn)(const struct list_head **, const struct list_head **);

/**
 * list_sort - sort the list using given comparator
 * @head: is a head of the list to be sorted
 * @comaprator: function comparing two entries, should return value lesser
 *              than 0 when the first argument is lesser than the second one.
 */
int list_sort(struct list_head *head, list_cmp_fn comparator);

#endif
