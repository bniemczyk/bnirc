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

#ifndef _IRC_H
#define _IRC_H

#include <sys/types.h>
#include <stdio.h>

#include "config.h"

#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SEMAPHORE_H
#include <semaphore.h>
#endif

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#ifdef HAVE_TIME_H
#include <time.h>
#endif

#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#endif

#ifdef HAVE_EXECINFO_H
#include <execinfo.h>
#endif

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_PWD_H
#include <pwd.h>
#endif

#ifdef HAVE_POLL_H
#include <poll.h>
#elif defined(HAVE_ASM_POLL_H)
#include <asm/poll.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#elif defined(HAVE_SOCKET_H)
#include <socket.h>
#endif

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef HAVE_BACKTRACE
extern void __real_backtrace	( void );
#define backtrace() __real_backtrace();
#define bnirc_backtrace() __real_backtrace();
#else
#define backtrace() io_colored_out(USER_RED, "no backtrace() available!\n");
#define bnirc_backtrace() io_colored_out(USER_RED, "no backtrace() available!\n");
#endif

/*
#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif
*/

#ifdef HAVE_GETOPT_H
#ifdef __GNUC__
#ifdef _GNU_SOURCE
#undef _GNU_SOURCE
#endif
#define _GNU_SOURCE
#endif
#include <getopt.h>
#ifdef _GNU_SOURCE
#undef _GNU_SOURCE
#endif
#endif

#ifdef __STRING
#undef __STRING
#endif

#define __STRING(x) #x

#ifdef assert
#undef assert
#endif

#define assert(x) if(!(x)) do { \
	io_colored_out(USER_RED, "BUG: assert(%s) failed on line %d in file %s!\n", __STRING(x), __LINE__, __FILE__); \
	bnirc_backtrace(); \
} while(0)

#define WIN_HAS_OUTPUT 1
#define WIN_HAS_DIR_OUTPUT 2

#ifndef INIT_CODE
#define INIT_CODE(x) static void __attribute__((constructor)) x ( void )
#endif

/* this is just for debugging */
#define free(x) do { \
	free(x); \
	x = NULL; \
} while(0);

#define NO_CHAR_AVAIL -7


typedef unsigned	window_t;
extern	ssize_t	bnirc_getline(char **, size_t *, FILE *);

/* global variables */
extern int	socket_fd;
extern FILE *	netio;
extern char	README[];
extern char	FAQ[];
extern char	*current_username;
extern int	input_debug;
extern int	is_away;
extern int	strict_rfc;
extern char	trail_char[];
extern char	*server_name;

/* some user io routines */
extern	void	io_init	( void );
/** use this to put stuff on the screen */
extern	void	io_out	( const char *, ...) __attribute__ ((format (printf, 1, 2)));
extern void wio_out ( window_t w, const char *, ... ) __attribute__ ((format (printf, 2, 3)));
extern	void	cio_out	( const char *, ...) __attribute__ ((format (printf, 1, 2)));
extern	void	cwio_out ( window_t, const char *, ...) __attribute__ ((format (printf, 2, 3)));
extern	void	cvwio_out (window_t, const char *, va_list);
extern	void	io_colored_out	( int color, const char *fmt, ...) __attribute__ ((format (printf, 2, 3)));
extern	void	wio_colored_out	( window_t w, int color, const char *fmt, ...) __attribute__ ((format (printf, 3, 4)));
extern	void	io_clear_screen	( window_t w );
extern	char	*io_get_input	( void );
extern	void	io_start_scroll	( void );
extern	void	io_refresh_info_line	( void );
extern	int	io_get_width	( void );

/* some network io routines */
extern 	void	irc_connect 	( int port, const char *server );
extern  int     irc_disconnect  ( const char *server );
extern	void	irc_out(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));
extern	void	irc_sout(const char *server, const char *fmt, ...) __attribute__ ((format (printf, 2, 3)));
extern	void	irc_nick(const char *username);
extern	void	irc_say(const char *);
/*extern	void	irc_join(char *, const char *server); */
extern	const char *irc_pick_server(void);
extern  const char *current_ip(void);
extern	void	irc_ctcp(const char *, const char *);
extern	void	irc_action(const char *);
extern	char *	irc_get_current_nick	( void );
extern const char *irc_server_nick	( const char *server );
extern	const char *	irc_get_current_server	( void );
extern	void	do_query	(const char *name);
extern	void	do_unquery	(void);
extern const char *current_query ( void );

/* supporting routines */
/* extern	void	channel_msg(const char *, const char *chan, const char *server, const char *cdata); */
char		*urlize	(const char *);
void		fgoogle	( const char * );
void		google	( const char * );
char		*timestamp	( void );
void		remove_char_from_str	( char *str, size_t i );
void		add_char_to_str		( char *str, size_t i, char c );
char *		string_cat		( size_t n, ... );
char *		string_lc		( const char * );
FILE *		bnirc_fopen		( const char *, const char * );
void *		bnirc_dlopen		( const char *, int );
char *		bnirc_find_file		( const char * );
char *		copy_string		( const char * );
char *		grab_nick		( const char *server_string );
char *		get_username		( void );
int		strcmp_nc		( const char *, const char *);
void * bnirc_zmalloc ( ssize_t s );
#define malloc(x) bnirc_zmalloc(x)

/** make argv style stuff */
void		make_argv		( int *, char ***, const char *str );
void		make_max_argv		( int *, char ***, const char *str, size_t max );
void		free_argv		( int, char ** );
char *		string_replace		( const char *haystack, const char *needle, const char *replacement );
char *		strstr_nc		( const char *, const char *);

/* is non-zero when we are all done */
extern	int	irc_shutdown;

extern	int	parse_input_string	( char *str );
extern	void	push_input_parser	( int(*func)(char *) );
extern	void	pop_input_parser	( void );
extern	int	parse_server_string	( const char *str );
extern	void	unload_all_plugins	( void );
extern	unsigned int	bnirc_usleep		( unsigned );

#define CHANNEL_JOIN		0x200

/*
 * some flags for usernames
 */
#define USER_DEFAULT	 	0x0
#define USER_IGNORE		0x1
#define USER_IS_SELF		0x2
#define USER_RED		0x4
#define USER_GREEN		0x8
#define USER_WHITE		0x10
#define USER_BLUE		0x20
#define USER_CONTROL_COLOR	0x40
#define USER_IS_OP		0x80
#define USER_MSG_QUEUED		0x100
#define USER_IGNORE_NOTICES	0x200
#define ALL_USER_COLORS		(USER_RED|USER_GREEN|USER_WHITE|USER_BLUE)
extern const char **users_in_channel ( const char *channel );

/*
 * some keys
 */
#define F0          0xff00
#define F1			0xff01
#define F2			0xff02
#define F3			0xff03
#define F4			0xff04
#define F5			0xff05
#define F6			0xff06
#define F7			0xff07
#define F8			0xff08
#define F9			0xff09
#define F10			0xff0a
#define F11			0xff0b
#define F12			0xff0c
#define DELETE			0xff0d
#define BACKSPACE		0xff0e
#define UP			0xff10
#define DOWN			0xff11
#define LEFT			0xff12
#define RIGHT			0xff13
#define PUP			0xff14
#define PDOWN			0xff15
#define HOME			0xff16
#define END			0xff17

/*
 * a non-zero value means "going away", 0 means "back"
 */
extern	void		away			( int );
/* IRC specific */
extern	void		away_handle_msg		( const char *user, const char *chan, const char *cdata );
/* not IRC specific */
extern	void		away_add_msg		( const char *user, const char *chan, const char *cdata );

/*
 * some windowing stuff
 */
extern	window_t	window_max;
extern	window_t	active_window;

void	set_active_window ( window_t );

/* option functions */
const char 	*string_option	( const char *name );
const char 	*safe_string_option	( const char *name );
int		int_option	( const char *name );
int		bool_option	( const char *name );
void		set_option	( const char *name, const char *value );
void dump_options_fd ( int fd );

#include "list.h"

/*
 * used by history.c and the io routines
 */
typedef struct {
	list_t	header;
	char	*str;
} history_t;

void		add_history		( const char * );
history_t	*get_next_history	( history_t * );
history_t	*get_prev_history	( history_t * );

typedef struct {
	const char	*cmd;
	const char	*usage;
	int		min_argc;
	int		max_argc;
	int(*hook)(int argc, char *argv[]);
	const char	*description;
} command_t;

typedef	struct	{
	const char	*keystring;
	int		strip_colon;
	int		argc;
	int(*hook)(int,char *[]);
} server_string_t;

typedef	struct	{
	list_t	head;		// for internal use

	int(*putc)(int c);
	int(*putstring)(const char *);
	int(*wputc)(window_t w, int c);
	int(*wputstring)(window_t w, const char *str);
	int(*getc)(void);
	int(*ungetc)(int c);
	char*(*get_line)(void);
	int(*init)(void);
	int(*cleanup)(void);
	int(*set_active_window)(window_t);
	int(*clear_window)(window_t);
	int(*set_color)(window_t, int);
	int(*set_cur_pos)(size_t x);
	int(*erase_past_cursor)(void);
	int(*add_to_input)(const char *);
	int(*clear_input)(void);
	void(*begin_io_block)(void);
	void(*end_io_block)(void);

	int		max_windows;
	window_t	*window_flags;
	int sizex;
	int sizey;
} io_driver_t;

extern	void	unregister_recieve_hook	( void(*)(int,char *[]) );
extern	void	register_recieve_hook	( void(*)(int,char *[]) );
extern	void	register_command_list	( command_t * );
extern	void	unregister_command_list	( command_t * );
extern	void	register_server_string_list	( server_string_t * );
extern	void	unregister_server_string_list	( server_string_t * );
extern	void	register_io_driver	( io_driver_t * );
extern	void	unregister_io_driver	( io_driver_t * );
extern	void	begin_io_block		( void );
extern	void	end_io_block		( void );

/**********************************************
 * bnIRC's poll interface, if dealing with
 * networks don't use this directly, instead
 * use client_connection and
 * client_line_connection interface
 **********************************************/
typedef struct	{
	int	fd;
	void	*data;
	void(*callback)(void *);
	void(*error_callback)(void *);
	void(*eof_callback)(void *);
	FILE	*in;
	FILE	*out;
} irc_poll_t;

/* set before any callbacks are called... NULL otherwise */
extern volatile irc_poll_t	*current_poll;

void	register_poll ( irc_poll_t * );
void	unregister_poll ( irc_poll_t * );
void	start_polling(void);
void	start_polling_block(void);
void	end_polling(void);

/*
 * for "client" connections
 */
typedef struct {
	irc_poll_t	poll;	// parent class
	struct sockaddr_in	address;
	const char	*hostname;
	int		port;
} client_connection_t;

typedef struct client_line_connection_s {
	client_connection_t con;
	void(*callback)(struct client_line_connection_s *, char *);
	char 	*buf;
	int  	buf_size;
	int 	cur_size;
} client_line_connection_t;

extern volatile client_line_connection_t *current_line_client;

typedef void(*callback_t)(void *);
typedef void(*client_callback_t)(client_connection_t *);
typedef void(*client_line_callback_t)(client_line_connection_t *, char *);

void	construct_client_connection ( int port, const char *server, client_callback_t, client_connection_t **);
void	construct_client_line_connection ( int port, const char *server, client_line_callback_t, client_line_connection_t **);
extern	int user_wants_pong;

/*
 * some plugin functions for _internal_ use
 */
void		plugin_irc_hook		( char *cdata );
void		plugin_input_hook	( char *cdata );
void		plugin_com_parser	( int com, char *cdata );
void		plugin_ctcp_in_hook	( const char *who, const char *victim, const char *cdata, const char *server );
void		plugin_privmsg_hook	( const char *who, const char *victim, const char *cdata, const char *server );
void		plugin_list		( void );
int		load_plugin		( const char * );
void		unload_plugin		( const char * );
void		reload_all_plugins	( void );
void		reload_plugin		( const char * );

static inline __attribute__((always_inline)) size_t	__bnirc_strlen(const char *s, const char *varname)
{
	assert(s != NULL);

	if(s == NULL) {
		cio_out("strlen(NULL)\n varname: %s\n", varname);
		return 0;
	}

	return strlen(s);
}

typedef struct {
	void *data;
	int size;
} regex_t;

extern const char *current_server_string, *current_input_string;
extern int regex ( int FLAGS, const char *regex, const char *data, ... );
extern int regex_search ( const char **start, int FLAGS, const char *regex, const char *data, ... );
extern char *regex_next_group();
extern int rgxignore_line(const char *line);
extern int rgxhilight_line(const char *line);

/* regex FLAGS */
#define RGX_DEFAULT 0x0
#define RGX_STOREGROUPS 0x1
#define RGX_DUMP 0x2
#define RGX_SHOWGROUPS 0x4
#define RGX_GLOBAL 0x8
#define RGX_DEBUG_MATCH 0x10
#define RGX_IGNORE_CASE 0x20

#define RGX_DEBUG RGX_SHOWGROUPS|RGX_DUMP|RGX_DEBUG_MATCH


#define strlen(x) __bnirc_strlen(x, __STRING(x))

typedef struct {
	list_t header;
	char *word;
} tablist_t;

typedef tablist_t*(*tabfunc)( const char *partial );

extern void add_loop_hook ( void(*)(void) );
extern void remove_loop_hook ( void(*)(void) );
extern void run_loop_hooks (void);
extern char **get_tab_completes ( const char *partial );
extern void add_tab_completes ( tabfunc );
extern void remove_tab_completes ( tabfunc );

#define tablist_add(x, n) do { \
	if (x == NULL) \
		x = n; \
	else \
		push(x, n); \
} while(0)

#define tablist_add_word(x, n) do { \
	tablist_t *tlist = malloc(sizeof *tlist); \
	list_init(tlist); \
	tlist->word = strdup(n); \
	tablist_add(x, tlist); \
} while(0)

#define TAB_INIT NULL

extern int use_comments;
extern void send_char_to_input(int);
extern void add_timed_func (int, void(*)(void *), void *arg);
extern void del_timed_func (void(*)(void *));
extern void run_timed_funcs ( void );

#include "channel.h"
#include "user.h"
#include "ircserver.h"

#endif
