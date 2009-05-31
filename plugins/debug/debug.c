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

static FILE *out = NULL;

static int	hello	( int argc, char *argv[] )
{
	if(argc < 2) {
		cio_out("usage: /load %s <filename>\n", argv[0]);
		return 1;
	}

	out = fopen(argv[1], "w");
	if(!out) {
		cio_out("could not open %s for write!\n", argv[0]);
		return 1;
	}

	cio_out("logging debug info to %s\n", argv[1]);
	return 0;
}

static int goodbye	( void )
{
	cio_out("done logging debug info\n");
	fclose(out);
	return 0;
}

static int input_hook		( char *cdata )
{
	fprintf(out, "USER: %s", cdata);
	fflush(out);
	return 0;
}

static int irc_hook		( char *cdata )
{
	fprintf(out, "SERVER: %s", cdata);
	fflush(out);
	return 0;
}

static plugin_t	plugin = {
	.name		= "debug",
	.description	= "logs all input from user and irc server",
	.version	= "0.0.1-1",
	.plugin_init	= hello,
	.plugin_cleanup = goodbye,
	.irc_hook	= irc_hook,
	.input_hook	= input_hook
};

REGISTER_PLUGIN(plugin, bnirc_debug);
