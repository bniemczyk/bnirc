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

#include "irc.h"
#include <stdint.h>

static hash_t connections;
char *server_name;

static void irc_recv_callback(client_line_connection_t *, char *);

typedef struct {
	irc_server_t *server;
	char *name;
} query_t;

static void timed_pings(void *discard)
{
	irc_server_t *i;

	for(i = hash_first(connections, irc_server_t *); i != NULL;
			i = hash_next(connections, i, irc_server_t *)) {
		irc_sout(key(i), "PING %s\n", key(i));
	}

	add_timed_func(10000, timed_pings, NULL);

	if(bool_option("debug_pings"))
		cio_out("%s: sending timed ping\n", timestamp());
}

INIT_CODE(my_init)
{
	hash_init(connections);
	cio_out("network code initialized\n");
}

query_t *query = NULL;

void do_query(const char *name)
{
	const char *servername = irc_pick_server();

	user_t *u = find_user(name, servername);

	if (query)
		free(query);

	query = malloc(sizeof(query_t));
	query->name = copy_string(u->name);
	query->server = hash_find_nc(connections, servername, irc_server_t *);
}

const char *current_query(void)
{
	if (query == NULL)
		return NULL;
	return query->name;
}

const char *irc_server_nick(const char *server)
{
	irc_server_t *serv = hash_find_nc(connections, server, irc_server_t *);
	if (!serv)
		return NULL;
	if (!serv->user)
		return NULL;

	if (bool_option("verbose"))
		cio_out("irc_server_nick() returning [%s]\n", serv->user->name);

	return serv->user->name;
}

user_t *server_user(const char *server)
{
	irc_server_t *serv = hash_find_nc(connections, server, irc_server_t *);
	if (!serv)
		return NULL;
	return serv->user;
}

void do_unquery(void)
{
	if (!query)
		return;

	free(query);
	query = NULL;
}

/*
 * FIXME:
 * 	fugly hack, a better way to do this
 * 	would be to have irc_out queue up a 
 * 	mssage with a priority, and then have
 * 	another thread send them out
 *
 * 	Another good way would be to move this to poll.c
 * 	with a poll_write() function of some sort...
 * 	then ratelimiting could be handled in a non-blocking manner..
 */
static void ratelimit(void)
{

	/**********************************************
	 * FIXME:
	 * ugh... i hate macros like this, but i do
	 * not know a better way, the current way is a
	 * lot like being on the recieving end of a
	 * massive ass fucking
	 **********************************************/
#define TIME_OK(a, b) ( \
		a.tv_sec == b.tv_sec \
			? a.tv_usec - b.tv_usec > 333333  \
				? 1  \
				: 0 \
			: a.tv_usec < 666666  \
				? 1 \
				: a.tv_usec - b.tv_usec < 666666  \
					? 1 \
					: 0 \
		)

	static struct timeval last_time = { 0 };
	struct timeval buf;

	while (1) {
		gettimeofday(&buf, NULL);

		if (TIME_OK(buf, last_time))
			break;

		bnirc_usleep(10000);
	}

	last_time.tv_sec = buf.tv_sec;
	last_time.tv_usec = buf.tv_usec;
#undef TIME_OK

}

/**********************************************
* how a server is picked:
*  1) check if current_line_client is non-NULL
*  2) check force_server_respect
*  3) check the active channel
*  4) default to server_name
**********************************************/
const char *irc_pick_server(void)
{
	channel_t *ch = active_channel();

	if (!server_name)
		return NULL;

	irc_server_t *tmp;
	if (current_line_client != NULL) {
		return current_line_client->con.hostname;
	}
	if (bool_option("force_server_respect")) {
		return server_name;
	} else if (ch != NULL && strlen(ch->name)) {
		tmp = hash_find_nc(connections, ch->server, irc_server_t *);
		return tmp->con.con.hostname;
	} else {
		/* this should either be valid, or NULL */
		return server_name;
	}
}

/**********************************************
* how a server is picked:
*  1) check if current_line_client is non-NULL
*  2) check force_server_respect
*  3) check the active channel
*  4) default to server_name
**********************************************/
client_connection_t *irc_pick_server_connection(void)
{
	channel_t *ch = active_channel();

	if (!server_name)
		return NULL;

	irc_server_t *tmp;
	if (current_line_client != NULL) {
		return (client_connection_t *) current_line_client;
	}
	if (bool_option("force_server_respect")) {
		return server_name;
	} else if (ch != NULL && strlen(ch->name)) {
		tmp = hash_find_nc(connections, ch->server, irc_server_t *);
		return &(tmp->con);
	} else {
		/* this should either be valid, or NULL */
		tmp = hash_find_nc(connections, server_name, irc_server_t *);
		return &(tmp->con);
	}
}

const char *current_ip ( void )
{
	struct sockaddr_in addr;
	uint32_t myip;
	static char IP[15];
	socklen_t namelen = sizeof addr;

	const char *server = irc_pick_server();
	if(!server)
		return "";
	irc_server_t *s = hash_find_nc(connections, server, irc_server_t *);
	getsockname(s->con.con.poll.fd, (struct sockaddr *)&addr, &namelen);

	/***************************************************************
	 * FIXME: not portable, will break with ipv6
	 ***************************************************************/
	myip = (uint32_t)addr.sin_addr.s_addr;

	sprintf(IP, "%d.%d.%d.%d",
			(myip & 0x000000ff) >> 0,
			(myip & 0x0000ff00) >> 8,
			(myip & 0x00ff0000) >> 16,
			(myip & 0xff000000) >> 24
	       );

	return IP;
}

const char *irc_get_current_server(void)
{
	channel_t *ch = active_channel();
	if (!ch)
		return NULL;

	return ch->server;
}

const user_t *irc_get_current_user(void)
{
	channel_t *ch = active_channel();
	if (!ch)
		return NULL;

	if (!ch->server)
		return NULL;

	irc_server_t *serv =
	    hash_find_nc(connections, irc_pick_server(), irc_server_t *);
	if (!serv)
		return NULL;

	return serv->user;
}

char *irc_get_current_nick(void)
{
	irc_server_t *serv =
	    hash_find_nc(connections, irc_pick_server(), irc_server_t *);
	if (!serv)
		return NULL;
	if (!serv->user)
		return NULL;
	return serv->user->name;
}

void irc_out(const char *fmt, ...)
{
	// FILE *f;

	irc_server_t *tmp =
	    hash_find_nc(connections, irc_pick_server(), irc_server_t *);

	if (!tmp) {
		cio_out("please connect to a server first!\n");
		return;
	}

	// f = tmp->con.con.poll.out;

	va_list ap;
	va_start(ap, fmt);
	vpollprintf(&(tmp->con.con.poll), fmt, ap);
	va_end(ap);

}

void irc_sout(const char *servername, const char *fmt, ...)
{
	if (server_name == NULL)
		return;

	irc_server_t *server =
	    hash_find_nc(connections, servername, irc_server_t *);
	assert(server != NULL);

	if (server == NULL) {
		cio_out("server %s not connected...\n", servername);
	}

	ratelimit();

	/*
	   if(feof(server->con.con.in) || feof(server->con.con.poll.out)
	   || ferror(server->con.con.in) || ferror(server->con.con.poll.out))
	   irc_connect(6667, NULL);
	 */

	va_list ap;
	va_start(ap, fmt);
	vpollprintf(&(server->con.con.poll), fmt, ap);
	va_end(ap);
}

char channels[12][256] = { "\0" };

/**********************************************
 * returns 0 if active, 1 otherwise
 **********************************************/
int irc_channel_active(const char *str)
{
	channel_t *tmp = find_channel(str, irc_pick_server());
	if (tmp->is_active(tmp))
		return 0;
	return 1;
}

void irc_say(const char *str)
{
	channel_t *ch = active_channel();

	if (ch == NULL)
		return;

	user_t *u = find_user(irc_server_nick(ch->server), ch->server);
	assert(str != NULL);
	assert(u != NULL);
	assert(ch != NULL);

	if ((!ch && !query) || (!ch->name && !query)) {
		cio_out("please join a channel first..\n");
		return;
	} else if (!query) {
		irc_sout(ch->server, "PRIVMSG %s :%s\n", ch->name, str);
		/* channel_msg(irc_server_nick(ch->server), ch->name, ch->server, str); */
		u->privmsg_print(u, ch->name, str);
	} else {
		irc_out("PRIVMSG %s :%s\n", query->name, str);
		/* channel_msg(query->server->user->name, query->name, key(query->server), str); */
		u->privmsg_print(u, query->name, str);
	}
}

void irc_msg(user_t * u, const char *str)
{
	irc_server_t *serv =
	    hash_find_nc(connections, u->server, irc_server_t *);
	user_t *me = find_user(irc_server_nick(u->server), u->server);
	assert(me != NULL);
	assert(serv);

	irc_sout(u->server, "PRIVMSG %s :%s\n", u->name, str);
	/* channel_msg(serv->user->name, u->name, key(serv), (char *)str); */
	u->privmsg_print(me, u->name, str);
}

void irc_command(const char *str)
{
	str++;
	irc_out("%s\n", str);
}

/**********************************************
 * *nick does something
 **********************************************/
void irc_action(const char *str)
{
	const char *channel = natural_channel_name();

	if (!channel || !irc_get_current_nick()) {
		return;
	}

	irc_out("PRIVMSG %s :%cACTION %s%c\n", channel, 1, str, 1);
	cio_out("%s ", timestamp());
	io_colored_out(USER_WHITE, "%s ", irc_get_current_nick());
	cio_out("%s\n", str);
}

void irc_snick(const char *name, irc_server_t * serv)
{
	if (!server_name) {
		cio_out("connect to a server first\n");
		return;
	}

	assert(serv);

	if (serv->user == NULL) {
		if (bool_option("verbose"))
			cio_out("creating initial user on server [%s]\n",
				key(serv));

		serv->user = new_user(name, key(serv));
		serv->user->flags |= USER_IS_SELF;

		if (bool_option("verbose"))
			cio_out("initial user on server [%s] has key [%s]\n",
				key(serv), key(serv->user));
	} else {
        serv->user->name = copy_string(name);
    }

	irc_sout(key(serv), "NICK %s\n", name);
}

void irc_nick(const char *name)
{
	irc_snick(name,
		  hash_find(connections, irc_pick_server(), irc_server_t *));
}

void irc_ctcp(const char *who, const char *str)
{
	irc_out("PRIVMSG %s :%c%s%c\n", who, 1, str, 1);
}

static void irc_recv_callback(client_line_connection_t * server_con, char *ln)
{
	plugin_irc_hook(ln);
	parse_server_string(ln);
}

static void irc_reconnect(client_line_connection_t *);

static void after_connect(irc_server_t * serv)
{
	static int already_pinging = 0;
	char *username = get_username();
	if (bool_option("server_password"))
		pollprintf(&(serv->con.con.poll), "PASS %s\n",
			string_option("server_password"));

	pollprintf(&(serv->con.con.poll), "USER %s 0 * :%s\n",
			bool_option("username") ? string_option("username") : username,
		bool_option("realname") ? string_option("realname") :
		"bnIRC User");

	if (serv->user != NULL) {
		assert(serv->user->name);
		// fprintf(serv->con.con.poll.out, "NICK %s\n", serv->user->name);
		irc_snick(serv->user->name, serv);
	} else if (bool_option("default_nick")) {
		irc_snick(string_option("default_nick"), serv);
	} else {
		irc_snick(get_username(), serv);
	}

	rejoin_channels(serv->con.con.hostname);

	fflush(serv->con.con.poll.out);
	fflush(serv->con.con.poll.in);
	free(username);

	if(!already_pinging) {
		already_pinging = 1;
		add_timed_func(10000, timed_pings, NULL);
	}
}

int irc_disconnect(const char *hostname)
{
    irc_server_t *server = hash_find_nc(connections, hostname, irc_server_t *);
    if(server == NULL) {
        io_colored_out(USER_RED, "Error: Not connected to %s!\n", hostname);
        return -1;
    }

    hash_remove(server);
    unregister_poll(&(server->con.con.poll));
    free(server);
    cio_out("disconnected from %s\n", hostname);

    return 0;
}

void irc_connect(int port, const char *server, int ssl)
{
	if (hash_find_nc(connections, server, void *) != NULL) {
		server_name = copy_string(server);
		cio_out("setting active server [%s]\n", server_name);
		return;
	}

	irc_server_t *new = malloc(sizeof(irc_server_t));
	new->user = NULL;
	client_line_connection_t *ptr = &(new->con);
	construct_client_line_connection(port, server, irc_recv_callback, &ptr, ssl);
	new->con.con.poll.eof_callback = (void (*)(void *)) irc_reconnect;
	new->con.con.poll.error_callback = (void (*)(void *)) irc_reconnect;

	if (ptr == NULL)
		return;

	hash_insert(connections, new, server);
	server_name = copy_string(server);

	after_connect(new);
}

static void irc_reconnect(client_line_connection_t * client_connection)
{
    char *hostname = copy_string(client_connection->con.hostname);
    int port = client_connection->con.port;
    irc_disconnect(hostname);
    int ssl = ((client_connection_t *)client_connection)->poll.is_ssl;
    irc_connect(port, hostname, ssl);
}
