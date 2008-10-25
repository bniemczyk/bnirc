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

#include "plugin.h"

#ifdef HAVE_SLANG_H
#include <slang.h>
#else
#error slang is required! you may need to install the slang-dev or slang-devel for your distro
#endif

static	void	intrinsic_command	( const char *com )
{
	char *buf = malloc(strlen(com) + 3);
	sprintf(buf, "/%s\n", com);
	parse_input_string(buf);
	free(buf);
}

static	void	intrinsic_say		( const char *in )
{
	irc_say(in);
}

static	void	intrinsic_msg		( const char *who, const char *msg )
{
	irc_msg(find_user(who, irc_pick_server()), msg);
}

static	void	intrinsic_putstring	( const char *in )
{
	io_colored_out(USER_RED, "[script] ");
	io_colored_out(USER_WHITE, "%s\n", in);
}

static	char *	intrinsic_channel	( void )
{
	const char *channel = natural_channel_name();
	if(!channel)
		return NULL;

	char *rv = malloc(strlen(channel) + 1);
	strcpy(rv, channel);
	return rv;
}


static hash_t	slang_commands;
static hash_t	slang_server_strings;
static hash_t	slang_files;

typedef struct {
	hash_header_t	head;
	command_t	*cmd;
	char *		hook;
} slang_command_t;

typedef struct {
	hash_header_t	head;
	server_string_t	*str;
	char *		hook;
} slang_server_string_t;

typedef struct {
	hash_header_t	head;
	unsigned	flags;
#define SCRIPT_PRAGMA_ONCE 1
} slang_file_t;

static	int	slang_server_string_hook	( int argc, char *argv[] )
{
	char *keystring = argv[1];
	
	slang_server_string_t *com = hash_find_nc(slang_server_strings, keystring, slang_server_string_t *);
	if(!com) {
		if(bool_option("verbose"))
			cio_out("could not find slang_hook for [%s]\n", keystring);
		return -1;
	}

	SLang_Name_Type *name = SLang_get_function(com->hook);
	if(!name) {
		if(bool_option("verbose"))
			cio_out("could not find named slang_hook for [%s]\n", keystring);
		return -1;
	}

	int i;

	SLang_Array_Type *array = SLang_create_array (SLANG_STRING_TYPE, 0, NULL, &argc, 1);
	assert(array != NULL);

	for(i = 0; i < argc; i++) {
		assert(argv[i] != NULL);
		if(-1 == SLang_set_array_element(array, &i, &argv[i])) {
			SLang_free_array(array);
			return -1;
		}
	}

	SLang_start_arg_list();
	if(
		SLang_push_integer(argc) == -1 ||
		SLang_push_array(array, 0) == -1
	  )
		io_colored_out(USER_RED, "could not push argc/argv to slang interp!\n");
	SLang_end_arg_list();

	if(-1 == SLexecute_function(name))
		io_colored_out(USER_RED, "could not execute %s()\n", com->hook);

	SLang_free_array(array);
	return 0;
}

static	int	slang_command_hook	( int argc, char *argv[] )
{
	slang_command_t *com = hash_find_nc(slang_commands, argv[0], slang_command_t *);

	if(!com) {
		if(bool_option("verbose"))
			cio_out("could not find slang_hook for [%s]\n", argv[0]);
		return -1;
	}

	SLang_Name_Type *name = SLang_get_function(com->hook);
	if(!name) {
		if(bool_option("verbose"))
			cio_out("could not find named slang_hook for [%s]\n", argv[0]);
		return -1;
	}

	if(bool_option("verbose")) {
		cio_out("executing scripted function %s()\n", com->hook);
	}

	int i;

	SLang_Array_Type *array = SLang_create_array (SLANG_STRING_TYPE, 0, NULL, &argc, 1);

	for(i = 0; i < argc; i++) {
		if(-1 == SLang_set_array_element(array, &i, &argv[i])) {
			SLang_free_array(array);
			return -1;
		}
	}

	SLang_start_arg_list();
	if(
		SLang_push_integer(argc) == -1 ||
		SLang_push_array(array, 0) == -1
	  )
		io_colored_out(USER_RED, "could not push argc/argv to slang interp!\n");
	SLang_end_arg_list();

	if(-1 == SLexecute_function(name))
		io_colored_out(USER_RED, "could not execute %s()\n", com->hook);

	SLang_free_array(array);
	return 0;
}

static	void	register_slang_command	( const char *cmd, const char *usage, 
		int *min_argc, int *max_argc, const char *hook, const char *description)
{
	command_t *clist = malloc(sizeof(command_t) * 2);
	memset(&clist[1], 0L, sizeof(command_t));
	clist[0].cmd = copy_string(cmd);
	clist[0].usage = copy_string(usage);
	clist[0].min_argc = *min_argc;
	clist[0].max_argc = *max_argc;
	clist[0].hook = slang_command_hook;
	clist[0].description = copy_string(description);
	register_command_list(clist);

	slang_command_t *buf = malloc(sizeof(slang_command_t));
	buf->cmd = clist;
	buf->hook = copy_string(hook);

	slang_command_t *tmp;
	if((tmp = hash_find(slang_commands, cmd, slang_command_t *)) != NULL)
		hash_remove(tmp);

	hash_insert(slang_commands, buf, cmd);
}

static	void	register_slang_server_string	( const char *keystring, int *strip_colon, int *argc, const char *hook )
{
	server_string_t *slist = malloc(sizeof(server_string_t) * 2);
	memset(&slist[1], 0, sizeof(server_string_t));
	slist[0].keystring = copy_string(keystring);
	slist[0].strip_colon = *strip_colon;
	slist[0].argc = *argc;
	slist[0].hook = slang_server_string_hook;
	register_server_string_list(slist);

	slang_server_string_t *buf = malloc(sizeof(slang_server_string_t));
	slang_server_string_t *tmp;

	buf->str = slist;
	buf->hook = copy_string(hook);

	if((tmp = hash_find(slang_server_strings, keystring, slang_server_string_t *)) != NULL)
		hash_remove(tmp);

	hash_insert(slang_server_strings, buf, keystring);
}

static	void	intrinsic_away	( int *a )
{
	away(*a);
}

static	void	intrinsic_irc_connect	( int *port, const char *server )
{
	irc_connect(*port, server);
}

static	void	intrinsic_irc_putstring	( const char *str )
{
	irc_out("%s", str);
}

static	void	intrinsic_exit			( int *status )
{
	exit(*status);
}

static void nick_privmsg ( const char *nick, const char *dest, const char *msg )
{
	user_t *u = find_user(nick, irc_pick_server());
	u->privmsg_print(u, dest, msg);
}

static const char * get_current_nick ( void )
{
	return irc_server_nick(irc_pick_server());
}

#define SFUNC(x) ((FVOID_STAR)x)

static	void	add_intrinsics	( void )
{
	if( -1 == SLadd_intrinsic_function("get_current_nick", SFUNC(get_current_nick), SLANG_STRING_TYPE, 0))
		io_colored_out(USER_RED, "cound not add get_current_nick()!\n");
		
	if( -1 == SLadd_intrinsic_function("nick_privmsg", SFUNC(nick_privmsg), SLANG_VOID_TYPE, 3, SLANG_STRING_TYPE,
					   SLANG_STRING_TYPE, SLANG_STRING_TYPE))
		io_colored_out(USER_RED, "could not add nick_privmsg()!\n");
		
	if( -1 == SLadd_intrinsic_function("register_command", SFUNC(register_slang_command), SLANG_VOID_TYPE, 6,
				SLANG_STRING_TYPE, SLANG_STRING_TYPE, SLANG_INT_TYPE,
				SLANG_INT_TYPE, SLANG_STRING_TYPE, SLANG_STRING_TYPE))
		io_colored_out(USER_RED, "could not add register_command()!\n");

	if( -1 == SLadd_intrinsic_function("irc_connect", SFUNC(intrinsic_irc_connect), SLANG_VOID_TYPE, 2, SLANG_INT_TYPE, SLANG_STRING_TYPE))
		io_colored_out(USER_RED, "could not add irc_connect()!\n");

	if( -1 == SLadd_intrinsic_function("irc_nick", SFUNC(irc_nick), SLANG_VOID_TYPE, 1, SLANG_STRING_TYPE))
		io_colored_out(USER_RED, "could not add irc_nick()!\n");

	if( -1 == SLadd_intrinsic_function("irc_putstring", SFUNC(intrinsic_irc_putstring), SLANG_VOID_TYPE, 1, SLANG_STRING_TYPE))
		io_colored_out(USER_RED, "could not add irc_putstring()!\n");

	if( -1 == SLadd_intrinsic_function("irc_action", SFUNC(irc_action), SLANG_VOID_TYPE, 1, SLANG_STRING_TYPE))
		io_colored_out(USER_RED, "could not add irc_action()!\n");

	if( -1 == SLadd_intrinsic_function("irc_ctcp", SFUNC(irc_ctcp), SLANG_VOID_TYPE, 2, SLANG_STRING_TYPE, SLANG_STRING_TYPE))
		io_colored_out(USER_RED, "could not add irc_ctcp()!\n");

	if( -1 == SLadd_intrinsic_function("register_server_string", SFUNC(register_slang_server_string), SLANG_VOID_TYPE, 4,
				SLANG_STRING_TYPE, SLANG_INT_TYPE, SLANG_INT_TYPE, SLANG_STRING_TYPE))
		io_colored_out(USER_RED, "could not add register_server_string()!\n");

	if( -1 == SLadd_intrinsic_function("irc_ctcp", SFUNC(irc_ctcp), SLANG_VOID_TYPE, 2, SLANG_STRING_TYPE, SLANG_STRING_TYPE))
		io_colored_out(USER_RED, "cound not add irc_ctcp()!\n");

	if( -1 == SLadd_intrinsic_function("command", SFUNC(intrinsic_command), SLANG_VOID_TYPE, 1, SLANG_STRING_TYPE))
		io_colored_out(USER_RED, "could not add command()!\n");

	if( -1 == SLadd_intrinsic_function("putstring", SFUNC(intrinsic_putstring), SLANG_VOID_TYPE, 1, SLANG_STRING_TYPE))
		io_colored_out(USER_RED, "could not add putstring()!\n");

	if( -1 == SLadd_intrinsic_function("say", SFUNC(intrinsic_say), SLANG_VOID_TYPE, 1, SLANG_STRING_TYPE))
		io_colored_out(USER_RED, "could not add say()!\n");

	if( -1 == SLadd_intrinsic_function("google", SFUNC(google), SLANG_VOID_TYPE, 1, SLANG_STRING_TYPE))
		io_colored_out(USER_RED, "could not add google()!\n");

	if( -1 == SLadd_intrinsic_function("fgoogle", SFUNC(fgoogle), SLANG_VOID_TYPE, 1, SLANG_STRING_TYPE))
		io_colored_out(USER_RED, "could not add fgoogle()!\n");

	if( -1 == SLadd_intrinsic_function("do_unquery", SFUNC(do_unquery), SLANG_VOID_TYPE, 0))
		io_colored_out(USER_RED, "could not add do_unquery()!\n");

	if( -1 == SLadd_intrinsic_function("msg", SFUNC(intrinsic_msg), SLANG_VOID_TYPE, 2, 
				SLANG_STRING_TYPE, SLANG_STRING_TYPE))
		io_colored_out(USER_RED, "could not add msg()!\n");

	if( -1 == SLadd_intrinsic_function("channel", SFUNC(intrinsic_channel), SLANG_STRING_TYPE, 0))
		io_colored_out(USER_RED, "could not add channel()!\n");

	if( -1 == SLadd_intrinsic_function("bool_option", SFUNC(bool_option), SLANG_INT_TYPE, 1, SLANG_STRING_TYPE))
		io_colored_out(USER_RED, "could not add bool_option()!\n");

	if( -1 == SLadd_intrinsic_function("int_option", SFUNC(int_option), SLANG_INT_TYPE, 1, SLANG_STRING_TYPE))
		io_colored_out(USER_RED, "could not add int_option()!\n");

	if( -1 == SLadd_intrinsic_function("string_option", SFUNC(string_option), SLANG_STRING_TYPE, 1, SLANG_STRING_TYPE))
		io_colored_out(USER_RED, "could not add string_option()!\n");

	if( -1 == SLadd_intrinsic_function("set_option", SFUNC(set_option), SLANG_VOID_TYPE, 2, SLANG_STRING_TYPE, SLANG_STRING_TYPE))
		io_colored_out(USER_RED, "could not add set_option()!\n");

	if( -1 == SLadd_intrinsic_function("away", SFUNC(intrinsic_away), SLANG_VOID_TYPE, 1, SLANG_INT_TYPE))
		io_colored_out(USER_RED, "could not add away()!\n");

	if( -1 == SLadd_intrinsic_function("load_plugin", SFUNC(load_plugin), SLANG_INT_TYPE, 1, SLANG_STRING_TYPE))
		io_colored_out(USER_RED, "could not add load_plugin()!\n");

	if( -1 == SLadd_intrinsic_function("unload_plugin", SFUNC(unload_plugin), SLANG_VOID_TYPE, 1, SLANG_STRING_TYPE))
		io_colored_out(USER_RED, "could not add unload_plugin()!\n");

	if( -1 == SLadd_intrinsic_function("grab_nick", SFUNC(grab_nick), SLANG_STRING_TYPE, 1, SLANG_STRING_TYPE))
		io_colored_out(USER_RED, "could not add grab_nick()!\n");

	if( -1 == SLadd_intrinsic_function("fork", SFUNC(fork), SLANG_INT_TYPE, 0))
		io_colored_out(USER_RED, "could not add fork()!\n");

	if( -1 == SLadd_intrinsic_function("exit", SFUNC(intrinsic_exit), SLANG_VOID_TYPE, 1, SLANG_INT_TYPE))
		io_colored_out(USER_RED, "cound not add exit()!\n");

}

static	int	init_hook	( int argc, char *argv[] )
{
	hash_init(slang_commands);
	hash_init(slang_server_strings);
	hash_init(slang_files);

	if(SLang_init_all() == -1) {
		io_colored_out(USER_RED, "could not initialize SLang!");
		return -1;
	}

	add_intrinsics();
	
	cio_out("slang scripting engine started!\n");
	return 0;
}

static	int	cleanup_hook	( void )
{
	slang_command_t *i = hash_first(slang_commands, slang_command_t *);
	for( ; i != NULL; i = hash_next(slang_commands, i, slang_command_t *)) {
		unregister_command_list(i->cmd);
	}

	slang_server_string_t *j = hash_first(slang_server_strings, slang_server_string_t *);
	for( ; j != NULL; j = hash_next(slang_server_strings, j, slang_server_string_t *))
		unregister_server_string_list(j->str);
	return 0;
}

static	int	rescript_hook	( int argc, char *argv[] )
{
	cleanup_hook();
	hash_init(slang_files);
	hash_init(slang_commands);
	hash_init(slang_server_strings);
	SLang_Error = 0;
	SLang_restart(1);
	add_intrinsics();
	return 0;
}

static	int	script_hook	( int argc, char *argv[] )
{
	char buf[200] = "scripts/";
	strncat(buf, argv[1], 190);
	FILE *f;

	slang_file_t *flags = hash_find(slang_files, argv[1], slang_file_t *);

	if(!flags) {
		flags = malloc(sizeof(slang_file_t));
		flags->flags = 0;
		hash_insert(slang_files, flags, argv[1]);
	} else {
		if(flags->flags & SCRIPT_PRAGMA_ONCE) {
			if(bool_option("verbose"))
				cio_out("skipping %s because it is %%pragma once\n", argv[1]);
			return 0;
		}
	}

	f = fopen(argv[1], "r");
	if(!f) 
		f = bnirc_fopen(buf, "r");

	if(!f) {
		io_colored_out(USER_RED, "could not find %s\n", argv[1]);
		return -1;
	}

	char *str = NULL;
	size_t size = 0;
	char 	**sargv;
	int	sargc;

	// do pre-processing
	while(1) {
		bnirc_getline(&str, &size, f);
		if(feof(f) || ferror(f))
			break;
		if(!strncmp("%pragma", str, strlen("%pragma"))) {
			make_argv(&sargc, &sargv, str);
			if(sargc > 1 && !strcmp("once", sargv[1])) {
				flags->flags |= SCRIPT_PRAGMA_ONCE;
			}
		} else if(!strncmp("%include", str, strlen("%include"))) {
			make_argv(&sargc, &sargv, str);
			if(sargc < 2 || sargv[1][0] != sargv[1][strlen(sargv[1]) - 1])
				continue;
			remove_char_from_str(sargv[1], 0);
			sargv[1][strlen(sargv[1]) - 1] = 0;
			script_hook(sargc, sargv);
		}
	}

	clearerr(f);
	fseek(f, 0L, SEEK_END);
	size = ftell(f);
	fseek(f, 0L, SEEK_SET);

	str = malloc(size + 1);
	str[size] = 0L;

	fread(str, 1, size, f);

	if(bool_option("verbose")) {
		io_colored_out(USER_WHITE, "SCRIPT -----\n");
		cio_out("%s\n", str);
		io_colored_out(USER_WHITE, "------------\n");
	}

	if(SLang_load_string(str) == -1) {
		io_colored_out(USER_RED, "could not load %s, restarting slang interp\n", argv[1]);
		rescript_hook(0, NULL);
		return -1;
	}

	return 0;
}


static	command_t commands[] = {
	{ "script", "script <filename>", 2, 2, script_hook, "runs a SLang script" },
	{ "rescript", "rescript", 1, 1, rescript_hook, "restarts the scripting engine" },
	{ NULL, NULL, 0, 0, NULL, NULL }
};

static plugin_t plugin = {
	.name		= "script",
	.version	= "0.0.1",
	.description	= "provides scripting abilities in the S-Lang language",
	.plugin_init	= init_hook,
	.plugin_cleanup	= cleanup_hook,
	.command_list	= commands
};

REGISTER_PLUGIN(plugin, script)
