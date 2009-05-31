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

static int	hello	( int argc, char *argv[] )
{
	int i;
	cio_out("hello world! argments:");
	for(i = 0; i < argc; i++)
		cio_out(" %s", argv[i]);
	cio_out("\n");

	return 0;
}

static int goodbye	( void )
{
	cio_out("goodbye cruel world!\n");
	return 0;
}

static int input_hook		( char *cdata )
{
	cio_out("hello.c input_hook: %s", cdata);
	return 0;
}

static int irc_hook		( char *cdata )
{
	cio_out("hello.c irc_hook: %s", cdata);
	return 0;
}

static plugin_t	plugin = {
	.name		= "hello",
	.description	= "basic hello world plugin",
	.version	= "0.0.1",
	.plugin_init	= hello,
	.plugin_cleanup = goodbye,
	.irc_hook	= irc_hook,
	.input_hook	= input_hook
};

REGISTER_PLUGIN(plugin, bnirc_hello);
