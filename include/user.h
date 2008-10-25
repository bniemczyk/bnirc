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

#ifndef _BNIRC_USER_H
#define _BNIRC_USER_H

#include "irc.h"

extern	hash_t	users_hash;

typedef struct {
	list_t header;
	char *channel;
} user_chlist_t;

typedef struct user_oo {
	hash_entry_t	header;
	char		*name;
	char		*server;
	int		flags;
	user_chlist_t	chlist;
	char		*queue;
	char		*host;
	void(*privmsg_print)(struct user_oo *, const char *dest, const char *msg);
	void(*add_channel)(struct user_oo *, const char *chan);
	void(*rm_channel)(struct user_oo *, const char *chan);
	void(*print_relevant_wins)(struct user_oo *, const char *fmt, ...);
	void(*recv_nick)(struct user_oo *, const char *nick);
	void(*speak_hook)(struct user_oo *);
	void(*set_queue)(struct user_oo *, const char *queue);
	void(*set_host)(struct user_oo *, const char *host);
} user_oo_t, user_t;

extern	void		construct_user		( user_oo_t *user, const char *name, const char *server );
extern	void		destruct_user		( user_oo_t *user );
extern	user_oo_t	*new_user		( const char *name, const char *server );
extern	void		delete_user		( user_oo_t * );

extern	user_oo_t	*find_user		( const char *name, const char *server );
extern	user_oo_t	*active_user		( void );

/**********************************************
 * does not change anything! only checks
 * if a user is in the channel
 * returns 1 for true, 0 for false
 **********************************************/
extern int user_in_channel ( user_oo_t *, const char *ch );

/* for tab completion... still fugly */
extern	void		user_find_next_lock	( void );
extern	void		user_find_next_unlock	( void );
extern	void		user_clear_match	( void );
extern	char *		user_find_next_match	( const char *partial );
extern	user_t *	irc_server_user		( void );

extern	void		irc_msg			( user_t *u, const char *msg );

#endif
