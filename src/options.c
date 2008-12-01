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

hash_t	options;

static	int	set_hook	( int, char *[] );
static	int	unset_hook	( int, char *[] );

static tablist_t *tablist_func ( const char *partial )
{
	option_t *i;
	tablist_t *rv = TAB_INIT;

	for(i = hash_first(options, option_t *); i != NULL;
			i = hash_next(options, i, option_t *)) {
		if(strncasecmp(key(i), partial, strlen(partial)))
			continue;
		tablist_add_word(rv, key(i));
	}

	return rv;
}

static command_t	commands[] = {
	{ "set", "set [option] [value]", 1, 3, set_hook, "set or show the value of an option" },
	{ "unset", "unset <option>", 2, 2, unset_hook, "unsets an option" },
	{ NULL, NULL, 0, 0, NULL, NULL }
};

INIT_CODE(my_init)
{
	hash_init(options);
	register_command_list(commands);
	add_tab_completes(tablist_func);
	cio_out("options code initialized\n");
}

void dump_options_fd ( int fd )
{
	option_t *i = hash_first(options, option_t *);
	for( ; i != NULL; i = hash_next(options, i, option_t *)) {
		write(fd, key(i), strlen(key(i)));
		write(fd, " => ", 4);
		write(fd, i->value, strlen(i->value));
		write(fd, "\n", 1);
	}
}

static int	set_hook	( int argc, char *argv[] )
{
	option_t *i;

	if(argc == 3) {
		set_option(argv[1], argv[2]);
		return 0;
	}

	if(argc == 2) {
		i = hash_find_nc(options, argv[1], option_t *);

		if(!i) {
			cio_out("%s is not set\n", argv[1]);
			return 0;
		}

		cio_out("%*s         \"%s\"\n", 20, key(i), i->value);
		return 0;
	}

	for(i = hash_first(options, option_t *); i != NULL; i = hash_next(options, i, option_t *)) {
		cio_out("%*s         \"%s\"\n", 20, key(i), i->value);
	}

	return 0;
}

static	int	unset_hook	( int argc, char *argv[] )
{
	set_option(argv[1], "");
	return 0;
}

void	set_option	( const char *name, const char *value )
{
	option_t *o = hash_find_nc(options, name, option_t *);

	if(!o) {
		o = malloc(sizeof(option_t));
		o->value = NULL;
		hash_insert(options, o, name);
	}

	if(o->value)
		free(o->value);

	o->value = malloc(strlen(value) + 1);
	strcpy(o->value, value);
}

int	bool_option	( const char *name )
{
	option_t *o = hash_find_nc(options, name, option_t *);

	if(!o) {
		return 0;
	}

	if(
			!strcmp("0", o->value) ||
			!strcmp_nc("false", o->value) ||
			!strcmp_nc("no", o->value) ||
			!strcmp_nc("off", o->value) ||
			!strcmp_nc("", o->value)
	  )
		return 0;

	return 1;
}

const char *	string_option	( const char *name )
{
	option_t *o = hash_find_nc(options, name, option_t *);

	if(!o || !strcmp(o->value, ""))
		return NULL;

	return o->value;
}

const char *	safe_string_option	( const char *name )
{
	option_t *o = hash_find_nc(options, name, option_t *);

	if(!o || !strcmp(o->value, ""))
		return "";

	return o->value;
}

int		int_option	( const char *name )
{
	option_t *o = hash_find_nc(options, name, option_t *);

	if(!o)
		return 0;

	return atoi(o->value);
}
