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

#if defined(HAVE_ncursesw)
#include <ncursesw/ncurses.h>
#elif defined(HAVE_ncurses)
#include <ncurses.h>
#elif defined(HAVE_curses)
#include <curses.h>
#else
#error NEED CURSES!
#endif

#ifdef HAVE_USE_DEFAULT_COLORS
#define COLOR_DEFAULT -1
#else
#define COLOR_DEFAULT COLOR_BLACK
#endif

static io_driver_t io_driver;
static int io_block_stack = 0;
static int io_ready = 0;

#define size_x io_driver.sizex
#define size_y io_driver.sizey

/* as best as i can tell, ncurses cannot handle more then 10 pads */
#define MAX_WINDOWS 10
#define main_window (windows[active_window])
#define page_up (page_ups[active_window])

#define RED 1
#define BLUE 2
#define GREEN 3
#define WHITE 4
#define CYAN 5
#define LINE_COLOR 6
#define LINE_EMPH_COLOR 7

/* silly color schemes */
#define COLOR_COMMIE 0xffff0001


#define PAD_LINES 400
#define LINES_SHOWING (size_y - 3)

#define PAD_NATURAL_TOP (PAD_LINES - (LINES_SHOWING + 1))

#define pad_top	(page_up <= 0 ? PAD_NATURAL_TOP : PAD_NATURAL_TOP - (page_up * (LINES_SHOWING - 10)))

#define max_page_up (PAD_NATURAL_TOP / (LINES_SHOWING - 10))

#define REFRESH() do { \
	if(io_block_stack) { io_ready = 1; break; } \
	pnoutrefresh(main_window, pad_top, 0, 0, 0, LINES_SHOWING, size_x); \
	refresh_info_line(); \
	doupdate(); \
} while(0)

static WINDOW *windows[MAX_WINDOWS] = { NULL };
static WINDOW *input_window;
static WINDOW *line_window;
static int page_ups[MAX_WINDOWS] = { 0 };
static size_t cursor_pos = 0;
static int clear_input = 0;
static unsigned my_window_flags[MAX_WINDOWS];
static int ncurses_shutdown = 0;
static void refresh_info_line(void);



static int wputc_hook(window_t w, int c)
{
	waddch(windows[w], c);
	if(w == active_window && c == '\n') REFRESH();
	return 0;
}

static int putc_hook(int c)
{
	/*
	waddch(windows[active_window], c);
	REFRESH();
	return 0;
	*/
	wputc_hook(active_window, c);
	return 0;
}

static int wputstring_hook(window_t w, const char *s);

static int putstring_hook(const char *s)
{
	return wputstring_hook(active_window, s);
}

static int wputstring_hook(window_t w, const char *s)
{
	static char nls[MAX_WINDOWS] = { 0 };
	assert(w < MAX_WINDOWS);

	if (nls[w] != 0) {
		waddch(windows[w], '\n');
		nls[w] = 0;
	}

	char *str = copy_string(s);
	if (str[strlen(str) - 1] == '\n') {
		str[strlen(str) - 1] = 0;
		nls[w] = 1;
	}

	waddstr(windows[w], str);
	free(str);
	if(w == active_window && strchr(s, '\n')) REFRESH();
	return 0;
}

#define ALT_KEYCODE (27)

static void new_sigwinch_handler_(int sig);
static int getc_hook(void)
{
	int c;

	nodelay(input_window, TRUE);
    noecho();

	c = wgetch(input_window);

    /*
    if(c != ALT_KEYCODE) 
        waddch(input_window, c);
    */

	if(c == ERR)
		return NO_CHAR_AVAIL;

    if(bool_option("debug_keys"))
        cio_out("recieved %d\n", c);

	switch (c) {
        case ALT_KEYCODE: // ALT KEY (ESC)
            c = wgetch(input_window);
            if(c <= '0' || c > '9')
                return c;
            else
                return F0+(c - '0');

        case KEY_RESIZE:
            new_sigwinch_handler_(SIGWINCH);
            return getc_hook();
        case KEY_F(1):
        case F1:
            return F1;
        case KEY_F(2):
        case F2:
            return F2;
        case KEY_F(3):
        case F3:
            return F3;
        case KEY_F(4):
        case F4:
            return F4;
        case KEY_F(5):
        case F5:
            return F5;
        case KEY_F(6):
        case F6:
            return F6;
        case KEY_F(7):
        case F7:
            return F7;
        case KEY_F(8):
        case F8:
            return F8;
        case KEY_F(9):
        case F9:
            return F9;
        case KEY_F(10):
        case F10:
            return F10;
        case KEY_F(11):
        case F11:
            return F11;
        case KEY_F(12):
        case F12:
            return F12;
        case 0x7:
        case KEY_DC:
        case DELETE:
            return DELETE;
        case KEY_BACKSPACE:
        case 127:
        case BACKSPACE:
            return BACKSPACE;
        case KEY_UP:
        case UP:
            return UP;
        case KEY_DOWN:
        case DOWN:
            return DOWN;
        case KEY_LEFT:
        case LEFT:
            return LEFT;
        case KEY_RIGHT:
        case RIGHT:
            return RIGHT;
        case KEY_NPAGE:
            if (page_up > 0)
                page_up--;
            REFRESH();
            return getc_hook();
        case KEY_PPAGE:
            if (page_up < max_page_up)
                page_up++;
            REFRESH();
            return getc_hook();
        case KEY_HOME:
            return HOME;
        case KEY_END:
            return END;
        case '\n':
            werase(input_window);
            REFRESH();
            return '\n';
        default:
            waddch(input_window, c);
            return c;
    }
}

static int ungetc_hook(int c)
{
	ungetch(c);
	REFRESH();
	return 0;
}

static int set_active_window_hook(window_t w)
{
	if (bool_option("verbose"))
		cio_out("old active window = %d, new = %d\n", active_window, w);

	active_window = w;
	my_window_flags[active_window] &=
	    ~(WIN_HAS_OUTPUT | WIN_HAS_DIR_OUTPUT);
	REFRESH();
	return 0;
}

typedef struct {
	int color;
	const char *text;
} color_option_t;

static color_option_t colors[] = {
	{COLOR_RED, "red"},
	{COLOR_BLUE, "blue"},
	{COLOR_DEFAULT, "default"},
	{COLOR_GREEN, "green"},
	{COLOR_BLACK, "black"},
	{COLOR_WHITE, "white"},
	{COLOR_COMMIE, "commie"},
	{0, NULL}
};

int sinfoc_command(int argc, char *argv[])
{
	assert(argc == 2);
	int i;

	for (i = 0; colors[i].text != NULL; i++) {
		if (strcasecmp(argv[1], colors[i].text) == 0) {
			switch (colors[i].color) {
			case COLOR_DEFAULT:
				init_pair(LINE_COLOR, COLOR_DEFAULT,
					  colors[i].color);
				init_pair(LINE_EMPH_COLOR, COLOR_RED,
					  colors[i].color);
				break;
			case COLOR_GREEN:
				init_pair(LINE_COLOR, COLOR_BLACK,
					  colors[i].color);
				init_pair(LINE_EMPH_COLOR, COLOR_RED,
					  colors[i].color);
				break;
			case COLOR_BLUE:
				init_pair(LINE_COLOR, COLOR_WHITE,
					  colors[i].color);
				init_pair(LINE_EMPH_COLOR, COLOR_RED,
					  colors[i].color);
				break;
			case COLOR_RED:
				init_pair(LINE_COLOR, COLOR_DEFAULT, COLOR_RED);
				init_pair(LINE_EMPH_COLOR, COLOR_BLUE,
					  COLOR_RED);
				break;
			case COLOR_BLACK:
				init_pair(LINE_COLOR, COLOR_WHITE, COLOR_BLACK);
				init_pair(LINE_EMPH_COLOR, COLOR_RED,
					  COLOR_BLACK);
				break;
			case COLOR_WHITE:
				init_pair(LINE_COLOR, COLOR_BLACK, COLOR_WHITE);
				init_pair(LINE_EMPH_COLOR, COLOR_RED,
					  COLOR_WHITE);
				break;
			case COLOR_COMMIE:
				init_pair(LINE_COLOR, COLOR_YELLOW, COLOR_RED);
				init_pair(LINE_EMPH_COLOR, COLOR_BLUE,
					  COLOR_RED);
				break;
			default:
				assert(!"code should never get here");
			}

			return 0;
		}
	}

	cio_out("please use one of the following colors:\n");
	for (i = 0; colors[i].text != NULL; i++) {
		cio_out("%s\n", colors[i].text);
	}

	return -1;
}

static command_t commands[] = {
	{"sinfoc", "sinfoc [color]", 2, 2, sinfoc_command,
	 "set color of info line"},
	{NULL, NULL, 0, 0, NULL}
};

static void refresh_info_line(void)
{
	if (bool_option("hide_info_bar")) {
		werase(line_window);
		wrefresh(line_window);
		wrefresh(input_window);
		return;
	}

	const char *channel = natural_channel_name();

	const char tag[] = PACKAGE "-" VERSION;
	int i, j, buf;
	char *time = timestamp();

	if (size_x == 0)
		return;

	werase(line_window);

	for (i = 0; i < 3; i++) {
		waddch(line_window, ' ');
	}

	waddstr(line_window, time);
	waddstr(line_window, "     ");
	i += strlen(time) + 5;

	free(time);

	waddstr(line_window, tag);
	i += strlen(tag);

	waddch(line_window, ' ');
	i++;

	if (channel) {
		waddch(line_window, '[');
		waddstr(line_window, channel);
		const char *query = current_query();
		if (query != NULL) {
			waddstr(line_window, " query=");
			waddstr(line_window, query);
			i += strlen(query) + 7;
		}
		waddch(line_window, ']');
		i += strlen(channel) + 2;
	}

	waddstr(line_window, " [");
	i += 3;			// also includes ] that i put on later
	buf = 0;
	char num[8];
	for (j = 0; j < window_max; j++) {
		if (my_window_flags[j] & WIN_HAS_DIR_OUTPUT) {
			wattroff(line_window, COLOR_PAIR(LINE_COLOR));
			wattron(line_window, COLOR_PAIR(LINE_EMPH_COLOR));
			if (buf == 0)
				waddch(line_window, ' ');
			sprintf(num, "%d ", j + 1);
			waddstr(line_window, num);
			wattroff(line_window, COLOR_PAIR(LINE_EMPH_COLOR));
			wattron(line_window, COLOR_PAIR(LINE_COLOR));
		} else if (my_window_flags[j] & WIN_HAS_OUTPUT) {
			if (buf == 0)
				waddch(line_window, ' ');
			sprintf(num, "%d ", j + 1);
			waddstr(line_window, num);
			// sprintf(num, "%d", j);
			// waddstr(line_window, num);
		} else {
			continue;
		}

		buf = 1;
		if (j >= 10)
			i += 3;
		else
			i += 2;
	}

	if (buf) {
		i += 1;
	}

	waddch(line_window, ']');

	if (is_away) {
		waddstr(line_window, "  away");
		i += 6;
	}

	if (page_up) {
		const char *msg = " You are paged up.";
		waddstr(line_window, msg);
		i += strlen(msg);
	}

	for (; i < size_x - 1; i++)
		waddch(line_window, ' ');

	wmove(input_window, 0, cursor_pos);

	wnoutrefresh(line_window);
	wnoutrefresh(input_window);
}

static inline void clear_all_windows();

static void _sigwinch_sig_handler(int sig)
{
	int i;

	if (sig)
		endwin();

	refresh();
	getmaxyx(stdscr, size_y, size_x);
	resizeterm(size_y, size_x);

	for (i = 0; i < MAX_WINDOWS; i++) {
		if (sig)
			delwin(windows[i]);
		windows[i] = newpad(PAD_LINES, size_x);
		idlok(windows[i], TRUE);
		scrollok(windows[i], 1);
		wclear(windows[i]);
	}

	delwin(input_window);
	input_window = newwin(1, size_x, size_y - 1, 0);
	scrollok(input_window, 1);
	wclear(input_window);
	clear_input = 1;

	delwin(line_window);
	line_window = newwin(1, size_x, size_y - 2, 0);
	scrollok(line_window, 1);
	wattrset(line_window, A_BOLD | COLOR_PAIR(LINE_COLOR));
	wclear(line_window);

	// noecho();
	keypad(input_window, TRUE);
	cbreak();
	nl();

	clear_all_windows();

	wclear(input_window);
	wclear(line_window);

	REFRESH();

	cio_out("window is %dx%d\n", size_x, size_y);
}

static void new_sigwinch_handler_(int sig)
{
	int i;

	assert(sig == SIGWINCH);
	getmaxyx(stdscr, size_y, size_x);


	/*
	   if(is_term_resized(size_y, size_x) == FALSE) {
	   if(bool_option("verbose"))
	   cio_out("SIGWINCH recieved when no resize needed!\n");
	   return;
	   }
	 */

	if (bool_option("verbose")) {
		cio_out("resizing to %dx%d\n", size_x, size_y);
	}

	resizeterm(size_y, size_x);

	for (i = 0; i < MAX_WINDOWS; i++)
		wresize(windows[i], PAD_LINES, size_x);

	delwin(input_window);
	input_window = newwin(1, size_x, size_y - 1, 0);
	scrollok(input_window, 1);
	wclear(input_window);
	clear_input = 1;

	delwin(line_window);
	line_window = newwin(1, size_x, size_y - 2, 0);
	scrollok(line_window, 1);
	wattrset(line_window, A_BOLD | COLOR_PAIR(LINE_COLOR));
	wclear(line_window);

	// noecho();
	keypad(input_window, TRUE);
	cbreak();
	nl();
	wclear(line_window);
	wclear(input_window);
	REFRESH();
}


static void refresher_func(void *discard)
{
	refresh_info_line();
	doupdate();
	add_timed_func(1000, refresher_func, NULL);
}


static int init_hook(void);


static int clear_window_hook(window_t w)
{
	wclear(windows[w]);
	wattron(windows[w], COLOR_PAIR(CYAN));
	mvwaddstr(windows[w], PAD_NATURAL_TOP, 0, "initializing io\n");
	wattroff(windows[w], COLOR_PAIR(CYAN));
	REFRESH();
	return 0;
}

static inline void clear_all_windows(void)
{
	window_t i;
	for (i = 0; i < MAX_WINDOWS; i++) {
		clear_window_hook(i);
	}
}

static int cleanup_hook(void)
{
    del_timed_func(refresher_func);
	endwin();

	// sigaction(SIGWINCH, (struct sigaction *)SIG_DFL, NULL);
	// remove_loop_hook(refresher_func);

	ncurses_shutdown = 1;
	return 0;
}

static int set_color_hook(window_t w, int c)
{
	switch (c) {
	case USER_CONTROL_COLOR:
		wattroff(windows[w], A_BOLD);
		wattron(windows[w], COLOR_PAIR(CYAN));
		break;
	case USER_RED:
		wattron(windows[w], COLOR_PAIR(RED));
		wattron(windows[w], A_BOLD);
		break;
	case USER_BLUE:
		wattron(windows[w], COLOR_PAIR(BLUE));
		wattron(windows[w], A_BOLD);
		break;
	case USER_WHITE:
		wattron(windows[w], COLOR_PAIR(WHITE));
		wattron(windows[w], A_BOLD);
		break;
	case USER_GREEN:
		wattron(windows[w], COLOR_PAIR(GREEN));
		wattron(windows[w], A_BOLD);
		break;
	default:
		wattroff(windows[w], A_BOLD);
		wattroff(windows[w], COLOR_PAIR(GREEN));
		wattroff(windows[w], COLOR_PAIR(WHITE));
		wattroff(windows[w], COLOR_PAIR(BLUE));
		wattroff(windows[w], COLOR_PAIR(RED));
		wattroff(windows[w], COLOR_PAIR(CYAN));
	}
	return 0;
}

static int set_cur_pos_hook(size_t x)
{
    char buf[1024];
    wmove(input_window, 0, 0);
    int len = winnstr(input_window, buf, sizeof(buf) - 1);

    //while(buf[strlen(buf)-1] & 0xc0)
        //buf[strlen(buf)-1] = 0;

    buf[len] = 0;
    int offset = mbstowcs(NULL, buf, 0) - len;
	cursor_pos = x+offset;
	wmove(input_window, 0, cursor_pos);
	return 0;
}

static int erase_past_cursor_hook(void)
{
	wclrtobot(input_window);
	return 0;
}

static int add_to_input_hook(const char *c)
{
	waddstr(input_window, c);
	return 0;
}

static int clear_input_hook(void)
{
	werase(input_window);
	set_cur_pos_hook(0);
	return 0;
}

static void _begin_io_block ( void )
{
	io_block_stack++;
}

static void _end_io_block ( void )
{
	io_block_stack--;
	if(io_ready) {
		io_ready = 0;
		REFRESH();
	}
}

static io_driver_t io_driver = {
	.putc = putc_hook,
	.putstring = putstring_hook,
	.wputc = wputc_hook,
	.wputstring = wputstring_hook,
	.getc = getc_hook,
	.ungetc = ungetc_hook,
	.init = init_hook,
	.cleanup = cleanup_hook,
	.set_active_window = set_active_window_hook,
	.clear_window = clear_window_hook,
	.set_color = set_color_hook,
	.set_cur_pos = set_cur_pos_hook,
	.erase_past_cursor = erase_past_cursor_hook,
	.add_to_input = add_to_input_hook,
	.clear_input = clear_input_hook,
	.begin_io_block = _begin_io_block,
	.end_io_block = _end_io_block,

	.window_flags = my_window_flags,
	.max_windows = MAX_WINDOWS,

	.sizex = 0,
	.sizey = 0
};

static int init_hook(void)
{
	initscr();
	//dup2(1, 2);
	start_color();
	getmaxyx(stdscr, size_y, size_x);

#ifdef HAVE_USE_DEFAULT_COLORS
	use_default_colors();
#endif

	init_pair(RED, COLOR_RED, COLOR_DEFAULT);
	init_pair(GREEN, COLOR_GREEN, COLOR_DEFAULT);
	init_pair(BLUE, COLOR_BLUE, COLOR_DEFAULT);
	init_pair(WHITE, COLOR_DEFAULT, COLOR_DEFAULT);
	init_pair(CYAN, COLOR_CYAN, COLOR_DEFAULT);
	init_pair(LINE_COLOR, COLOR_WHITE, COLOR_BLUE);
	init_pair(LINE_EMPH_COLOR, COLOR_RED, COLOR_BLUE);


	_sigwinch_sig_handler(0);

#ifdef HAVE_USE_DEFAULT_COLORS
	cio_out
	    ("you are using a good curses implementation... using your default colors\n");
#endif
	/* signal(SIGWINCH, _sigwinch_sig_handler); */
	// signal(SIGWINCH, new_sigwinch_handler_);


	window_max = MAX_WINDOWS;
	add_timed_func(1000, refresher_func, NULL);

	return 0;
}

static plugin_t plugin = {
	.name = "io_ncurses",
	.version = "0.0.1",
	.description = "provides an ncurses interface",
	.io_driver = &io_driver,
	.command_list = commands
};

REGISTER_PLUGIN(plugin, io_ncurses)
