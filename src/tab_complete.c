/********************************************************************
 * tab_complete.c <8/11/2005>
 *
 * Copyright (C) 2005 Brandon Niemczyk <brandon@snprogramming.com>
 *
 * DESCRIPTION:
 *
 * CHANGELOG:
 *
 * LICENSE: GPL Version 2
 *
 * TODO:
 *
 ********************************************************************/

#ifndef NDEBUG
#ifndef __cplusplus
extern int printf(const char *, ...);
#define DPRINT printf
#else
extern "C" int printf(const char *, ...);
#define DPRINT ::printf
#endif
#else
#define DPRINT(x,...)
#endif

#ifndef NULL
#ifndef __cplusplus
#define NULL ((void *)0)
#else
#define NULL 0
#endif
#endif

#include "irc.h"

typedef struct {
	list_t head;
	tabfunc func;
} tabfunc_list_t;

static list_t head;
static int do_init = 1;

void add_tab_completes(tabfunc func)
{
	if (do_init) {
		do_init = 0;
		list_init(&head);
	}

	tabfunc_list_t *f = malloc(sizeof *f);
	f->func = func;
	push(&head, f);
}

void remove_tab_completes(tabfunc func)
{
	tabfunc_list_t *i;

	for (i = (tabfunc_list_t *) head.next; i != (tabfunc_list_t *) & head;
	     list_next(i)) {
		if (i->func == func) {
			list_prev(i);
			list_remove(list_next_(i));
		}
	}
}

static tablist_t *get_tab_completes_list(const char *partial)
{
	tabfunc_list_t *i;
	tablist_t *rv = NULL, *tmp;

	for (i = (tabfunc_list_t *) head.next; i != (tabfunc_list_t *) &head;
	     list_next(i)) {
		if (rv == NULL)
			rv = i->func(partial);
		else {
			tmp = i->func(partial);
			if (tmp) {
				list_join(rv, tmp);
			}
		}
	}

	return rv;
}

static void free_tab_completes_list(tablist_t * c)
{
	tablist_t *p;

	while (c != list_next_(c)) {
		p = list_next_(c);
		list_remove(p);
		free(p->word);
		free(p);
	}

	free(c->word);
	free(c);
}

char **get_tab_completes(const char *partial)
{
	tablist_t *list = get_tab_completes_list(partial);
	tablist_t *i;
	char **argv;

	static char *argv_none[] = { NULL };
	if (!list)
		return argv_none;

	int count = 1;

	for (i = list_next_(list); i != list; list_next(i))
		count++;

	argv = malloc(sizeof(char *) * (count + 1));
	argv[count] = NULL;

	count = 1;
	argv[0] = strdup(list->word);
	for (i = list_next_(list); i != list; list_next(i)) {
		argv[count] = strdup(i->word);
		count++;
	}

	free_tab_completes_list(list);

	return argv;
}
