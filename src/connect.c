/*
 * Copyright (C) 2004 Brandon Niemczyk
 * 
 * DESCRIPTION:
 * 	provides network connection wrappers around irc_poll_t 
 *
 * CHANGELOG:
 * 
 * LICENSE: GPL Version 2
 *
 * TODO:
 * 	need to write clean destructors
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

/**********************************************
 * creates a connection and registers it with
 * our polling interface
 **********************************************/
void construct_client_connection(int port, const char *server,
				 void (*callback) (client_connection_t *),
				 client_connection_t ** con)
{
	assert(server);
	assert(port);
	assert(callback);

	int one = 1;
	if (*con == NULL) {
		if (bool_option("verbose"))
			cio_out
			    ("mallocing for connection in construct_client_connection()\n");
		*con = malloc(sizeof(client_connection_t));
	}

	if (bool_option("verbose"))
		cio_out("attempting to resolve [%s]\n", server);

	struct hostent *host = gethostbyname(server);

	if (!host) {
		cio_out("could not resolve %s\n", server);
		*con = NULL;
		return;
	}

	(*con)->address.sin_family = AF_INET;
	(*con)->address.sin_port = htons(port);
	memcpy(&((*con)->address.sin_addr), host->h_addr, host->h_length);

	(*con)->poll.fd = socket(PF_INET, SOCK_STREAM, 0);
	if (connect
	    ((*con)->poll.fd, (struct sockaddr *) &((*con)->address),
	     sizeof(struct sockaddr_in))) {
		cio_out("could not connect to %s!\n", server);
		*con = NULL;
		return;
	}

	setsockopt((*con)->poll.fd, SOL_SOCKET, SO_KEEPALIVE, &one,
		   sizeof(int));

	/**********************************************
	 * we want to reconnect whenever possible...
	 **********************************************/
	(*con)->poll.eof_callback = NULL;
	(*con)->poll.error_callback = NULL;
	(*con)->hostname = copy_string(server);
	(*con)->poll.data = (void *) (*con);
	(*con)->poll.callback = (callback_t) callback;
	if (bool_option("verbose"))
		cio_out("creating connection at %p\n", *con);

	register_poll((irc_poll_t *) (*con));

	cio_out("connected to %s\n", server);
}

volatile client_line_connection_t *current_line_client = NULL;

static void line_callback(client_line_connection_t * con)
{
	assert(con);
	int tmp;
	char *buf;

	if (!(con->buf)) {
		con->buf = malloc(1024);
		con->buf_size = 1024;
	}

	con->buf[con->cur_size] = fgetc(con->con.poll.in);
	con->cur_size += 1;
	con->buf[con->cur_size] = 0;

	if (con->buf_size <= con->cur_size - 1) {
		tmp = (con->cur_size | 1) << 2;
		con->buf = realloc(con->buf, tmp);
		assert(con->buf);
		con->buf_size = tmp;
	}

	if (con->buf[con->cur_size - 1] == '\n'
	    || con->buf[con->cur_size - 1] == '\r') {
		buf = copy_string(con->buf);
		buf[con->cur_size - 1] = '\n';
		con->cur_size = 0;
		current_line_client = con;
		con->callback(con, buf);
		current_line_client = NULL;
		free(buf);
	}
}

void construct_client_line_connection(int port, const char *server,
				      client_line_callback_t callback,
				      client_line_connection_t ** con)
{
	if (*con == NULL) {
		*con = malloc(sizeof(client_line_connection_t));
	}

	if (bool_option("verbose"))
		cio_out("creating line connection %p\n", *con);

	(*con)->callback = (client_line_callback_t) callback;
	(*con)->buf_size = 0;
	(*con)->buf = NULL;
	(*con)->cur_size = 0;
	construct_client_connection(port, server,
				    (client_callback_t) line_callback,
				    (client_connection_t **) con);
}
