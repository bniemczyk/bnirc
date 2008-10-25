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

#ifndef _BNIRC_CHANNEL_H
#define _BNIRC_CHANNEL_H

#include "irc.h"

extern	hash_t	channels_hash;

typedef struct channel_oo {
	hash_entry_t	header;
	char		*name;
	char		*server;
	int		flags;
	window_t	win;
	void(*join)(struct channel_oo *);
	void(*part)(struct channel_oo *, const char *);
	void(*recv_join)(struct channel_oo *);
	void(*recv_part)(struct channel_oo *);
	void(*set_window)(struct channel_oo *, window_t w);
	void(*log)(struct channel_oo *, const char *who, const char *msg);
	int(*is_active)(struct channel_oo *);
	int		state;
	FILE *logfile;
} channel_oo_t, channel_t;

extern	void		construct_channel	( channel_oo_t *chan, const char *name, const char *server );
extern	void		destruct_channel	( channel_oo_t *chan );
extern	channel_oo_t	*new_channel		( const char *name, const char *server );
extern	void		delete_channel		( channel_oo_t * );

extern	channel_oo_t	*find_channel		( const char *name, const char *server );
extern	channel_oo_t	*active_channel		( void );
extern	const char *	natural_channel_name	( void );
extern	void		wall			( const char *msg );
extern void rejoin_channels ( const char *server );

#endif
