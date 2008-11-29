/********************************************************************
 * sigalrm.c <8/19/2005>
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

#include "irc.h"

static int time_compare ( struct timeval *, struct timeval * );

typedef struct {
	list_t header;
	struct timeval runat;
	void(*func)(void *);
	void *arg;
} alarm_t;


static alarm_t ready;
static alarm_t queued;
static inline void set_sigalrm ( struct timeval *now );
static int alarm_compare ( alarm_t *a, alarm_t *b );
static void do_timed_funcs ( void );

static void sig_handler ( int throwaway ) 
{
	alarm_t *i;
	alarm_t now;

	gettimeofday(&now.runat, NULL);

	for(i = bottom(&queued); i != NULL; i = bottom(&queued)) {
		if(alarm_compare(i, &now) > 0)
			break;
		list_remove(i);
		push(&ready, i);
	}

	if(bool_option("debug_sigalrm"))
		cio_out("SIGALRM recieved\n");
}

static inline void set_sigalrm ( struct timeval *now ) {
	struct itimerval alrm;
	alarm_t *next;

	next = bottom(&queued);

	if(!next) {
		setitimer(ITIMER_REAL, NULL, NULL);
		signal(SIGALRM, SIG_IGN);
		return;
	}

	alrm.it_value.tv_sec = next->runat.tv_sec - now->tv_sec;
	alrm.it_value.tv_usec = next->runat.tv_usec - now->tv_usec;

	while(alrm.it_value.tv_usec < 0) {
		alrm.it_value.tv_sec -= 1;
		alrm.it_value.tv_usec += 1000000;
	}

	/* just in case */
	if(alrm.it_value.tv_sec < 0) {
		alrm.it_value.tv_sec = 0;
		alrm.it_value.tv_usec = 1000;
	}

	setitimer(ITIMER_REAL, &alrm, NULL);
	signal(SIGALRM, sig_handler);

	if(bool_option("debug_sigalrm")) {
		cio_out("setting SIGALRM to %ld:%ld [now = %ld:%ld]\n",
				alrm.it_value.tv_sec, alrm.it_value.tv_usec,
				now->tv_sec, now->tv_usec);
		if(bool_option("backtraces"))
			backtrace();
	}
}

static int alarm_compare ( alarm_t *a, alarm_t *b )
{
	return time_compare (&(a->runat), &(b->runat));
}

static int time_compare ( struct timeval *a, struct timeval *b )
{
	if(a->tv_sec > b->tv_sec)
		return 1;
	if(a->tv_sec == b->tv_sec) {
		if(a->tv_usec == b->tv_usec)
			return 0;
		else if(a->tv_usec > b->tv_usec)
			return 1;
		else
			return -1;
	} else
		return -1;
}

void del_timed_func ( void(*func)(void *) )
{
    alarm_t *a;
    for(a = bottom(&queued); a != &queued; a = list_next(a))
    {
        if(a == NULL)
            break;

        if(a->func == func)
        {
            list_remove(a);
            del_timed_func(func);
            break;
        }
    }
}

void add_timed_func ( int msecs, void(*func)(void *), void *arg )
{
	static int do_init = 1;
	struct timeval now;

	if(do_init) {
		do_init = 0;
		list_init(&queued);
		list_init(&ready);
	}

	gettimeofday(&now, NULL);
	alarm_t *new = malloc(sizeof *new);
	new->func = func;
	new->arg = arg;
	new->runat = now;

	if(msecs >= 1000) new->runat.tv_sec += msecs / 1000;
	new->runat.tv_usec += (msecs % 1000) * 1000;

	while(new->runat.tv_usec >= 1000000) {
		new->runat.tv_sec += 1;
		new->runat.tv_usec -= 1000000;
	}

	list_ordered_insert(&queued, new, alarm_compare);

	// make sure sigalrm gets set
	new = bottom(&queued);
	set_sigalrm(&now);
}

void run_timed_funcs ( void )
{
	sig_handler(0);
	struct timeval now;

	do_timed_funcs();
	
	gettimeofday(&now, NULL);
	set_sigalrm(&now);
}

static void do_timed_funcs ( void )
{
	alarm_t *i;

	if(ready.header.next == NULL)
		return;

	for(i = bottom(&ready); i != NULL; i = bottom(&ready)) {
		i->func(i->arg);
		list_remove(i);
		free(i);
	}
}
