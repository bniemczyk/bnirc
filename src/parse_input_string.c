/*
 * Copyright (C) 2004 Brandon Niemczyk
 *
 * DESCRIPTION:
 *  Defines Basic hooks, as well as:
 *
 *  register_command_list()
 *  unregister_command_list()
 *  parse_input_string()
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

static command_t commands[];

static int help_hook(int argc, char *argv[]);

static int quit_hook(int argc, char *argv[])
{
	switch (argc) {
	case 1:
		if (!string_option("quit_string"))
			irc_out("QUIT\n");
		else
			irc_out("QUIT :%s\n", string_option("quit_string"));
		break;
	case 2:
		irc_out("QUIT :%s\n", argv[1]);
		break;
	default:
		assert(!"code should never get here");
	}

	irc_shutdown = 1;
	return 0;
}

static char **rgxignores = NULL;
static int rgxignores_alloced = 0;
static int rgxignores_len = 0;

int rgxignore_line(const char *line)
{
	int i;
	const char *buf;
	for (i = 0; i < rgxignores_len; i++) {
		if (rgxignores[i] == NULL)
			continue;
		if (regex_search
		    (&buf, RGX_DEFAULT | RGX_IGNORE_CASE, rgxignores[i], line))
			return 1;
	}
	return 0;
}

static int rgxignore_hook(int argc, char *argv[])
{
	int i;
	// if argc == 1, then just list them all
	if (argc == 1) {
		for (i = 0; i < rgxignores_len; i++) {
			if (rgxignores[i] == NULL)
				continue;
			io_colored_out(USER_RED, "ignoring: ");
			io_colored_out(USER_WHITE, "[%d] %s\n", i + 1,
				       rgxignores[i]);
		}
		return 0;
	}
	// get the index of the new ignore
	if (rgxignores_len == rgxignores_alloced) {
		if (rgxignores_alloced == 0)
			rgxignores_alloced = 12;
		else
			rgxignores_alloced <<= 1;
		rgxignores =
		    realloc(rgxignores, rgxignores_alloced * sizeof(char *));
	}

	i = rgxignores_len++;

	// set it to argv[1]
	rgxignores[i] = strdup(argv[1]);
	return 0;
}

static int rgxunignore_hook(int argc, char *argv[])
{
	int index = atoi(argv[1]) - 1;
	if (index < 0 || index >= rgxignores_len || rgxignores[index] == NULL) {
		cio_out("rgxignore %d does not exist", index - 1);
		return 1;
	}

	free(rgxignores[index]);
	rgxignores[index] = NULL;
	return 0;
}

static char **rgxhilights = NULL;
static int rgxhilights_alloced = 0;
static int rgxhilights_len = 0;

int rgxhilight_line(const char *line)
{
	int i;
	const char *buf;
	for (i = 0; i < rgxhilights_len; i++) {
		if (rgxhilights[i] == NULL)
			continue;
		if (regex_search
		    (&buf, RGX_DEFAULT | RGX_IGNORE_CASE, rgxhilights[i], line))
			return 1;
	}
	return 0;
}

static int rgxhilight_hook(int argc, char *argv[])
{
	int i;
	// if argc == 1, then just list them all
	if (argc == 1) {
		for (i = 0; i < rgxhilights_len; i++) {
			if (rgxhilights[i] == NULL)
				continue;
			io_colored_out(USER_RED, "hilighting: ");
			io_colored_out(USER_WHITE, "[%d] %s\n", i + 1,
				       rgxhilights[i]);
		}
		return 0;
	}
	// get the index of the new hilight
	if (rgxhilights_len == rgxhilights_alloced) {
		if (rgxhilights_alloced == 0)
			rgxhilights_alloced = 12;
		else
			rgxhilights_alloced <<= 1;
		rgxhilights =
		    realloc(rgxhilights, rgxhilights_alloced * sizeof(char *));
	}

	i = rgxhilights_len++;

	// set it to argv[1]
	rgxhilights[i] = strdup(argv[1]);
	return 0;
}

static int rgxunhilight_hook(int argc, char *argv[])
{
	int index = atoi(argv[1]) - 1;
	if (index < 0 || index >= rgxhilights_len || rgxhilights[index] == NULL) {
		cio_out("rgxhilight %d does not exist", index - 1);
		return 1;
	}

	free(rgxhilights[index]);
	rgxhilights[index] = NULL;
	return 0;
}

static int clear_hook(int argc, char *argv[])
{
	io_clear_screen(active_window);
	return 0;
}

static int load_hook(int argc, char *argv[])
{
	switch (argc) {
	case 1:
		plugin_list();
		break;
	case 2:
		load_plugin(argv[1]);
		break;
	default:
		assert(!"code should never get here");
	}
	return 0;
}

static int unload_hook(int argc, char *argv[])
{
	unload_plugin(argv[1]);
	return 0;
}

static int window_hook(int argc, char *argv[])
{
	window_t w = atoi(argv[1]);
	w--;
	if (w > window_max || w < 0)
		cio_out("please select a window between 1 and %d\n",
			window_max);
	else
		set_active_window(w);
	return 0;
}

static int readme_hook(int argc, char *argv[])
{
	io_colored_out(USER_RED, "*** beginning README ***\n");
	cio_out("%s", README);
	io_colored_out(USER_RED, "*** ending README ***\n");
	return 0;
}

static int faq_hook(int argc, char *argv[])
{
	io_colored_out(USER_RED, "*** beginning FAQ ***\n");
	cio_out("%s", FAQ);
	io_colored_out(USER_RED, "*** ending FAQ ***\n");
	return 0;
}

static int urlize_hook(int argc, char *argv[])
{
	io_colored_out(USER_WHITE, "%s\n", urlize(argv[1]));
	return 0;
}

static int logo_hook(int argc, char *argv[])
{
	const char *fname = string_option("logo_file");
	if (!fname) {
		cio_out("no logo file known!\n");
		return -1;
	}

	FILE *f = bnirc_fopen(fname, "r");
	if (!f)
		return -1;

	char *buf = NULL;
	size_t size = 0;

	io_out("\n");

	while (1) {
		bnirc_getline(&buf, &size, f);

		if (feof(f) || ferror(f))
			break;

		io_colored_out(USER_RED, "%s", buf);
	}

	io_out("\n\n");

	fclose(f);

	return 0;
}

static int reload_hook(int argc, char *argv[])
{
	if (argc == 1) {
		reload_all_plugins();
		return 0;
	}

	reload_plugin(argv[1]);
	return 0;
}

/**********************************************
 * FIXME:
 *  this really should go away and be replaced
 *  with a string_option()
 **********************************************/
static int trailchar_hook(int argc, char *argv[])
{
	if (argc == 1) {
		trail_char[0] = 0;
		return 0;
	}

	if (strlen(argv[1]) != 3 || argv[1][0] != '\'' || argv[1][2] != '\'') {
		io_colored_out(USER_RED,
			       "must be one character surrounded by ''\n");
		return -1;
	}

	trail_char[0] = argv[1][1];
	return 0;
}

static int source_hook(int argc, char *argv[])
{
	FILE *f = bnirc_fopen(argv[1], "r");
	if (!f) {
		io_colored_out(USER_RED, "could not open %s\n", argv[1]);
		return -1;
	}

	char *line = NULL;
	size_t s = 0;

	while (1) {
		bnirc_getline(&line, &s, f);
		if (feof(f) || ferror(f))
			break;
		parse_input_string(line);
	}

	fclose(f);

	cio_out("%s parsed\n", argv[1]);
	return 0;
}

static int exec_hook(int argc, char *argv[])
{
	char *buf = NULL;
	size_t buf_size = 0;
	FILE *f = NULL;
	char *i;
	char *tmp;

	if (!strncmp(argv[1], "-o ", 3)) {
		i = &argv[1][3];
		while (*i != 0 && (*i == ' ' || *i == '\t'))
			i++;

		io_colored_out(USER_RED, "exec -o [");
		io_colored_out(USER_WHITE, "%s", i);
		io_colored_out(USER_RED, "]\n");

		tmp = malloc(strlen(i) + strlen(" 2>/dev/null") + 1);
		sprintf(tmp, "%s 2>/dev/null", i);
		f = popen(tmp, "r");
		free(tmp);
		while (bnirc_getline(&buf, &buf_size, f)) {
			if (feof(f) || ferror(f))
				break;

			// kill the \n
			buf[strlen(buf) - 1] = 0;
			irc_say(buf);
		}
	} else {
		tmp = malloc(strlen(argv[1]) + strlen(" 2>/dev/null") + 1);
		sprintf(tmp, "%s 2>/dev/null", argv[1]);
		f = popen(tmp, "r");
		free(tmp);
		io_colored_out(USER_RED, "exec [");
		io_colored_out(USER_WHITE, "%s", argv[1]);
		io_colored_out(USER_RED, "]\n");
		while (bnirc_getline(&buf, &buf_size, f)) {
			if (feof(f) || ferror(f))
				break;
			io_colored_out(USER_WHITE, "%s", buf);
		}
	}

	if (buf != NULL)
		free(buf);
	return 0;
}

static int alias_hook(int argc, char *argv[]);

static int ping_hook(int argc, char *argv[]) {
	irc_out("PING %s\n", argv[1]);
	user_wants_pong = 1;
	return 0;
}

/**********************************************
 * the basic commands that should _always_ be
 * available before any plugins are loaded
 **********************************************/
static command_t commands[] = {
	{"exec", "exec [-o] <command>", 1, 2, exec_hook,
	 "execute a command and put the output into your main window, if the -o option used outpur will instead be "
	 "redirected to the channel"},

	{"ping", "ping <string>", 2, 2, ping_hook,
	 "ping the server" },

	{"alias", "alias [alias_name] [command]", 1, 3, alias_hook,
	 "sets an alias for a command, or views an existing alias"},

	{"trailchar", "trailchar ['<yourchar>']", 1, 2, trailchar_hook,
	 "sets the trailing character to be added after a tab complete, must be a single character surrounded with ''"
	 ", if there is no argument, it sets it to no trailing character"},

	{"clear", "clear", 1, 1, clear_hook, "clear your screen"},

	{"window", "window <window number>", 2, 2, window_hook,
	 "switch your active window"},

	{"win", "win <window number>", 2, 2, window_hook,
	 "switch your active window"},

	{"quit", "quit [quit message]", 1, 2, quit_hook, "quit bnIRC"},

	{"load", "load [plugin]", 1, 2, load_hook,
	 "load a plugin, if no plugin name is given /load lists all currently loaded plugins"},

	{"unload", "unload <plugin name or filename>", 2, 2, unload_hook,
	 "unload a plugin"},

	{"reload", "reload [plugin name]", 1, 2, reload_hook,
	 "reload a plugin, if [plugin] is not passed, all plugins will be reloaded"},

	{"readme", "readme", 1, 1, readme_hook,
	 "display the README that comes with bnirc"},

	{"faq", "faq", 1, 1, faq_hook, "display the FAQ that comes with bnirc"},

	{"logo", "logo", 1, 1, logo_hook, "display the logo"},

	{"help", "help [command]", 1, 2, help_hook, "this command"},

	{"source", "source <filename>", 2, 2, source_hook,
	 "reads in a file and parses it like user input"},

	{"urlize", "urlize <string>", 2, 2, urlize_hook,
	 "makes a string useable for a url"},

	{"rgxignore", "rgxignore [regex]", 1, 2, rgxignore_hook,
	 "ignore all output that matches the regex, if no regex is given, it shows"
	 " a list of all ignored regex's"},

	{"hilight", "hilight [regex]", 1, 2, rgxhilight_hook,
	 "higlight all output that matches the regex, if none is given, so all regexes"},

	{"rgxunignore", "rgxunignore <index>", 2, 2, rgxunignore_hook,
	 "erase regex from ignore list"},

	{"unhilight", "unhilight <index>", 2, 2, rgxunhilight_hook,
	 "erase regex from hilight list"},

	{NULL, NULL, 0, 0, NULL}
};

typedef struct {
	hash_header_t head;
	command_t *command;
} com_entry_t;

typedef struct {
	hash_header_t head;
	char *replace;
} alias_t;

static hash_t head;
static hash_t alias_head;

/**********************************************
 * register_command_list needs to be available
 * durning INIT_CODE, so we do this a bit
 * differently
 **********************************************/
static void my_init(void)
{
	static int do_init = 1;
	if (!do_init)
		return;

	do_init = 0;
	hash_init(head);
	hash_init(alias_head);
	register_command_list(commands);
}

static tablist_t *tab_commands(const char *partial)
{
	tablist_t *rv = NULL, *tmp;
	com_entry_t *i;
	char *new;

	const char *cmd_char = safe_string_option("command_char");
	if (!strcmp("", cmd_char))
		cmd_char = "/";

	if (strncmp
	    (cmd_char, partial,
	     strlen(partial) >
	     strlen(cmd_char) ? strlen(cmd_char) : strlen(partial))) {
		return NULL;
	}

	for (i = hash_first(head, com_entry_t *); i != NULL;
	     i = hash_next(head, i, com_entry_t *)) {
		new = malloc(strlen(cmd_char) + strlen(i->command->cmd) + 1);
		strcpy(new, cmd_char);
		strcat(new, i->command->cmd);
		if (strncasecmp(partial, new, strlen(partial))) {
			free(new);
			continue;
		}
		tmp = malloc(sizeof *tmp);
		list_init(tmp);
		tmp->word = new;
		tablist_add(rv, tmp);
	}

	return rv;
}

static tablist_t *alias_tabs(const char *partial)
{
	tablist_t *list = NULL;
	alias_t *i;

	const char *cmd_char = string_option("command_char");

	if (strncmp(cmd_char, partial,
		    strlen(cmd_char) > strlen(partial) ?
		    strlen(partial) : strlen(cmd_char)
	    ))
	return list;

	partial += strlen(cmd_char);

	for (i = hash_first(alias_head, alias_t *); i != NULL;
	     i = hash_next(alias_head, i, alias_t *)) {
		if (strncasecmp(partial, key(i), strlen(partial)))
			continue;
		tablist_t *tmp = malloc(sizeof *tmp);
		list_init(tmp);
		// tmp->word = strdup(key(i));
		tmp->word = malloc(strlen(cmd_char) + strlen(key(i)) + 1);
		strcpy(tmp->word, cmd_char);
		strcat(tmp->word, key(i));
		tablist_add(list, tmp);
	}

	return list;
}

INIT_CODE(add_tab_complete)
{
	add_tab_completes(tab_commands);
	add_tab_completes(alias_tabs);
}

static int alias_hook(int argc, char *argv[])
{
	my_init();
	alias_t *buf;

	if (argc == 1) {
		for (buf = hash_first(alias_head, alias_t *); buf != NULL;
		     buf = hash_next(alias_head, buf, alias_t *)) {
			io_colored_out(USER_GREEN, "/%s -> /%s\n", key(buf),
				       buf->replace);
		}
		return 0;
	}

	if (argc == 2) {
		buf = hash_find_nc(alias_head, argv[1], alias_t *);
		if (!buf) {
			io_colored_out(USER_GREEN, "%s is not a known alias\n",
				       argv[1]);
			return 0;
		} else {
			io_colored_out(USER_GREEN, "%s -> %s\n", key(buf),
				       buf->replace);
			return 0;
		}
	}

	if ((buf = hash_find_nc(alias_head, argv[1], alias_t *)) != NULL) {
		free(buf->replace);
		buf->replace = copy_string(argv[2]);
		return 0;
	}

	buf = malloc(sizeof(alias_t));
	buf->replace = copy_string(argv[2]);
	hash_insert(alias_head, buf, argv[1]);

	return 0;
}

/**********************************************
 * INIT_CODE() safe function to register a set
 **********************************************/
void register_command_list(command_t * c)
{
	my_init();
	command_t *i = c;
	com_entry_t *buf;
	com_entry_t *tmp;

	for (i = c; i != NULL && i->cmd != NULL; i++) {
		buf = malloc(sizeof(com_entry_t));
		buf->command = i;

		if ((tmp = hash_find_nc(head, i->cmd, com_entry_t *)) != NULL)
			hash_remove(tmp);

		hash_insert(head, buf, i->cmd);
	}
}

/**********************************************
 * INIT_CODE() safe, although i don't know why
 * it needs to be...
 **********************************************/
void unregister_command_list(command_t * c)
{
	my_init();
	command_t *i;
	com_entry_t *buf;

	for (i = c; i != NULL && i->cmd != NULL; i++) {
		buf = hash_find_nc(head, i->cmd, com_entry_t *);
		if (!buf)
			continue;
		hash_remove(buf);
		free(buf);
	}
}

static int help_hook(int argc, char *argv[])
{
	my_init();
	com_entry_t *i;

	if (argc == 2) {
		if (!strncmp(argv[1], safe_string_option("command_char"),
			     strlen(safe_string_option("command_char")))) {
			argv[1] += strlen(safe_string_option("command_char"));
		}

		i = hash_find_nc(head, argv[1], com_entry_t *);

		if (!i) {
			io_colored_out(USER_RED, "unknown command\n");
			return -1;
		}

		if (i->command->cmd)
			io_colored_out(USER_GREEN, "%*s", 16, i->command->cmd);
		if (i->command->usage)
			io_colored_out(USER_WHITE, "%*s %s%s\n", 10, "usage: ",
				       string_option("command_char"),
				       i->command->usage);
		if (i->command->description)
			io_colored_out(USER_RED, "description: %s\n",
				       i->command->description);

		return 0;
	}

	for (i = hash_first(head, com_entry_t *); i != NULL;
	     i = hash_next(head, i, com_entry_t *)) {
		if (i->command->cmd)
			io_colored_out(USER_GREEN, "%*s", 16, i->command->cmd);
		if (i->command->usage)
			io_colored_out(USER_WHITE, "%*s %s%s\n", 10, "usage: ",
				       string_option("command_char"),
				       i->command->usage);
	}

	return 0;
}

const char *current_input_string = NULL;
int use_comments = 0;

typedef struct {
	list_t header;
	int (*func) (char *);
} parser_t;

parser_t parser_head;

INIT_CODE(init_parser_stack)
{
	list_init(&parser_head);
}

void push_input_parser(int (*func) (char *))
{
	parser_t *p = malloc(sizeof *p);
	p->func = func;
	push(&parser_head, p);
}

void pop_input_parser(void)
{
	parser_t *p = top(&parser_head);
	if (!p)
		return;
	pop(&parser_head);
	free(p);
}

int parse_input_string(char *str)
{
	my_init();
	const char *command_char = "/";
	int cmdchr_len = 1;
	char **argv;
	int argc;
	char *buf;
	alias_t *alias;
	parser_t *tmp;

	if (!str) {
		return -1;
	}

	if (!list_empty(&parser_head)) {
		tmp = top(&parser_head);
		argc = tmp->func(str);
		return argc;
	}

	if (strlen(str) == 0) {
		return 0;
	}

	if (use_comments && str[0] == '#') {
		return 0;
	}

	if (str[0] == '\n') {
		return 0;
	}

	current_input_string = str;

	// strip the newline
	str[strlen(str) - 1] = 0;

	// strip extra whitespace
	while (str[strlen(str) - 1] == ' ')
		str[strlen(str) - 1] = 0;

	if (bool_option("verbose")) {
		cio_out("parsing input string[%s]\n", str);
	}

	if (bool_option("command_char")) {
		command_char = (char *) string_option("command_char");
		cmdchr_len = strlen(command_char);
	}

	if (strncasecmp(command_char, str, cmdchr_len)) {
		irc_say(str);
		return 0;
	}

	buf = malloc(strlen(str) + 1);
	strcpy(buf, str);
	buf += cmdchr_len;

	char **buf_argv;
	int buf_argc;

	make_argv(&buf_argc, &buf_argv, buf);

	if (buf_argc == 0)
		goto parse_input_string_out;

	if ((alias = hash_find_nc(alias_head, buf_argv[0], alias_t *)) != NULL) {
		buf -= cmdchr_len;
		free(buf);
		buf = string_replace(str, buf_argv[0], alias->replace);
		// buf_argv[0] = copy_string(alias->replace);
		buf += cmdchr_len;
		free_argv(buf_argc, buf_argv);
		make_argv(&buf_argc, &buf_argv, buf);
	}

	com_entry_t *e = hash_find_nc(head, buf_argv[0], com_entry_t *);

	if (!e) {
		irc_out("%s\n", buf);
		goto parse_input_string_out;
	}

	make_max_argv(&argc, &argv, buf, e->command->max_argc);

	if (argc < e->command->min_argc) {
		cio_out("usage: /%s\n", e->command->usage);
		goto parse_input_string_out;
	}

	e->command->hook(argc, argv);

      parse_input_string_out:
	free_argv(buf_argc, buf_argv);
	buf -= cmdchr_len;
	free(buf);
	current_input_string = NULL;
	return 0;
}
