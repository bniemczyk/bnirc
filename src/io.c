/*
 * Copyright (C) 2004 Brandon Niemczyk
 *
 * DESCRIPTION:
 * 	Main I/O routines, these interface with the i/o driver directly
 * 	so any calles should be wrapped with if(feature(blar))
 *
 * 	such as:
 *
 * 	if(feature(ungetc))
 * 		driver->ungetc(some_char);
 *
 * CHANGELOG:
 *
 * LICENSE: GPL Version 2
 *
 * TODO:
 *
 */

/** @file io.c */

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

#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include "irc.h"

#if defined(HAVE_LIBREADLINE) && !defined(OSX)
#include <readline/readline.h>
#include <readline/history.h>
#endif


static io_driver_t head;
static io_driver_t *driver;
static size_t cursor_pos;
static int clear_input;
window_t	window_max;
window_t	active_window = 0;
char		trail_char[] = "\0\0";

/*
 * why are getc and putc macros?, thats a bit annoying in here
 * so lets just #undef them, fputc and fgetc can still be used
 */
#ifdef putc
#undef putc
#endif

#ifdef getc
#undef getc
#endif

/*
 * check if a feature is available with the current driver
 */
#define feature(x) ( (driver == NULL || driver == &head ) ? 0 : driver->x )

/*
 * without this the code would be a lot less readable
 */
#define set_cur_pos(x) if(feature(set_cur_pos)) driver->set_cur_pos(x)

static	char *	prepare_str	( const char *str )
{
	static size_t pos = 0;
	size_t i = 0;
	size_t last_space = 0;
	char *tmp;

	if(bool_option("drop_output") && !bool_option("redirect_user"))
		return copy_string("");

	char *ret = malloc(strlen(str) + 1);
	strcpy(ret, str);

	if(!feature(sizex))
		return ret;

	for(i = 0; i < strlen(ret); i++) {

		if(pos > driver->sizex - 2 && last_space) {
			ret[last_space] = '\n';
			pos = i - last_space;
		}

		/* backspace and delete keys aren't really a sec vuln, but annoying */
		if(ret[i] == 127 || ret[i] == 8)
			ret[i] = ' ';

		if(ret[i] == ' ' || ret[i] == '\t') {
			if(pos) {
				last_space = i;
			} else {
				last_space = 0;
			}
		}

		while(pos == 0 && ret[i] == '\n')
			remove_char_from_str(ret, i);

		if(ret[i] == '\n') {
			pos = 0;
		} else {
			pos++;
		}
	}

	/* strip colors, and bold */
	if(bool_option("strip_markup")) {
		while((tmp = strchr(ret, '\003')) != NULL) {
			remove_char_from_str(ret, tmp - ret);
			// remove first num
			if(isdigit(*tmp)) {
				remove_char_from_str(ret, tmp - ret);
			}
			// second?
			if(isdigit(*tmp)) {
				remove_char_from_str(ret, tmp - ret);
			}

			// is there a bg color?
			if(*tmp == ',') {
				remove_char_from_str(ret, tmp - ret);
				if(isdigit(*tmp))
					remove_char_from_str(ret, tmp - ret);
				if(isdigit(*tmp))
					remove_char_from_str(ret, tmp - ret);
			}
		}

		while((tmp = strchr(ret, '\002')) != NULL) {
			remove_char_from_str(ret, tmp - ret);
		}
	}

	return ret;
}

INIT_CODE(my_init)
{
	list_init(&head);
	active_window = 0;
	window_max = 1;
	cio_out("io code initialized\n");
}

static inline void redirect_output ( const char *c )
{
	// kill the trailing \n if there is one
	if(bool_option("debug_redirect"))
		printf("%s\n", c);
	if(c[strlen(c) - 1] == '\n')
		((char *)c)[strlen(c) - 1] = 0;

	user_t *u = find_user(string_option("redirect_user"), irc_pick_server());
	irc_msg(u, c);
}

static inline void	putstring	( const char *c )
{
	char *s = prepare_str(c);

	if(bool_option("redirect_user")) {
		redirect_output(c);
		return;
	}

	if(feature(putstring))
		driver->putstring(s);
	else if(feature(putc))
		while(*s != 0) {
			driver->putc(*s);
			s++;
		}
	else {
		printf("%s", s);
		fflush(stdout);
	}
	free(s);
}

static inline void	wputstring	( window_t w, const char *c )
{
	char *s = prepare_str(c);

	if(bool_option("redirect_user")) {
		redirect_output(c);
		return;
	}

	if(feature(wputstring))
		driver->wputstring(w, s);
	else if(feature(wputc))
		while(*s != 0) {
			driver->wputc(w, *s);
			s++;
		}
	else {
		putstring(s);
	}
	free(s);
}

static inline void init_driver	( void )
{
	if(feature(init))
		driver->init();

	if(feature(max_windows))
		window_max = driver->max_windows - 1;
}

static inline void cleanup_driver	( io_driver_t *d )
{
	assert(d != NULL);

	if(d->cleanup)
		d->cleanup();

	window_max = 0;
}

void	register_io_driver	( io_driver_t *d )
{
	assert(d != NULL);

	if((io_driver_t *)head.head.next != &head)
		cleanup_driver((io_driver_t *)head.head.next);
	list_insert(&head, d);
	driver = (io_driver_t *)head.head.next;
	init_driver();
}

void	unregister_io_driver	( io_driver_t *d )
{
	assert(d != NULL);

	cleanup_driver(d);
	list_remove(d);
	driver = (io_driver_t *)head.head.next;
	init_driver();
}

int	io_get_width	( void )
{
	return feature(sizex) ? driver->sizex - 1 : 80;
}

static	inline	int	get_char	( void )
{
	if(feature(getc))
		return driver->getc();
	else
		return fgetc(stdin);
}

void	set_active_window	( window_t i )
{
	if(
			feature(set_active_window)
			&& feature(max_windows)
			&& driver->max_windows > 1
			&& i < driver->max_windows
	  ) {
			driver->set_active_window(i);
	}
}

extern char **user_find_all_matches(const char *);

/**************************************
 * bash style nick completion
 * FIXME: what if str isn't a big enough
 * buffer?
 **************************************/
static int bash_style_tab_complete ( char **str, size_t *size )
{
	int pos = *size;
	int len = 0;
	char *i = &((*str)[pos - 1]);
	char *buf;
	char **nicks;
	int j, k, l;
	char c;

	while(*i != ' ' && pos != 0) {
		i--;
		len++;
		pos--;
	}


	if(len == 0)
		return 0;

	buf = malloc(len + 1);
	strncpy(buf, ++i, len);
	buf[len] = 0;

	if(bool_option("verbose")) {
		cio_out("bash-tab-completing: %s\n", buf);
	}

	nicks = get_tab_completes(buf);
	free(buf);
	if(nicks[0] == NULL) {
		return 0;
	}

	/**************************************
	 * find the greatest amount of
	 * chars we can complete
	 **************************************/
	int no_more = 0;
	for(j = *size - (i - *str), k = 0; ; k++, j++) {
		if(k >= strlen(nicks[0]))
			break;
		c = nicks[0][k];
		for(l = 1; nicks[l] != NULL; l++) {
			if(k >= strlen(nicks[l]) || toupper(nicks[l][k]) != toupper(c)) {
				no_more = 1;
				break;
			}
		}
		if(no_more)
			break;
	}

	/**************************************
	 * do the complete
	 **************************************/
	if(k != 0) {
		char *complete = malloc(k + 1);
		strncpy(complete, nicks[0], k);
		complete[k] = 0;

		if(nicks[1] == NULL) {
			// buf = string_cat(2, complete, trail_char);
			buf = malloc(strlen(complete) + strlen(trail_char) + 1);
			strcpy(buf, complete);
			strcat(buf, trail_char);
			free(complete);
			complete = buf;
		}

		set_cur_pos(pos);
		if(feature(erase_past_cursor))
			driver->erase_past_cursor();
		if(feature(add_to_input))
			driver->add_to_input(complete);
		*size = cursor_pos = strlen(*str) + strlen(complete) - len;
		set_cur_pos(cursor_pos);
		strcpy(i, complete);
		(*str)[*size] = 0;
	}

	/**************************************
	 * find the longest nick
	 **************************************/
	int longest = 0;
	for(j = 0; nicks[j] != NULL; j++) {
		if(strlen(nicks[j]) > longest)
			longest = strlen(nicks[j]);
	}

	int sizex = feature(sizex) ? driver->sizex - 1: 80;
	int times = sizex / (longest + 4);

	/**************************************
	 * show the possible nicks
	 **************************************/
	if(nicks[1] != NULL) {
		for(j = 0; nicks[j] != NULL; j++) {
			if(j && j % times == 0)
				io_out("\n");
			io_colored_out(USER_RED, " [");
			io_colored_out(USER_WHITE, "%-*s", longest, nicks[j]);
			io_colored_out(USER_RED, "] ");
		}
		io_out("\n");
	}

	free_argv(j, nicks);
	return 1;
}

static int tab_complete ( char **str, size_t *size )
{
	return bash_style_tab_complete(str, size);
}

static int false_char = 0;

void send_char_to_input ( int c ) {
	assert(c != '\n');
	false_char = c;
	assert(io_get_input() == NULL);
}

/***************************************************************
 * the goto makes this a bit hard to follow, but the basic
 * idea is to simulate a coroutine, without any bad asm hacks :)
 ***************************************************************/
char	*io_get_input	( void )
{
	static size_t buf_size = 0;
	static size_t buf_alloced = 0;
	static char *buf = NULL;
	static char *buf2 = NULL;
	static int tmp;
	static int do_goto = 0;
	static history_t *history = NULL;

	if(!irc_shutdown && bool_option("no_input")) {
		return NULL;
	}

	if(do_goto) {
		do_goto = 0;
		goto get_input_resume;
	}


	if(feature(get_line)) {
		buf = driver->get_line();
		goto out;
	}

	set_cur_pos(0);
	history = NULL;


	if(clear_input) {
		if(feature(clear_input))
			driver->clear_input();
	}

	while(1) {

		if(buf_alloced == 0) {
			buf_alloced = 0x40;
			buf = malloc(buf_alloced);
			buf2 = malloc(buf_alloced);
			buf[0] = 0;
			buf2[0] = 0;
		}

		if(buf_alloced <= buf_size + 0x20){
			buf_alloced = (buf_alloced | 0x20) << 1;
			buf = realloc(buf, buf_alloced);
			buf2 = realloc(buf2, buf_alloced);
		}

		assert(buf);
		assert(buf_alloced);
		set_cur_pos(cursor_pos);

get_input_resume:
		if(false_char) {
			tmp = false_char;
			false_char = 0;
		} else {
			tmp = get_char();
		}

		if(tmp == NO_CHAR_AVAIL) {
			do_goto = 1;
			return NULL;
		}

		/*
		 * FIXME:
		 * i really should set something a bit more
		 * elegant up then this massive switch statement
		 */
		switch(tmp) {
			case F1:
			case F2:
			case F3:
			case F4:
			case F5:
			case F6:
			case F7:
			case F8:
			case F9:
			case F10:
			case F11:
			case F12:
				set_active_window(tmp - F1);
				 break;

			case '\r':
			case '\n':
				buf[buf_size] = '\n';
				buf[buf_size + 1] = 0;
				buf_size = 0;
				if(feature(clear_input))
					driver->clear_input();
				user_clear_match();
				cursor_pos = 0;
				goto out;

			case '\t':
				tab_complete(&buf, &buf_size);
				break;

			case DELETE:
				if(buf_size) {
					buf_size--;
					if(feature(clear_input))
						driver->clear_input();
					remove_char_from_str(buf, cursor_pos--);
					if(feature(add_to_input))
						driver->add_to_input(buf);
				}
				break;

			case BACKSPACE:
				if(buf_size) {
					buf_size--;
					if(feature(clear_input))
						driver->clear_input();
					remove_char_from_str(buf, --cursor_pos);
					if(feature(add_to_input))
						driver->add_to_input(buf);
				}
				break;

			/*
			 * for the history, we don't have
			 * to worry about allocating buf
			 * because if it's in history, we know
			 * it fits
			 */
			case UP:
				if(!history) {
					strcpy(buf2, buf);
				}
				history = get_next_history(history);
				if(history) {
					buf_size = strlen(history->str);
					strcpy(buf, history->str);
					if(feature(clear_input)) driver->clear_input();
					if(feature(add_to_input)) driver->add_to_input(buf);
					if(feature(ungetc)) driver->ungetc(BACKSPACE);
					cursor_pos = strlen(buf);
				} else {
					buf_size = strlen(buf2);
					strcpy(buf, buf2);
					if(feature(clear_input)) driver->clear_input();
					if(feature(add_to_input)) driver->add_to_input(buf);
					cursor_pos = strlen(buf);
				}
				break;

			case HOME:
				cursor_pos = 0;
				break;

			case END:
				cursor_pos = strlen(buf);
				break;

			case DOWN:
				if(!history) {
					strcpy(buf2, buf);
				}
				history = get_prev_history(history);
				if(history) {
					buf_size = strlen(history->str);
					strcpy(buf, history->str);
					if(feature(clear_input)) driver->clear_input();
					if(feature(add_to_input)) driver->add_to_input(buf);
					if(feature(ungetc)) driver->ungetc(BACKSPACE);
					cursor_pos = strlen(buf);
				} else {
					buf_size = strlen(buf2);
					strcpy(buf, buf2);
					if(feature(clear_input)) driver->clear_input();
					if(feature(add_to_input)) driver->add_to_input(buf);
					cursor_pos = strlen(buf);
				}
				break;

			case LEFT:
				if(cursor_pos)
					cursor_pos--;
				break;
			case RIGHT:
				if(cursor_pos < buf_size)
					cursor_pos++;
				break;

			default:
				buf_size++;
				if(feature(clear_input)) driver->clear_input();
				add_char_to_str(buf, cursor_pos, (char)tmp);
				buf[buf_size] = 0;
				if(feature(add_to_input)) driver->add_to_input(buf);
				cursor_pos++;
		}

	}

out:
	set_cur_pos(0);
	add_history(buf);
	return buf;
}

void	io_clear_screen	( window_t w )
{
	if(feature(clear_window))
		driver->clear_window(w);
}

/* output routines */

void	begin_io_block	( void )
{
	if(feature(begin_io_block))
		driver->begin_io_block();
}

void	end_io_block	( void )
{
	if(feature(end_io_block))
		driver->end_io_block();
}

static inline void handle_window_status ( char *buf, window_t w )
{
	if(!feature(window_flags))
		return;

	if(w == active_window || !buf)
		return;

	driver->window_flags[w] |= WIN_HAS_OUTPUT;

	if(irc_get_current_nick() && strstr_nc(buf, irc_get_current_nick()))
		driver->window_flags[w] |= WIN_HAS_DIR_OUTPUT;
}

void	vwio_out	( window_t w, const char *fmt, va_list ap )
{
	assert(fmt);

	char buf[4096];
	vsnprintf(buf, 4095, fmt, ap);
	buf[4095] = 0;

	wputstring(w, buf);

	if(feature(set_color) && bool_option("color"))
		driver->set_color(w, 0);

	handle_window_status(buf, w);
}

void	wio_out	( window_t w, const char *fmt, ... )
{
	va_list ap;
	va_start(ap, fmt);
	vwio_out(w, fmt, ap);
	va_end(ap);
}
void	io_out	( const char *fmt, ... )
{
	va_list ap;
	va_start(ap, fmt);
	vwio_out(active_window, fmt, ap);
	va_end(ap);
}

void	vwio_colored_out	( window_t w, int color, const char *fmt, va_list ap )
{
	assert(fmt);

	char buf[4096];
	vsnprintf(buf, 4095, fmt, ap);
	buf[4095] = 0;

	if(feature(set_color) && bool_option("color"))
		driver->set_color(w, color);

	wputstring(w, buf);

	if(feature(set_color) && bool_option("color"))
		driver->set_color(w, 0);

	handle_window_status(buf, w);
}

void	wio_colored_out	( window_t w, int color, const char *fmt, ... )
{
	va_list ap;
	va_start(ap, fmt);
	vwio_colored_out(w, color, fmt, ap);
	va_end(ap);
}

void	io_colored_out	( int color, const char *fmt, ... )
{
	va_list ap;
	va_start(ap, fmt);
	vwio_colored_out(active_window, color, fmt, ap);
	va_end(ap);
}

/* control/cyan output */

void	cvwio_out	( window_t w, const char *fmt, va_list ap )
{
	assert(fmt);

	char buf[4096];
	vsnprintf(buf, 4096, fmt, ap);
	buf[4095] = 0;


	if(!rgxignore_line(buf)) {
		if(feature(set_color) && bool_option("color"))
			driver->set_color(w, USER_CONTROL_COLOR);
		wputstring(w, buf);
		handle_window_status(buf, w);
		if(feature(set_color) && bool_option("color"))
			driver->set_color(w, 0);
	}
}

void	cwio_out	( window_t w, const char *fmt, ... )
{
	va_list ap;
	va_start(ap, fmt);
	cvwio_out(w, fmt, ap);
	va_end(ap);
}

void	cio_out	( const char *fmt, ... )
{
	va_list ap;
	va_start(ap, fmt);
	cvwio_out(active_window, fmt, ap);
	va_end(ap);
}
