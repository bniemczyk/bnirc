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

#ifndef _BNIRC_PLUGIN_H
#define _BNIRC_PLUGIN_H

#include "irc.h"

#define irc_str0 recieve_lval.str0
#define irc_str1 recieve_lval.str1
#define irc_str2 recieve_lval.str2
#define command_str input_lval.str

typedef struct {
	list_t		head;		// internal use
	void		*handle;	// internal use
	char		*filename;	// internal use
	char		*loadstring;	// internal use
	size_t		ref;

	// should be set by the plugin
	const char	name[20];
	const char	version[20];
	const char	description[512];

	/*
	 * hooks that are provided to plugins
	 * ignored if they are NULL
	 */

	// called on plugin load
	int(*plugin_init)(int argc, char *argv[]);

	// called on plugin unload
	int(*plugin_cleanup)(void);

	/**********************************************
	 * list of plugins needed before this one...
	 **********************************************/
	const char **deps;

	/*
	 * the following 2 hooks give you _everything_
	 * and allow you to change it, before it gets
	 * parsed
	 * 
	 * if you do change it, _please_ for the love of god
	 * end the line with \n\0 for irc_hook, and \0
	 * for the input_hook so you don't break
	 * the internal parser
	 */

	// called for all input recieved from the user
	// line by line
	int(*input_hook)(char *cdata);

	// called for all input recieved from the irc server
	// line by line
	int(*irc_hook)(char *cdata);

	// called when an incoming ctcp message
	// is recieved, cdata does _not_ include the ctcp quotes
	int(*ctcp_in_hook)(const char *who, const char *victim, const char *cdata, const char *server);

	// called anytime a privmsg is going to be displayed
	int(*privmsg_hook)(const char *who, const char *victim, const char *cdata, const char *server);

	/* list of commands it implements for the input parser */
	command_t	*command_list;
	server_string_t	*server_string_list;

	/* io driver */
	io_driver_t	*io_driver;

} plugin_t;

extern void register_plugin(plugin_t *);
extern int init_plugin(plugin_t *p, const char *cdata, void *handle, int argc, char **argv);

#define REGISTER_PLUGIN(x, y) \
plugin_t *__do_register_##y ( void ) { \
	register_plugin(&x); \
	return &x; \
}

#endif // _BNIRC_PLUGIN_H
