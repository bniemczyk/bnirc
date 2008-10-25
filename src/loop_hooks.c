/********************************************************************
 * loop_hooks.c <8/10/2005>
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
	list_t header;
	void(*func)(void);
} hook_t;

static list_t head;

INIT_CODE(init_me) {
	list_init(&head);
}

void add_loop_hook ( void(*func)(void) )
{
	hook_t *hook = malloc(sizeof(hook_t));
	hook->func = func;
	push(&head, hook);
}

void remove_loop_hook ( void (*func) ( void ) )
{
	hook_t *i;

	for(i = (hook_t *)head.next; i != (hook_t *)&head; list_next(i)) {
		if(i->func == func) {
			list_prev(i);
			// notice the extra _
			list_remove(list_next_(i));
		}
	}
}

void run_loop_hooks ( void )
{
	hook_t *i;

	for(i = (hook_t *)head.next; i != (hook_t *)&head; list_next(i)) {
		i->func();
	}
}
