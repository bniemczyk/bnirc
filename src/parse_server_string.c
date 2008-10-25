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

#include "irc.h"

static	inline	char *get_non_cdata	( const char *str )
{
	const char *i = str;
	char	*buf = NULL;
	size_t	len;

	if(!str)
		return NULL;

	if(*i == ':')
		i++;

	while(*i != ':' && *i != 0)
		i++;

	len = (i - str);

	buf = malloc(len + 1);
	if(str[0] == ':')
		strncpy(buf, &str[1], len);
	else
		strncpy(buf, str, len);
	buf[len] = 0;

	return buf;
}

static	int	ping_hook		( int argc, char *argv[] )
{
	if(bool_option("show_pongs"))
		cio_out("responding to PING with '%s'\n", argv[1]);
	irc_out("PONG %s\n", argv[1]);
	return 0;
}

int user_wants_pong = 0;

static	int	pong_hook		( int argc, char *argv[] )
{
	if(user_wants_pong) {
		io_colored_out(USER_RED, "(PONG) ");
		io_colored_out(USER_WHITE, "%s\n", argv[3]);
		user_wants_pong = 0;
	}
	return 0;
}

typedef	struct {
	list_t	head;
	server_string_t *strings;
} server_string_list_t;

typedef struct {
	list_t head;
	void(*callback)(int,char**);
} privmsg_callback_t;

static list_t privmsg_callbacks;

static server_string_t	strings[] = {

	/* need to reply to PINGs! */
	{ "ping", 1, 2, ping_hook },
	{ "pong", 1, 4, pong_hook },

	/* terminate my command list */
	{ NULL, 0, 0, NULL }
};

static server_string_list_t	head;

void	register_server_string_list	( server_string_t *s )
{
	server_string_list_t *tmp = malloc(sizeof(server_string_list_t));
	tmp->strings = s;
	list_init(tmp);
	list_insert(&head, tmp);
}

void	unregister_server_string_list	( server_string_t *s )
{
	server_string_list_t *i;
	for(i = (server_string_list_t *)head.head.next; i != (server_string_list_t *)&head; ) {
		server_string_list_t *tmp = i;
		list_next(i);
		if(tmp->strings == s) {
			list_remove(tmp);
		}
	}
}

typedef struct {
	list_t header;
	void(*func)(int, char *[]);
} rcv_hook_t;

rcv_hook_t hook_head;

void	register_recieve_hook	( void(*func)(int, char *[]) )
{
	rcv_hook_t *hook = malloc(sizeof(rcv_hook_t));
	hook->func = func;
	push(&hook_head, hook);
}

void	unregister_recieve_hook	( void(*func)(int, char *[]) )
{
	rcv_hook_t *i, *tmp;
	i = (rcv_hook_t *)bottom(&hook_head);

	while(i != &hook_head) {
		if(i->func == func) {
			tmp = i;
			list_next(i);
			list_remove(tmp);
			free(tmp);
		} else {
			list_next(i);
		}
	}
}

INIT_CODE(server_string_init)
{
	list_init(&head);
	list_init(&hook_head);
	list_init(&privmsg_callbacks);
	register_server_string_list(strings);
}

/*
 * parses a string sent from the server as described in RFC 2812
 * and looks for a registered handler function, calling it if found
 */
const char *current_server_string = NULL;
int	parse_server_string	( const char *str )
{
	assert(str);

	if(bool_option("show_all_server_strings"))
		cio_out(str);

	current_server_string = str;

	size_t 	i;
	char 	**argv;
	int	argc;
	char	*buf = NULL;
	server_string_t	*si;
	server_string_list_t *sl;

	/* make a buffer */
	buf = copy_string(str);
	assert(buf != NULL);

	buf[strlen(buf) - 1] = 0;

	if(strlen(buf) == 0) {
		return -1;
	}

	make_argv(&argc, &argv, buf);
	assert(argc != 0);

	// we want a consistent format regardless of whether or not a prefix
	// was sent, so if one wasn't we fake it
	if(argv[0][0] != ':') {		// no prefix was found
		char **tmp = malloc(sizeof(char *) * (argc + 1));

		tmp[0] = malloc(strlen(irc_pick_server()) + 2);
		tmp[0][0] = ':';
		strcpy(&tmp[0][1], irc_pick_server());

		for(i = 0; i < argc; i++) {
			tmp[i + 1] = copy_string(argv[i]);
		}
		free_argv(argc, argv);
		argc++;
		argv = tmp;
	}

	// run hooks
	rcv_hook_t *rcvp = &hook_head;
	for(list_next(rcvp); rcvp != &hook_head; list_next(rcvp)) {
		rcvp->func(argc, argv);
	}

	// find a function to run
	sl = (server_string_list_t *)(head.head.next);
find_next_string_handler:
	for( ; (list_t *)sl != (list_t *)&head; list_next(sl)) {
		si = sl->strings;
		while(si->keystring != NULL) {
			if(strcmp_nc(argv[1], si->keystring) == 0) {
				goto found_string_handler;
			}
			si++;
		}
	}

	// nothing was found
	if(bool_option("verbose") || bool_option("show_unhandled_msgs")) {
		cio_out("unhandled string sent from server: %s\n", str);
	}

	free_argv(argc, argv);
	free(buf);
	return -1;

found_string_handler:
	free_argv(argc, argv);

	if(buf[0] == ':') {
		make_max_argv(&argc, &argv, buf, si->argc);
	} else {
		make_max_argv(&argc, &argv, buf, si->argc - 1);
		char **tmp = malloc(sizeof(char *) * (argc + 1));
		tmp[0] = copy_string(irc_pick_server());
		add_char_to_str(tmp[0], 0, ':');
		for(i = 0; i < argc; i++) {
			tmp[i + 1] = copy_string(argv[i]);
		}
		free_argv(argc, argv);
		argc++;
		argv = tmp;
	}

	if(si->strip_colon && strlen(argv[argc - 1]) != 0 && argv[argc - 1][0] == ':') {
		remove_char_from_str(argv[argc - 1], 0);
	}

	/**********************************************
	 * make sure argc is correct, so that the
	 * callbacks don't get any suprises
	 **********************************************/
	assert(!(argc > si->argc));
	if(argc < si->argc) {
		char **tmp = argv;
		argv = malloc(sizeof(char *) * si->argc);
		for(i = 0; i < argc; i++) {
			argv[i] = tmp[i];
		}
		for(i = argc; i < si->argc; i++) {
			argv[i] = malloc(1);
			argv[i][0] = 0;
		}
		argc = si->argc;
	}

	int retval = si->hook(argc, argv);

	if(retval) {
		list_next(sl);
		goto find_next_string_handler;
	}

	free_argv(argc, argv);
	current_server_string = NULL;
	return retval;
}
