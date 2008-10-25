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

typedef struct {
	list_t header;
	user_oo_t *u;
} user_oo_list_t;

hash_t users_hash;

INIT_CODE(users_init)
{
	hash_init(users_hash);
	cio_out("user.c initialized!\n");
}

/* forward decls */
static void set_queue(user_oo_t * u, const char *h);

/* @DISABLED@
static void action_print(user_t * u, const char *dest, const char *msg)
{
	if (strlen(msg) < 9)
		return;

	char *act = copy_string(msg);
	/COMMENT* kill the CTCP quotes and ACTION keyword *COMMENT/
	act[strlen(act) - 1] = 0;
	act += 8;
	window_t w;

	/COMMENT* calculate window *COMMENT/
	if (*dest == '#') {
		channel_t *ctmp = find_channel(dest, u->server);
		if (ctmp == NULL) {
			if (bool_option("verbose")) {
				cio_out
				    ("server being bad: recieved actin from [%s] to [%s]\n",
				     u->name, dest);
			}
			goto action_print_out;
		}
		w = ctmp->win;
	} else {
		w = active_window;
	}

	cwio_out(w, "%s %s/%s %s\n", timestamp(), u->name, dest, act);

      action_print_out:
	act -= 8;
	free(act);
}
 @DISABLED@ */

void privmsg_print(user_t * u, const char *dest, const char *msg)
{
	user_t *ud = NULL;
	channel_t *cd = NULL;
	int color = 0;
	window_t w;

	if (u->flags & USER_IGNORE || rgxignore_line(msg))
		return;		// fuck em

	if (msg[0] == 1) {
		plugin_ctcp_in_hook(u->name, dest, msg, u->server);
		return;
	}

	if (*dest == '#') {
		cd = find_channel(dest, u->server);
		if (cd == NULL) {
			if (bool_option("verbose")) {
				cio_out
				    ("server behaving badly: msg to [%s:%s] recieved\n",
				     dest, u->server);
			}
			return;
		}

		if (bool_option("log")) {
			cd->log(cd, u->name, msg);
		}
	} else {
		ud = find_user(dest, u->server);
		if (ud == NULL) {
			if (bool_option("verbose")) {
				cio_out
				    ("server behaving badly: msg to [%s:%s] recieved\n",
				     dest, u->server);
			}
			return;
		}
	}

	/* get window and print prefix */
	if (ud != NULL) {
		w = active_window;
		cwio_out(w, "%s ", timestamp());
		if (u->flags & USER_IS_SELF)
			wio_colored_out(w, USER_RED, "msgTo(%s): ", dest);
		else
			wio_colored_out(w, USER_RED, "msgFrom(%s): ", u->name);
	} else {
		w = cd->win;
		cwio_out(w, "%s ", timestamp());
		if (cd->is_active(cd))
			wio_colored_out(w, USER_WHITE, "%s: ", u->name);
		else
			wio_colored_out(w, USER_WHITE, "%s/%s: ", u->name,
					cd->name);
	}

	/* should this msg be highlighted? stored? */
	color = u->flags & ALL_USER_COLORS;
	if (bool_option("bell_on_speak") && !(u->flags & USER_IS_SELF)) {
		printf("\a");
	}

	if (strstr_nc(msg, irc_server_nick(u->server)) != NULL ||
			rgxhilight_line(msg)) {
		if (bool_option("verbose")) {
			cio_out("string [%s] contains [%s]\n", msg,
				irc_server_nick(u->server));
		}
		if (!bool_option("bell_on_speak")
		    && bool_option("bell_on_address")) {
			printf("\a");
		}
		if (!color)
			color = USER_WHITE;
		away_handle_msg(u->name, dest, msg);
	} else {
		if (bool_option("verbose")) {
			cio_out("string [%s] does not contain [%s]\n", msg,
				irc_server_nick(u->server));
		}
	}

	/* finally print the msg */
	if (!color) {
		wio_out(w, "%s\n", msg);
	} else {
		wio_colored_out(w, color, "%s\n", msg);
	}

	/* does this user have a queued message? */
	if (u->queue != NULL) {
		char tmp[1024];
		snprintf(tmp, 1023, "private queued msg from [%s]:",
			 irc_server_nick(u->server));
		tmp[1023] = 0;
		irc_msg(u, tmp);
		irc_msg(u, u->queue);
		if (bool_option("tell_queue_sent") || bool_option("verbose")) {
			cio_out("queued msg [%s] for [%s] sent\n", u->queue,
				u->name);
		}
		set_queue(u, NULL);
	}
}


/**********************************************
 * these locking functions are ugly as hell...
 * but user_clear_match() and 
 * user_find_next_match() are not reentrant
 **********************************************/
void user_find_next_lock(void)
{
}

void user_find_next_unlock(void)
{
}

static user_oo_t *matched_user = NULL;

void user_clear_match(void)
{
	matched_user = NULL;
}

/**********************************************
 * this should not be used without locking. an
 * example would be:
 *
 * user_find_next_lock();
 * while(something)
 *   user_find_next_match(partial);
 * user_clear_match();
 * user_find_next_unlock();
 *
 * FIXME: this interface shouldn't be used
 * anymore
 **********************************************/
char *user_find_next_match(const char *partial)
{
	const char *server = irc_pick_server();
	channel_t *ch = active_channel();
	/***************************************************************
	 * in case the last hash_entry matches, thanks zars
	 ***************************************************************/
	static int r_NULL = 0;
	if (!server)
		return NULL;

	if (matched_user == NULL) {
		if(r_NULL) {
			r_NULL = 0;
			return NULL;
		}

		matched_user = hash_first(users_hash, user_oo_t *);
	}

	while (matched_user != NULL) {
		assert(matched_user->server);
		assert(matched_user->name);
		if (!strcasecmp(matched_user->server, server) &&
		    strlen(matched_user->name) >= strlen(partial) &&
		    !strncasecmp(matched_user->name, partial, strlen(partial))
		    && user_in_channel(matched_user, ch->name)
		    ) {
			char *rv = strdup(matched_user->name);
			matched_user =
			    hash_next(users_hash, matched_user, user_oo_t *);
			if(!matched_user)
				r_NULL = 1;
			return rv;
		}

		matched_user = hash_next(users_hash, matched_user, user_oo_t *);
	}

	return NULL;
}

const char **users_in_channel(const char *channel)
{
	int alloced = 12;
	const char **rv = malloc(alloced * sizeof(char *));
	int found = 0;
	user_t *u;
	for (u = hash_first(users_hash, user_t *);
	     u != NULL; u = hash_next(users_hash, u, user_t *)) {
		if (user_in_channel(u, channel)) {
			rv[found++] = u->name;
			rv[found] = NULL;
			if (found >= alloced - 2) {
				alloced <<= 1;
				rv = realloc(rv, alloced * sizeof(char *));
			}
		}
	}

	return rv;
}

tablist_t *user_find_all_matches(const char *partial)
{
	tablist_t *rv = TAB_INIT;
	user_t *i;

	for(i = hash_first(users_hash, user_t *); i != NULL;
			i = hash_next(users_hash, i, user_t *)) {
		if(strncasecmp(i->name, partial, strlen(partial)) ||
				!user_in_channel(i, natural_channel_name()))
			continue;
		tablist_add_word(rv, i->name);
	}

	return rv;
}

INIT_CODE(setup_tab_complete) {
	add_tab_completes(user_find_all_matches);
}

static void rm_channel(user_oo_t * u, const char *chan)
{
	assert(u);
	assert(chan);

	user_chlist_t *j, *i = (user_chlist_t *) (u->chlist.header.next);

	while (i != &(u->chlist)) {
		j = (user_chlist_t *) (i->header.next);
		if (!strcasecmp(chan, i->channel))
			list_remove(i);
		i = j;
	}
}

/**********************************************
 * checks if a user is currently in a channel
 **********************************************/
int user_in_channel(user_oo_t * u, const char *chan)
{
	assert(u);
	assert(chan);

	user_chlist_t *i = &(u->chlist);
	for (list_next(i); i != &(u->chlist); list_next(i)) {
		if (!strcasecmp(i->channel, chan))
			return 1;
	}
	return 0;
}

static void add_channel(user_oo_t * u, const char *chan)
{
	assert(chan != NULL);
	rm_channel(u, chan);

	user_chlist_t *tmp = malloc(sizeof(user_chlist_t));
	tmp->channel = copy_string(chan);

	list_insert(&(u->chlist), tmp);
}

/**********************************************
 * to be called whenever you want to change
 * the nickname of a user, this is called
 * recv_nick, because it *usually* should only
 * be called when a NICK line was recieved
 * from the server
 **********************************************/
static void recv_nick(user_oo_t * user, const char *nick)
{
	assert(user != NULL);
	assert(user->name != NULL);
	assert(user->server != NULL);
	assert(nick != NULL);

	user->print_relevant_wins(user,
				  "%s is now known as %s [%s]\n",
				  user->name, nick, user->server);

	user->name = copy_string(nick);

	char *key = malloc(strlen(nick) + strlen(user->server) + 2);
	sprintf(key, "%s:%s", nick, user->server);
	user_t *tmp = hash_find_nc(users_hash, key, user_t *);
	if (tmp != NULL)
		hash_remove(tmp);

	hash_remove(user);
	hash_insert(users_hash, user, key);
	free(key);
}

static void speak_hook(user_oo_t * user)
{
	assert(user);
	if (user->queue) {
		io_colored_out(USER_RED, "delivering queued message [");
		io_colored_out(USER_WHITE, "%s", user->queue);
		io_colored_out(USER_RED, "] %s\n", user->name);
		irc_msg(user, user->queue);
		free(user->queue);
	}
}

static void set_host(user_oo_t * u, const char *h)
{
	assert(u);
	if (u->host)
		free(u->host);
	if (h == NULL)
		u->host = NULL;
	else
		u->host = copy_string(h);
}

static void set_queue(user_oo_t * u, const char *h)
{
	assert(u);
	if (u->queue)
		free(u->queue);
	if (h == NULL)
		u->queue = NULL;
	else
		u->queue = copy_string(h);
}

static void print_relevant_wins(user_oo_t * u, const char *fmt, ...)
{
	window_t wins[255] = { 0 };
	user_chlist_t *i = (user_chlist_t *) u->chlist.header.next;
	while (i != &(u->chlist)) {
		channel_t *tmp = find_channel(i->channel, u->server);
		if (!tmp || wins[tmp->win]) {
			list_next(i);
			continue;
		}
		va_list ap;
		va_start(ap, fmt);
		cvwio_out(tmp->win, fmt, ap);
		va_end(ap);
		wins[tmp->win] = 1;
	}
}

void construct_user(user_oo_t * user, const char *name, const char *server)
{
	char *key = malloc(strlen(name) + strlen(server) + 2);
	strcpy(key, name);
	strcat(key, ":");
	strcat(key, server);
	hash_insert(users_hash, user, key);

	assert(hash_find_nc(users_hash, key, void *));

	if (bool_option("verbose")) {
		cio_out("constructing user with key [%s]\n", key);
	}

	user->name = copy_string(name);
	user->server = copy_string(server);
	user->flags = 0;
	user->queue = NULL;
	user->host = NULL;
	user->add_channel = add_channel;
	user->rm_channel = rm_channel;
	user->set_host = set_host;
	user->recv_nick = recv_nick;
	user->speak_hook = speak_hook;
	user->set_queue = set_queue;
	user->set_host = set_host;
	user->print_relevant_wins = print_relevant_wins;
	user->privmsg_print = privmsg_print;
	list_init(&(user->chlist));

	free(key);
}

void destruct_user(user_oo_t * user)
{
	if (bool_option("verbose"))
		cio_out("destructing user with key [%s]\n", key(user));

	hash_remove(user);

	free(user->name);
	free(user->server);

	user_chlist_t *j, *i = (user_chlist_t *) user->chlist.header.next;
	while (i != &(user->chlist)) {
		j = (user_chlist_t *) i->header.next;
		list_remove(i);
		i = j;
	}
}

user_oo_t *new_user(const char *name, const char *server)
{
	user_oo_t *u = malloc(sizeof(user_oo_t));
	construct_user(u, name, server);
	return u;
}

void delete_user(user_oo_t * u)
{
	destruct_user(u);
	free(u);
}

user_oo_t *find_user(const char *name, const char *server)
{
	char *key = malloc(strlen(name) + strlen(server) + 2);
	strcpy(key, name);
	strcat(key, ":");
	strcat(key, server);
	if (bool_option("verbose"))
		cio_out("searching for user with key [%s]\n", key);

	user_oo_t *u = hash_find_nc(users_hash, key, user_oo_t *);

	free(key);

	if (u == NULL)
		return new_user(name, server);

	return u;
}
