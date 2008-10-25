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


typedef struct {
	hash_entry_t	head;
	client_line_connection_t	con;
	user_t		*user;
} irc_server_t;
