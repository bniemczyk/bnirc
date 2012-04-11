/*
 * Copyright (C) 2004 Brandon Niemczyk
 *
 * DESCRIPTION:
 * 	creates a basic app-wide interface to the poll() function
 * 	if you are using network sockets, you'd probably be more interested
 * 	in the superclasses client_connection_t and client_line_connection_t
 * 	and the functions found in connect.c
 *
 * 	this file creates a thread that does nothing but poll() all the registered
 * 	irc_poll_t * objects, when something is ready to be read, it calls
 * 	the objects callback
 *
 * CHANGELOG:
 *
 * LICENSE: GPL Version 2
 *
 */

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
#define NULL ((void *)0)
#endif

#define INITIAL_SLOTS_ALLOC 12

#include "irc.h"

static irc_poll_t **poll_pointers;
static struct pollfd *fds;
static volatile int fd_slots;
static volatile int slots_in_use;

volatile irc_poll_t *current_poll = NULL;

static void __attribute__ ((constructor)) init(void)
{
	cio_out("initializing poll.c\n");
	poll_pointers = malloc(sizeof(irc_poll_t *) * INITIAL_SLOTS_ALLOC);
	fds = malloc(sizeof(struct pollfd) * INITIAL_SLOTS_ALLOC);
	fd_slots = INITIAL_SLOTS_ALLOC;
	slots_in_use = 0;
}

static int alloc_free_slot(void)
{
	if (slots_in_use < fd_slots)
		return slots_in_use++;
	else if (slots_in_use == fd_slots) {
		fd_slots <<= 1;
		poll_pointers =
		    realloc(poll_pointers, sizeof(irc_poll_t *) * fd_slots);
		fds = realloc(fds, sizeof(struct pollfd) * fd_slots);
		return slots_in_use++;
	} else {
		assert(!"code should never get here");
		return -1;
	}
}

void unregister_poll(irc_poll_t * sel)
{
	int i;


	for (i = 0; i < slots_in_use; i++) {
		if (fds[i].fd == sel->fd) {
			break;
		}
	}

	if (i == slots_in_use)
		goto unregister_poll_out;

	for (++i; i < slots_in_use; i++) {
		memcpy(&fds[i - 1], &fds[i], sizeof(struct pollfd));
		poll_pointers[i - 1] = poll_pointers[i];
	}

	slots_in_use--;

      unregister_poll_out:
	return;
}

void register_poll(irc_poll_t * sel)
{
	int i;

	for (i = 0; i < slots_in_use; i++) {
		if (fds[i].fd == sel->fd) {
			assert(!"attempt to add irc_poll_t multiple times!");
		}
	}

	i = alloc_free_slot();
	fds[i].fd = sel->fd;
	fds[i].events = POLLIN;
	poll_pointers[i] = sel;

	sel->in = fdopen(sel->fd, "r");
	sel->out = fdopen(sel->fd, "w");

	/* is there a nicer way to deal with this? */
	setvbuf(sel->in, NULL, _IONBF, 0);

	if (bool_option("verbose"))
		cio_out("registered poll [%p] with slot %d\n", sel, i);

	return;
}

/***************************************************************
 * main polling function for non-threaded
 ***************************************************************/

static void poll_loop_func  (void)
{
	int i, poll_ok;
	irc_poll_t *tmp;

	/***************************************************************
	 * we depend on this function sleeping
	 ***************************************************************/
	if (!slots_in_use) {
		bnirc_usleep(1);
		return;
	}

	/**********************************************
	 * handle errors and eofs, the default
	 * behaviour is quite dangerous
	 * if no callbacks were provided
	 **********************************************/
	for (i = 0; i < slots_in_use; i++) {
		if (poll_pointers[i] != NULL) {
			assert(poll_pointers[i]->in);
			assert(poll_pointers[i]->out);

			if (ferror(poll_pointers[i]->in)
			    || ferror(poll_pointers[i]->out)) {
				if (poll_pointers[i]->error_callback !=
				    NULL)
					poll_pointers[i]->
					    error_callback(poll_pointers
							   [i]->data);
				else {
					clearerr(poll_pointers[i]->in);
					clearerr(poll_pointers[i]->out);
				}
			}

			if (feof(poll_pointers[i]->in)
			    || feof(poll_pointers[i]->out)) {
				if (poll_pointers[i]->eof_callback !=
				    NULL)
					poll_pointers[i]->
					    eof_callback(poll_pointers
							 [i]->data);
				else {
					clearerr(poll_pointers[i]->in);
					clearerr(poll_pointers[i]->out);
				}
			}
		}
	}

	/***************************************************************
	 * go as long as possible until either we can read something
	 * or a signal interrupts us
	 ***************************************************************/
	poll_ok = poll(fds, slots_in_use, 1000);

	/***************************************************************
	 * cheap optimization so that we don't have to
	 * constantly refresh, instead we let the callbacks
	 * *think* they are printing to the screen, and
	 * do it all at the end in one refresh
	 ***************************************************************/
	begin_io_block();

	if (poll_ok < 0) {
#ifdef HAVE_ERRNO_H
		switch (errno) {
		case EAGAIN:
			cio_out("poll() returned EAGAIN\n");
			break;
		case EINTR:
			/* this is perfectly ok, do nothing */
			break;
		case EINVAL:
			cio_out("poll() returned EINVAL\n");
			break;
		default:
			assert(!"poll returned an unknown error!\n");
		}
#else
		assert
		    (!"poll returned an unknown error! (errno unavailable)\n");
#endif
	}

	for (i = 0; i < slots_in_use; i++) {

		if (fds[i].revents == 0) {
			continue;
		}

		fflush(poll_pointers[i]->in);
		fflush(poll_pointers[i]->out);

		tmp = poll_pointers[i];

		assert(tmp->callback != NULL);
		current_poll = tmp;

		/***************************************************************
		 * finally, we handle our input
		 ***************************************************************/
		tmp->callback(tmp->data);
		current_poll = NULL;
	}

	run_timed_funcs();

	/***************************************************************
	 * refresh the screen if it needs it
	 ***************************************************************/
	end_io_block();
}

void pollprintf(irc_poll_t *poll, const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	vpollprintf(poll, format, ap);
	va_end(ap);
}

void vpollprintf(irc_poll_t *poll, const char *format, va_list ap)
{
	if(poll->is_ssl) {
		size_t len = strlen(format) + (1024);
		char *str = malloc(sizeof(char) * len);
		vsnprintf(str, len, format, ap);
#ifdef SSL_AVAIL
		int sslwlen = SSL_write(poll->ssl, str, strlen(str));
		if(sslwlen != strlen(str)) 
		{
			cio_out("SSL_write() returned %d\n", sslwlen);
			if(sslwlen == -1) {
				cio_out("%s\n", ERR_error_string(SSL_get_error(poll->ssl, sslwlen),NULL));
			}
		}
		BIO_flush(poll->bio);
#endif
	} else {
		vfprintf(poll->out, format, ap);
	}
}

INIT_CODE(register_poll_loop) {
	add_loop_hook(poll_loop_func);
}
