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

/* send output to the bitbucket */
static	int my_putc		( int c )
{
	return 0;
}

static	int	init	( void )
{
	// daemonize
	if(fork())
		exit(0);
	return 0;
}

static	io_driver_t	my_io_driver = {
	.putc	= my_putc
};

static	plugin_t	plugin	= {
	.name		= "rserver",
	.version	= "0.0.1",
	.description	= "remote control for bnIRC",
	.io_driver	= &my_io_driver
};

REGISTER_PLUGIN(plugin, rserver);
