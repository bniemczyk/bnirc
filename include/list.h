/*
 * Copyright (C) 2004 Brandon Niemczyk
 * 
 * DESCRIPTION:
 *
 * CHANGELOG:
 *
 * LICENSE: GPL Version 2
 *
 * TODO:
 *
 */

#ifndef _LIST_H
#define _LIST_H

/*
 * when gcc inlines this it create's subtle bugs especially in user.c
 */
#ifdef __GNUC__
#define LIST_FUNC static __attribute__((noinline))
#else
#define LIST_FUNC static inline
#endif

#include "config.h"

#ifdef HAVE_SEMAPHORE_H
#include <semaphore.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

typedef struct list_struct {
	struct list_struct	*next, *prev;
} list_t;

LIST_FUNC void list_init	( void *l )
{
	list_t *ll = l;
	ll->next = l;
	ll->prev = l;
}

/*
LIST_FUNC void _list_next	( list_t **l )
{
	*l = (*l)->next;
}

#define list_next(list) _list_next((list_t **)&list)
*/
LIST_FUNC void * list_next_ ( void *l )
{
	list_t *ll = l;
	return ll->next;
}

#define list_next(x) ( x = list_next_(x))

/*
LIST_FUNC void _list_prev	( list_t **l )
{
	*l = (*l)->prev;
}

#define list_prev(list) _list_prev((list_t **)&list);
*/
LIST_FUNC void * list_prev_ ( void *l )
{
	list_t *ll = l;
	return ll->prev;
}

#define list_prev(x) ( x = list_prev_(x))

LIST_FUNC void list_insert_	( list_t *prev, list_t *node )
{
	node->next = prev->next;
	node->prev = prev;
	node->next->prev = node;
	node->prev->next = node;
}

#define list_insert(prev, node) list_insert_((list_t *)prev, (list_t *)node)

LIST_FUNC void list_join_ ( list_t *prev, list_t *join )
{
	/***************************************************************
	 *            first(prev)
	 *             /        \
	 *       last(prev) first(join)
	 *  body(last) \        / body(join)
	 *            last(join)
	 ***************************************************************/
	list_t *pbody = prev->next;
	prev->next = join;
	join->prev = prev;
	join->next = pbody;
}

#define list_join(prev, node) list_join_((list_t *)prev, (list_t *)node)

LIST_FUNC void _list_remove	( list_t *node )
{
	assert(node->next);
	assert(node->prev);
	node->next->prev = node->prev;
	node->prev->next = node->next;
}

#define list_remove(node) _list_remove((list_t *)node)

LIST_FUNC int	_list_count ( list_t *node)
{
	list_t 	*i;
	int	rv = 0;
	for(i = node->next; i != node; list_next(i))
		rv++;

	return ++rv;
}

#define list_count(list) _list_count((list_t *)list)
#define list_empty(list) ( ((list_t *)list)->next != (list_t *)list ? 0 : 1 )

/* stack and queue abstraction */
#define push(list, node) list_insert(((list_t *)list)->prev, node)
#define push_front(list, node) list_insert(list, node)
#define top(list) ( ((list_t *)list)->prev != (list_t *)list ? (void *)((list_t *)list)->prev : NULL )
#define bottom(list) ( ((list_t *)list)->next != (list_t *)list ? (void *)((list_t *)list)->next : NULL )
#define pull(list) if(bottom(list)) list_remove(bottom(list))
#define pop(list) if(top(list)) list_remove(top(list))

LIST_FUNC void list_ordered_insert_ ( list_t *head, list_t *node, int(*cmp)(void *, void *) )
{
	list_t *i = bottom(head);

	if(!i) {
		push(head, node);
		return;
	}

	for( ; i != head; list_next(i)) {
		if(cmp(node, i) < 0) {
			push(i, node);
			return;
		}
	}

	push(head, node);
}

#define list_ordered_insert(a, b, f) list_ordered_insert_((list_t *)a, (list_t *)b, (int(*)(void *, void *))f)


#ifndef HASH_SIZE
#define HASH_SIZE 256
#endif

/* hash stuff */
typedef struct {
	list_t	header;
	char	*key;
	size_t	index;
} hash_entry_t;

typedef	hash_entry_t	hash_t[HASH_SIZE];

LIST_FUNC	void	hash_init	( hash_t h )
{
	size_t i;

	for(i = 0; i < HASH_SIZE; i++) {
		list_init(&h[i]);
	}
}

LIST_FUNC	size_t __hash_get_index	( const char *key )
{
	size_t rv = 0;
	size_t i;

	for(i = 0; i < strlen(key); i++) {
		rv += key[i];
		/* make this hash case insensitive */
		if('A' <= key[i] && key[i] <= 'Z') {
			rv += 'a' - 'A';
		}
	}

	rv ^= 0xffffffff;

	/* make sure it's not < 0 */
	rv &= 0x7fffffff;
	return (rv % (HASH_SIZE - 1)) + 1;
}

#define key(entry)	( ((hash_entry_t *)entry)->key )

LIST_FUNC	hash_entry_t *	__hash_find	( hash_t hash, const char *key )
{
	if(!key)
		return NULL;

	size_t i;
	i = __hash_get_index(key);
	hash_entry_t *e;
	e = (hash_entry_t *)bottom(&hash[i]);

	if(!e)
		return NULL;

	while(e != (hash_entry_t *)&hash[i]) {
		if(!strcmp(key(e), key))
			return e;
		list_next(e);
	}

	return NULL;
}

#define hash_find(hash, key, cast) ((cast)__hash_find(hash, key))

LIST_FUNC	hash_entry_t *	__hash_find_nc	( hash_t hash, const char *key )
{
	if(key == NULL)
		return NULL;

	size_t i = __hash_get_index(key);
	hash_entry_t *e = (hash_entry_t *)bottom(&hash[i]);

	if(e == NULL)
		return NULL;

	while(e != (hash_entry_t *)&hash[i]) {
		if(!strcmp_nc(key(e), key))
			return e;
		list_next(e);
	}

	return NULL;
}

#define hash_find_nc(hash, key, cast) ((cast)__hash_find_nc(hash, key))

LIST_FUNC	void		__hash_insert	( hash_t hash, hash_entry_t *e, const char *key )
{
	// calling code should always check to make sure it doesn't
	// exist already first
	assert(!hash_find(hash, key, hash_entry_t *));

	key(e) = malloc(strlen(key) + 1);
	strcpy(key(e), key);
	e->index = __hash_get_index(key);

	push_front(&hash[e->index], e);
}

#define hash_insert(h, e, k) __hash_insert( h, (hash_entry_t *)e, k )

#define hash_index(e) ( ((hash_entry_t *)e)->index )

LIST_FUNC	hash_entry_t *	__hash_first	( hash_t h )
{
	size_t i;
	for(i = 0; i < HASH_SIZE; i++) {
		if(bottom(&h[i]))
			return (hash_entry_t *)bottom(&h[i]);
	}

	return NULL;
}

#define hash_first(h, cast) ( (cast)__hash_first(h) )

LIST_FUNC	hash_entry_t * __hash_next	( hash_t h, hash_entry_t *e )
{
	hash_entry_t *n = e;
	list_next(n);

	if(n != &h[e->index])
		return n;

	size_t i;

	for(i = e->index + 1; i < HASH_SIZE; i++) {
		if(bottom(&h[i]))
			return (hash_entry_t *)bottom(&h[i]);
	}

	return NULL;
}

#define hash_next(h, e, cast) ( (cast)__hash_next(h, (hash_entry_t *)e) )

#define hash_remove(x) list_remove(x)

typedef hash_entry_t hash_header_t;

#endif // _LIST_H
