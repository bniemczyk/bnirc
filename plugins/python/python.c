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

#include "config.h"

/*
 * this needs to be included before any of the standard headers
 * because it effects some macros
 */

#ifdef OSX
#include <python/Python.h>
#else
#include <Python.h>
#endif

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

#define RETURN_NOTHING do { \
	Py_INCREF(Py_None); \
	return Py_None; \
} while(0)

/*
 * my commands hash
 */
static hash_t py_commands;
static hash_t py_server_strings;

typedef struct {
	hash_entry_t header;
	PyObject *func;
	command_t *clist;
} py_command_t;

typedef struct {
	hash_entry_t header;
	PyObject *func;
	server_string_t *slist;
} py_server_string_t;

typedef struct {
	list_t header;
	PyObject *func;
} loop_t, tab_t;

list_t py_loop_list;
list_t py_tab_list;

tablist_t *py_tabs		( const char *partial )
{
	tablist_t *rv = TAB_INIT;
	char *tmp;
	PyObject *list = NULL;
	int len, j;
	tab_t *i;
	PyObject *arg = Py_BuildValue("(s)", partial);


	for(i = list_next_(&py_tab_list); (list_t *)i != &py_tab_list; list_next(i)) {
		list = PyEval_CallObject(i->func, arg);
		if(!list) {
			assert(!"could not run tab completion function");
			PyErr_Print();
			PyErr_Clear();
			return rv;
		}
		len = PyList_Size(list);
		for(j = 0; j < len; j++) {
			tmp = PyString_AsString(PyList_GetItem(list, j));
			if(tmp == NULL)
				continue;
			tablist_add_word(rv, tmp);
		}
	}

	Py_XDECREF(list);
	Py_XDECREF(arg);

	return rv;
}

typedef struct {
	PyObject *func;
	int	fd;
} py_poll_data_t;

static void py_poll_callback ( void *in )
{
	py_poll_data_t *data = in;
	PyObject *arg = Py_BuildValue("()");

	PyObject *result = PyEval_CallObject(data->func, arg);
	if(!result) {
		assert(!"could not run python callback");
		PyErr_Print();
		PyErr_Clear();
	}

	Py_XDECREF(result);
	Py_XDECREF(arg);
}

typedef struct {
	list_t header;
	irc_poll_t poll;
} py_poll_t;

static list_t poll_head;

static PyObject *py_RegisterPoll ( PyObject *self, PyObject *args )
{
	int fd;
	PyObject *func;
	py_poll_t *poll = malloc(sizeof *poll);

	if(!PyArg_ParseTuple(args, "iO", &fd, &func)) {
		assert(!"could not register poll function!");
		RETURN_NOTHING;
	}

	py_poll_data_t *data = malloc(sizeof *data);
	data->func = func;
	data->fd = fd;

	Py_XINCREF(func);

	poll->poll.fd = fd;
	poll->poll.in = fdopen(fd, "r");
	poll->poll.out = fdopen(fd, "w");
	poll->poll.callback = py_poll_callback;
	poll->poll.error_callback = NULL;
	poll->poll.eof_callback = NULL;
	poll->poll.data = data;
	register_poll(&(poll->poll));

	push(&poll_head, poll);

	RETURN_NOTHING;
}

static PyObject *py_UnregisterPoll ( PyObject *self, PyObject *args )
{
	py_poll_t *i, *j;
	int fd;

	if(!PyArg_ParseTuple(args, "i", &fd)) {
		assert(!"could not unregister poll function!");
		RETURN_NOTHING;
	}


	for(i = list_next_(&poll_head); i != (py_poll_t *)&poll_head; list_next(i)) {
		py_poll_data_t *data = i->poll.data;
		if(data->fd == fd) {
			j = list_next_(i);
			list_remove(i);
			unregister_poll(&(i->poll));
			Py_XDECREF(data->func);
			free(data);
			free(i);
			i = j;
			list_prev(i);
		}
	}

	RETURN_NOTHING;
}

/***************************************************************
 * timed functions
 ***************************************************************/
static void	py_timed_func	( void *func )
{
	PyObject *arg = Py_BuildValue("()");
	PyEval_CallObject(func, arg);
	Py_XDECREF((PyObject *)func);
	Py_XDECREF(arg);
}

static PyObject *	py_TimedFunction	( PyObject *self, PyObject *args )
{
	PyObject *func;
	int msecs;

	if(!PyArg_ParseTuple(args, "iO", &msecs, &func)) {
		assert(!"failed to set python timed function!");
		RETURN_NOTHING;
	}

	Py_XINCREF(func);
	add_timed_func(msecs, py_timed_func, func);
	RETURN_NOTHING;
}

static void	py_loops	( void )
{
	loop_t *i;
	PyObject *result = NULL;
	PyObject *arg = Py_BuildValue("()");

	for(i = (loop_t *)py_loop_list.next; i != (loop_t *)&py_loop_list; list_next(i)) {
		result = PyEval_CallObject(i->func, arg);
		if(result == NULL) {
			assert(!"could not run python function");
			PyErr_Print();
			PyErr_Clear();
			return;
		}
	}

	Py_XDECREF(arg);
}

// run a pycommand
static int run_py_command ( int argc, char *argv[] )
{
	PyObject *arglist = NULL;
	PyObject *arg = NULL;
	PyObject *result = NULL;
	PyObject *tmp = NULL;
	int i;
	int test;

	// build the arg list
	arglist = PyList_New(0);
	assert(arglist != NULL);

	for(i = 0; i < argc; i++) {
		tmp = PyString_FromString(argv[i]);
		test = PyList_Append(arglist, tmp);
		assert(!test);
	}

	arg = Py_BuildValue("(O)", arglist);
	Py_XDECREF(arglist);
	arglist = arg;

	assert(arglist != NULL);

	// find the python code to run
	py_command_t *com = hash_find_nc(py_commands, argv[0], py_command_t *);
	if(com == NULL) {
		assert(!"could not find python command!!!");
		return -1;
	}

	// run the code and check the result
	result = PyEval_CallObject(com->func, arglist);
	if(result == NULL) {
		assert(!"failed to run python callback");
		PyErr_Print();
		PyErr_Clear();
		return -1;
	}

	if(!PyInt_Check(result)) {
		assert(!"python callback ran, but returned non-integer");
		Py_XDECREF(result);
		return -1;
	}

	int rv = PyInt_AsLong(result);
	Py_XDECREF(result);
	return rv;
}

// run a py_server_string
static int run_py_server_string ( int argc, char *argv[] )
{
	PyObject *arglist = NULL;
	PyObject *arg = NULL;
	PyObject *result = NULL;
	PyObject *tmp = NULL;
	int i;
	int test;

	// build the arg list
	arglist = PyList_New(0);
	assert(arglist != NULL);

	for(i = 0; i < argc; i++) {
		tmp = PyString_FromString(argv[i]);
		test = PyList_Append(arglist, tmp);
		assert(!test);
	}

	arg = Py_BuildValue("(O)", arglist);
	Py_XDECREF(arglist);
	arglist = arg;

	assert(arglist != NULL);

	// find the python code to run
	py_server_string_t *com = hash_find_nc(py_server_strings, argv[1], py_server_string_t *);
	if(com == NULL) {
		assert(!"could not find python server_string!!!");
		return -1;
	}

	// run the code and check the result
	result = PyEval_CallObject(com->func, arglist);
	if(result == NULL) {
		assert(!"failed to run python callback");
		PyErr_Print();
		PyErr_Clear();
		return -1;
	}

	if(!PyInt_Check(result)) {
		assert(!"python callback ran, but returned non-integer");
		Py_XDECREF(result);
		return -1;
	}

	int rv = PyInt_AsLong(result);
	Py_XDECREF(result);
	return rv;
}


/*
 * the python module functions
 */

static PyObject *py_GetSettings ( PyObject *self, PyObject *args )
{
	PyObject *settings = PyList_New(0);
	assert(settings != NULL);

	option_t *i;
	for(i = hash_first(options, option_t *); i != NULL; i = hash_next(options, i, option_t *)) 
	{
		PyObject *opt = PyString_FromString(key(i));
		PyList_Append(settings, opt);
	}

	return settings;
}

static PyObject *py_RegisterLoop ( PyObject *self, PyObject *args ) {
	cio_out("RegisterLoop called\n");
	loop_t *loop = malloc(sizeof *loop);

	if(!PyArg_ParseTuple(args, "O", &(loop->func))) {
		assert(!"could not add loop argument");
		return NULL;
	}

	Py_XINCREF(loop->func);
	cio_out("added python loop hook\n");
	push(&py_loop_list, loop);
	if(bool_option("verbose"))
		cio_out("py_loop_list == { %p, %p } [%p]\n", py_loop_list.next, py_loop_list.prev, &py_loop_list);

	RETURN_NOTHING;
}

static PyObject *py_RegisterTab ( PyObject *self, PyObject *args ) {
	cio_out("RegisterTab called\n");
	tab_t *tab = malloc(sizeof *tab);

	if(!PyArg_ParseTuple(args, "O", &(tab->func))) {
		assert(!"could not add tab argument");
		return NULL;
	}

	Py_XINCREF(tab->func);
	cio_out("added python tab hook\n");
	push(&py_tab_list, tab);
	if(bool_option("verbose"))
		cio_out("py_tab_list == { %p, %p } [%p]\n", py_tab_list.next, py_tab_list.prev, &py_tab_list);

	RETURN_NOTHING;
}

static PyObject *py_IncomingMsg ( PyObject *self, PyObject *args ) {
	char *user_col, *msg;

	if(!PyArg_ParseTuple(args, "ss", &user_col, &msg)) {
		assert(!"could not parse args for py_IncomingMessage");
		return NULL;
	}

	cio_out("%s ", timestamp());
	io_colored_out(USER_RED, "%s: ", user_col);
	io_colored_out(USER_WHITE, "%s\n", msg);

	RETURN_NOTHING;
}

static PyObject *py_HandleAway ( PyObject *self, PyObject *args ) {
	char *user, *chan, *msg;

	if(!PyArg_ParseTuple(args, "sss", &user, &chan, &msg)) {
		assert(!"could not parse args for py_HandleAway");
		return NULL;
	}

	away_add_msg(user, chan, msg);

	RETURN_NOTHING;
}

/*
 * registers a new command
 */
static PyObject *py_RegisterCommand ( PyObject *self, PyObject *args ) {
	py_command_t *com;
	int minargc;
	int maxargc;
	char *name;
	char *usage;
	char *desc;

	com = malloc(sizeof(py_command_t));

	if(!PyArg_ParseTuple(args, "sssiiO", &name, &usage, &desc, &minargc, &maxargc, &(com->func))) {
		assert(!"could not parse args for RegisterCommand");
		return NULL;
	}
	if(!PyCallable_Check(com->func)) {
		PyErr_SetString(PyExc_TypeError, "parameter must be callable");
		return NULL;
	}

	Py_XINCREF(com->func);

	py_command_t *tmp = hash_find_nc(py_commands, name, py_command_t *);
	if(tmp != NULL) {
		hash_remove(tmp);
		Py_XDECREF(tmp->func);
		free(tmp);
		hash_insert(py_commands, com, name);
		RETURN_NOTHING;
	}

	command_t *clist = malloc(sizeof(command_t) * 2);
	memset(&clist[1], 0L, sizeof(command_t));
	clist[0].cmd = copy_string(name);
	clist[0].usage = copy_string(usage);
	clist[0].min_argc = minargc;
	clist[0].max_argc = maxargc;
	clist[0].hook = run_py_command;
	clist[0].description = copy_string(desc);
	register_command_list(clist);
	com->clist = clist;

	hash_insert(py_commands, com, name);
	RETURN_NOTHING;
}

static PyObject *py_RegisterServerString ( PyObject *self, PyObject *args )
{
	py_server_string_t *sst = malloc(sizeof(py_server_string_t));
	char *keyword = NULL;
	int strip_colon;
	int argc;
	PyObject *func = NULL;

	if(!PyArg_ParseTuple(args, "siiO", &keyword, &strip_colon, &argc, &func))
		return NULL;

	if(!PyCallable_Check(func)) {
		PyErr_SetString(PyExc_TypeError, "parameter must be callable");
		return NULL;
	}

	Py_XINCREF(func);
	sst->func = func;
	server_string_t *slist = malloc(sizeof(server_string_t) * 2);
	memset(&slist[1], 0L, sizeof(server_string_t));
	slist[0].keystring = copy_string(keyword);
	slist[0].strip_colon = strip_colon;
	slist[0].argc = argc;
	slist[0].hook = run_py_server_string;
	sst->slist = slist;

	py_server_string_t *tmp = hash_find_nc(py_server_strings, keyword, py_server_string_t *);
	if(tmp != NULL) {
		hash_remove(tmp);
		Py_XDECREF(tmp->func);
		unregister_server_string_list(tmp->slist);
	}

	hash_insert(py_server_strings, sst, keyword);
	register_server_string_list(slist);

	RETURN_NOTHING;
}


static PyObject *py_UserMsg ( PyObject *self, PyObject *args ) {
	if(bool_option("verbose"))
		cio_out("py_UserMsg()\n");

	const char *msg;

	if(!PyArg_ParseTuple(args, "s", &msg))
		return NULL;
	io_colored_out(USER_WHITE, "%s\n", msg);

	RETURN_NOTHING;
}

static PyObject *py_IrcMain ( PyObject *self, PyObject *args ) {
	while(!irc_shutdown) {
		run_loop_hooks();
	}

	RETURN_NOTHING;
}

static PyObject *py_IrcShutdown ( PyObject *self, PyObject *args ) {
	irc_shutdown = 1;
	RETURN_NOTHING;
}

static PyObject *py_DebugMsg ( PyObject *self, PyObject *args ) {
	if(bool_option("verbose"))
		cio_out("py_DebugMsg()\n");

	const char *msg;

	if(!PyArg_ParseTuple(args, "s", &msg))
		return NULL;
	cio_out("%s\n", msg);

	RETURN_NOTHING;
}

static PyObject *py_ColoredMsg ( PyObject *self, PyObject *args ) {
	if(bool_option("verbose"))
		cio_out("py_ColoredMsg()\n");
	
	const char *msg;
	const int color;
	
	if(!PyArg_ParseTuple(args, "si", &msg, &color))
		return NULL;

	switch(color) {
	case USER_WHITE:
		io_colored_out(USER_WHITE, "%s", msg);
		break;
	case USER_RED:
		io_colored_out(USER_RED, "%s", msg);
		break;
	case USER_GREEN:
		io_colored_out(USER_GREEN, "%s", msg);
		break;
	case USER_BLUE:
		io_colored_out(USER_BLUE, "%s", msg);
		break;
	default:
		io_colored_out(USER_WHITE, "%s", msg);
		break;
	}

	RETURN_NOTHING;
}

static PyObject *py_IrcAction ( PyObject *self, PyObject *args ) {
	if(bool_option("verbose"))
		cio_out("py_IrcAction()\n");

	const char *msg;

	if(!PyArg_ParseTuple(args, "s", &msg))
		return NULL;
	irc_action(msg);

	RETURN_NOTHING;
}

static PyObject *py_IrcSay ( PyObject *self, PyObject *args ) {
	if(bool_option("verbose"))
		cio_out("py_IrcSay()\n");

	const char *msg;
	const char *dest;
	user_t *u;

	dest = natural_channel_name();
	if(!dest) {
		RETURN_NOTHING;
	}

	if(!PyArg_ParseTuple(args, "s", &msg))
		return NULL;

	u = find_user(irc_server_nick(irc_pick_server()), irc_pick_server());
	assert(u != NULL);

	u->privmsg_print(u, dest, msg);
	irc_out("PRIVMSG %s :%s\n", dest, msg);
	RETURN_NOTHING;
}

static PyObject *py_CurrentNick ( PyObject *self, PyObject *args )
{
	return PyString_FromString(irc_server_nick(irc_pick_server()));
}

static PyObject *py_CurrentIp ( PyObject *self, PyObject *args )
{
	return PyString_FromString(current_ip());
}

static PyObject *py_Ask ( PyObject *self, PyObject *args )
{
	const char *question;
	if(!PyArg_ParseTuple(args, "s", &question))
		return NULL;
	return PyString_FromString(ask(question));
}

static PyObject *py_GetCurrentInfo ( PyObject *self, PyObject *args )
{
	return Py_BuildValue("(ssss)",
			irc_server_nick(irc_pick_server()),
			natural_channel_name(),
			irc_pick_server(),
			current_ip());
}

static PyObject *py_IrcMsg ( PyObject *self, PyObject *args ) {
	if(bool_option("verbose"))
		cio_out("py_IrcMsg()\n");

	const char *msg;
	const char *dest;
	user_t *u;

	if(!PyArg_ParseTuple(args, "ss", &dest, &msg))
		return NULL;

	u = find_user(irc_server_nick(irc_pick_server()), irc_pick_server());
	assert(u != NULL);

	u->privmsg_print(u, dest, msg);
	irc_out("PRIVMSG %s :%s\n", dest, msg);
	RETURN_NOTHING;
}

static PyObject *py_IrcRaw ( PyObject *self, PyObject *args ) {
	if(bool_option("verbose"))
		cio_out("py_IrcRaw()\n");

	const char *msg;

	if(!PyArg_ParseTuple(args, "s", &msg))
		return NULL;

	irc_out("%s", msg);

	RETURN_NOTHING;
}

static PyObject *py_Load ( PyObject *self, PyObject *args ) {
	if(bool_option("verbose"))
		cio_out("py_Load()\n");

	const char *plugin;

	if(!PyArg_ParseTuple(args, "s", &plugin))
		return NULL;

	load_plugin(plugin);
	RETURN_NOTHING;
}

static PyObject *py_Unload ( PyObject *self, PyObject *args ) {
	if(bool_option("verbose"))
		cio_out("py_Unload()\n");

	const char *plugin;

	if(!PyArg_ParseTuple(args, "s", &plugin))
		return NULL;

	unload_plugin(plugin);
	RETURN_NOTHING;
}

static PyObject *py_Connect ( PyObject *self, PyObject *args )
{
	int port;
	const char *server;

	if(bool_option("verbose"))
		cio_out("py_Connect()\n");

	if(!PyArg_ParseTuple(args, "si", &server, &port))
		return NULL;

	irc_connect(port, server, 0);

	RETURN_NOTHING;
}

static PyObject *py_SSLConnect ( PyObject *self, PyObject *args )
{
	int port;
	const char *server;

	if(bool_option("verbose"))
		cio_out("py_Connect()\n");

	if(!PyArg_ParseTuple(args, "si", &server, &port))
		return NULL;

	irc_connect(port, server, 1);

	RETURN_NOTHING;
}

static PyObject *py_Nick ( PyObject *self, PyObject *args )
{
	const char *nick;

	if(!PyArg_ParseTuple(args, "s", &nick))
		return NULL;

	irc_nick(nick);
	RETURN_NOTHING;
}

static PyObject *py_Join ( PyObject *self, PyObject *args )
{
	char *channel;
	channel_t *tmp;

	if(!PyArg_ParseTuple(args, "s", &channel))
		return NULL;

	tmp = find_channel(channel, irc_pick_server());

	if(tmp == NULL)
		tmp = new_channel(channel, irc_pick_server());

	tmp->set_window(tmp, active_window <= window_max && active_window > 0 ? active_window : 0 );
	tmp->join(tmp);
	RETURN_NOTHING;
}

static PyObject *py_SetOption ( PyObject *self, PyObject *args )
{
	const char *option;
	const char *value;

	if(!PyArg_ParseTuple(args, "ss", &option, &value))
		return NULL;

	set_option(option, value);
	RETURN_NOTHING;
}

static PyObject *py_BoolOption(PyObject *self, PyObject *args)
{
	const char *option;

	if(!PyArg_ParseTuple(args, "s", &option))
		return NULL;

	return Py_BuildValue("i", bool_option(option));
}

static PyObject *py_StringOption(PyObject *self, PyObject *args)
{
	const char *option;
	if(!PyArg_ParseTuple(args, "s", &option))
		return NULL;

	return Py_BuildValue("s", safe_string_option(option));
}

static PyObject *py_IntOption(PyObject *self, PyObject *args)
{
	const char *option;
	if(!PyArg_ParseTuple(args, "s", &option))
		return NULL;

	return Py_BuildValue("i", int_option(option));
}

static PyObject *py_ShowPlugins ( PyObject *self, PyObject *args )
{
	plugin_list();
	RETURN_NOTHING;
}

static PyObject *py_ShowOptions ( PyObject *self, PyObject *args )
{
	char *test = copy_string("/set\n");
	parse_input_string(test);

	RETURN_NOTHING;
}

static PyObject *py_Part ( PyObject *self, PyObject *args )
{
	char *channel;
	channel_t *tmp;

	if(!PyArg_ParseTuple(args, "s", &channel))
		return NULL;

	tmp = find_channel(channel, irc_pick_server());
	if(!tmp)
		RETURN_NOTHING;
	tmp->part(tmp, NULL);
	RETURN_NOTHING;
}

static PyObject *py_ParseInput ( PyObject *self, PyObject *args )
{
	char *input;

	if(!PyArg_ParseTuple(args, "s", &input))
		return NULL;

	parse_input_string(input);
	RETURN_NOTHING;
}

static PyObject *py_GrabNick ( PyObject *self, PyObject *args )
{
	char *host;

	if(!PyArg_ParseTuple(args, "s", &host)) {
		assert("GrabNick needs a string!");
		return NULL;
	}

	return Py_BuildValue("s", grab_nick(host));
}

static PyObject *py_IsAway ( PyObject *self, PyObject *args )
{
	return Py_BuildValue("i", is_away);
}


static PyObject * py_Urlize ( PyObject *self, PyObject *args )
{
	char *orig;

	if(!PyArg_ParseTuple(args, "s", &orig))
		return NULL;

	return Py_BuildValue("s", urlize(orig));
}

typedef struct {
	list_t header;
	PyObject *func;
} iparser_t;

static list_t iparsers;

static PyObject *py_PushInputParser ( PyObject *self, PyObject *args )
{
	iparser_t *parser;
	PyObject *tmp;

	if(!PyArg_ParseTuple(args, "O", &tmp))
		return NULL;

	parser = malloc(sizeof *parser);
	parser->func = tmp;
	push(&iparsers, parser);
	Py_XINCREF(tmp);

	RETURN_NOTHING;
}

static PyObject *py_PopInputParser ( PyObject *self, PyObject *args )
{
	pop(&iparsers);
	RETURN_NOTHING;
}

/*
 * python method table
 */
static PyMethodDef bnircMethods[] = {
	{"IsAway", py_IsAway, METH_VARARGS,
		"IsAway(): returns 1 if away, 0 if not"},
	{"IncomingMsg", py_IncomingMsg, METH_VARARGS,
		"IncomingMsg(user_column, message): Prints a private message formatted message."},
	{"HandleAway", py_HandleAway, METH_VARARGS,
		"HandleAway(user, chan, message): if needed, will store away message." },
	{"UserMsg", py_UserMsg, METH_VARARGS,
		"UserMsg(): Prints a message to the bnIRC screen."},
	{"DebugMsg", py_DebugMsg, METH_VARARGS,
		"DebugMsg(): Prints a debugging message."},
	{"ColoredMsg", py_ColoredMsg, METH_VARARGS,
		"ColoredMsg(): Prints a message in specified color."},
	{"IrcMain", py_IrcMain, METH_VARARGS,
		"IrcMain(): runs irc main loop."},
	{"IrcShutdown", py_IrcShutdown, METH_VARARGS,
		"IrcShutdown(): Kills IrcMain()."},
	{"IrcSay", py_IrcSay, METH_VARARGS,
		"IrcSay(msg): Says something on IRC."},
	{"IrcMsg", py_IrcMsg, METH_VARARGS,
		"IrcMsg(dest, msg): Says something on IRC to someone or a specific channel."},
	{"RegisterLoop", py_RegisterLoop, METH_VARARGS,
		"RegisterLoop(loop_func)" },
	{"RegisterPoll", py_RegisterPoll, METH_VARARGS,
		"RegisterPoll(fd, func) fd being the file descriptor (must be an int)" },
	{"UnregisterPoll", py_UnregisterPoll, METH_VARARGS,
		"UnregisterPoll(fd) remove filedescriptor from being polled" },
	{"TimedFunction", py_TimedFunction, METH_VARARGS,
		"TimedFunction(msecs, function) Run function in msecs milliseconds" },
	{"RegisterTab", py_RegisterTab, METH_VARARGS,
		"RegisterTab(tab_func)" },
	{"RegisterCommand", py_RegisterCommand, METH_VARARGS,
		"RegisterCommand(name, usage, description, min_argcount, max_argcount, func)"},
	{"RegisterServerString", py_RegisterServerString, METH_VARARGS,
		"RegisterServerString(key, strip_colon, argc, func): Hooks into stuff from server."},
	{"CurrentNick", py_CurrentNick, METH_VARARGS,
		"string CurrentNick(): Returns your current nick." },
	{"CurrentIp", py_CurrentIp, METH_VARARGS,
		"CurrentIp();" },
	{"IrcAction", py_IrcAction, METH_VARARGS,
		"IrcAction(does_something): *<your_nick> does_something." },
	{"IrcRaw", py_IrcRaw, METH_VARARGS,
		"IrcRaw(msg): Sends a raw message to the IRC server." },
	{"IrcConnect", py_Connect, METH_VARARGS,
		"IrcConnect(server, port): Connects to an IRC server." },
	{"IrcSSLConnect", py_SSLConnect, METH_VARARGS,
		"IrcSSLConnect(server, port): Connects to an IRC server via SSL." },
	{"IrcNick", py_Nick, METH_VARARGS,
		"IrcNick(nickname): Sets your nickname" },
	{"IrcJoin", py_Join, METH_VARARGS,
		"IrcJoin(channel): Joins a channel." },
	{"IrcPart", py_Part, METH_VARARGS,
		"IrcPart(channel): Leaves a channel." },
	{"LoadPlugin", py_Load, METH_VARARGS,
		"LoadPlugin(name): Loads a bnirc plugin." },
	{"UnloadPlugin", py_Unload, METH_VARARGS,
		"UnloadPlugin(name): Unloads a bnirc plugin." },
	{"SetOption", py_SetOption, METH_VARARGS,
		"SetOption(option, value): Sets a bnirc option value." },
	{"BoolOption", py_BoolOption, METH_VARARGS,
		"int BoolOption(option): Returns 0 for false, 1 for true." },
	{"StringOption", py_StringOption, METH_VARARGS,
		"string StringOption(option)" },
	{"IntOption", py_IntOption, METH_VARARGS,
		"int IntOption(option)" },
	{"ShowPlugins", py_ShowPlugins, METH_VARARGS,
		"ShowPlugins(): Shows loaded bnirc plugins." },
	{"ShowOptions", py_ShowOptions, METH_VARARGS,
		"ShowOptions(): Show bnirc options." },
	{"ParseInput", py_ParseInput, METH_VARARGS,
		"ParseInput(inputstring): Parses input string and executes correct action." },
	{"GrabNick", py_GrabNick, METH_VARARGS,
		"GrabNick(hoststring): Host string is usually args[0] for a serverstring callback." },
	{"GetCurrentInfo", py_GetCurrentInfo, METH_VARARGS,
		"GetCurrentInfo(): returns your nick, channel, and server in a tuple (in that order)." },
	{"Urlize", py_Urlize, METH_VARARGS,
		"Urlize(string): Makes a URL usable string." },
	{"Ask", py_Ask, METH_VARARGS,
		"Ask(string): Ask the user for a question and prompt for the answer" },
	{"GetSettings", py_GetSettings, METH_VARARGS,
		"GetSettings(): returns a list of option names that can be used with StringOption and friends"},
	{NULL, NULL, 0, NULL}
};

static plugin_t plugin;
static int no_initialize = 0;

/* this is so the module can be used without being IN bnirc */
PyMODINIT_FUNC initbnirc ( void )
{
	char *argv[] = { "builtin" };
	if(!bool_option("verbose")) {
		cio_out("setting up the /set command\n");
	}
	no_initialize = 1;
	register_plugin(&plugin);
	init_plugin(&plugin, "builtin", NULL, 1, argv);
}

static int python_hook ( int, char *[] );

static int	hello	( int argc, char *argv[] )
{
	cio_out("initializing python!\n");

	PyObject *m;

	hash_init(py_commands);
	hash_init(py_server_strings);
	list_init(&py_loop_list);
	list_init(&py_tab_list);
	list_init(&poll_head);

	char *_argv[] = { "python", "startup.py", NULL };

	if(!no_initialize) {
		Py_SetProgramName("python");
		Py_Initialize();
		m = Py_InitModule("bnirc", bnircMethods);
		python_hook(2, _argv);
	} else {
		m = Py_InitModule("bnirc", bnircMethods);
	}
	if (m == NULL) io_colored_out(USER_RED, 
		"could not init bnirc module\n");

	add_loop_hook(py_loops);
	add_tab_completes(py_tabs);

	PyModule_AddIntConstant(m, "USER_WHITE", USER_WHITE);
	PyModule_AddIntConstant(m, "USER_RED", USER_RED);
	PyModule_AddIntConstant(m, "USER_GREEN", USER_GREEN);
	PyModule_AddIntConstant(m, "USER_BLUE", USER_BLUE);

	return 0;
}


static int goodbye	( void )
{
	cio_out("removing python commands\n");
	py_command_t *i;
	py_server_string_t *j;

	remove_loop_hook(py_loops);
	remove_tab_completes(py_tabs);

	for(i = hash_first(py_commands, py_command_t *); i != NULL;
			i = hash_next(py_commands, i, py_command_t *)) {
		unregister_command_list(i->clist);
	}

	for(j = hash_first(py_server_strings, py_server_string_t *); j != NULL;
			j = hash_next(py_server_strings, j, py_server_string_t *)) {
		unregister_server_string_list(j->slist);
	}

	cio_out("goodbye python!\n");
	Py_Finalize();
	return 0;
}

static int python_hook	( int argc, char *argv[] )
{
	char filename[200] = "scripts/";
	char *buf;
	FILE *fp;

	strncat(filename, argv[1], 199);
	filename[199] = 0;
	buf = bnirc_find_file(filename);
	if(!buf) {
		cio_out("could not find file [%s]\n", filename);
		return -1;
	}

	fp = fopen(buf, "r");

	if(bool_option("verbose"))
		cio_out("running %s\n", buf);

	if(PyRun_SimpleFile(fp, buf))
		io_colored_out(USER_RED, "script error!\n");

	fclose(fp);
	free(buf);

	return 0;
}

/***************************************************************
 * for python REPL mode
 ***************************************************************/
static int python_parser ( char *line )
{
	static char *buf = NULL;
	const char *outbuf;
	int  *outbuflen = NULL;
	size_t len = strlen(line);
	PyObject *m, *ret;
	int i;
	int is_blank = 1;
	static int dont_die = 0;

	// buf = realloc(buf, buf == NULL ? len + 1 : strlen(buf) + len + 1);
	if(buf)
		buf = realloc(buf, strlen(buf) + len + 1);
	else
		buf = malloc(len + 1);

	strcat(buf, line);

	for(i = 0; i < len; i++)
		if(!isspace(line[i]))
			is_blank = 0;

	if(is_blank && !dont_die) {
		cio_out("Leaving Python REPL\n");
		pop_input_parser();
		return 0;
	}

	cio_out(">>> %s", line);

	for(i = 0; !is_blank && i < len && line[i] == ' '; i++)
		send_char_to_input(' ');

	if(line[len-2] == ':') {
		send_char_to_input(' ');
		i++;
		dont_die = 1;
	}

	if(!i || is_blank) {
		m = PyImport_AddModule("__main__");
		if(!m)
			return -1;
		m = PyModule_GetDict(m);
		Py_INCREF(m);
		ret = PyRun_String(buf, Py_single_input, m, m);
		if(!ret) {
			PyErr_Print();
		} else {
			if(!PyObject_AsCharBuffer(ret, &outbuf, outbuflen)) {
				cio_out("%s\n", outbuf);
			}
			Py_XDECREF(ret);
		}
		Py_XDECREF(m);
		free(buf);
		buf = NULL;
		PyErr_Clear();
		dont_die = 0;
	}

	return 0;
}

static int python_repl ( int argc, char *argv[] )
{
	cio_out("enter a blank line to end python REPL mode\n");
	push_input_parser(python_parser);
	return 0;
}

static command_t commands[] = {
	{ "python", "python <filename>", 2, 2, python_hook, "runs a python script" },
	{ "py:repl", "py:repl", 1, 1, python_repl, "start Read Evaluate Print Loop" },
	{ NULL, NULL, 0, 0, NULL, NULL }
};

static plugin_t	plugin = {
	.name		= "python",
	.description	= "python scripting for bnirc",
	.version	= "0.0.2",
	.plugin_init	= hello,
	.plugin_cleanup = goodbye,
	.command_list = commands
};

REGISTER_PLUGIN(plugin, bnirc_python);
