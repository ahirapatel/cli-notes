#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <form.h>

#define TAG_ROWS 1
#define TAG_COLS 10
#define NOTE_ROWS 4
#define NOTE_COLS 20
#define GAP 2

#define FORM_START_Y (GAP)
#define FORM_START_X (strlen(NOTE_STR)+GAP)

#define NOTE_HEADER "New Note"
#define TAG_STR "Tag (Optional): "
#define NOTE_STR "Note (Required): "
#define DONE_STR "Hit Enter When Done"

int main()
{
	FIELD *fields[3];
	FORM *form;
	WINDOW *window;
	int rows, cols;
	int maxrows, maxcols;
	int winrows, wincols;

	initscr();
	start_color();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);	// TODO: Needed?

	init_pair(1, COLOR_WHITE, COLOR_BLUE);

	//new_field(rows,cols,starty,startx,offscreen,buffers)
	fields[0] = new_field(TAG_ROWS,  TAG_COLS,  0,     0, 0, 0);
	fields[1] = new_field(NOTE_ROWS, NOTE_COLS, 0+TAG_ROWS+GAP-1, 0, 0, 0);
	fields[2] = NULL;	// TODO: Examples do this, check why.

	set_field_back(fields[0], A_UNDERLINE);
	field_opts_off(fields[0], O_AUTOSKIP);
	field_opts_on(fields[0], O_BLANK);
	//set_field_type(fields[0], TYPE_ALPHA, 0);
	set_field_back(fields[1], A_UNDERLINE);
	field_opts_off(fields[1], O_AUTOSKIP);
	field_opts_on(fields[1], O_BLANK);

	form = new_form(fields);
	scale_form(form, &rows, &cols);
	getmaxyx(stdscr, maxrows, maxcols);	// This is a macro, so no pointers.

	//newwin(rows,cols,starty,startx)
	winrows = rows + FORM_START_Y + GAP;
	wincols = cols + FORM_START_X + GAP;
	window = newwin(winrows, wincols, (maxrows-winrows)/2, (maxcols-wincols)/2);
	keypad(window, TRUE);	// TODO: Needed?

	box(window, 0, 0);
	mvwaddstr(window, 0,       (wincols-strlen(NOTE_HEADER))/2, NOTE_HEADER);
	mvwaddstr(window, FORM_START_Y,     GAP, TAG_STR);
	mvwaddstr(window, FORM_START_Y+TAG_ROWS+GAP-1, GAP, NOTE_STR);
	mvwaddstr(window, winrows-1, (wincols-strlen(DONE_STR))/2, DONE_STR);

	set_form_win(form, window);
	// derwin(window,rows,cols,rely,relx);
	set_form_sub(form, derwin(window, rows, cols, FORM_START_Y, FORM_START_X));

	mvprintw(LINES-2, 0, "moomoo");
	post_form(form);
	refresh();
	wrefresh(window);

	wgetch(window);

	unpost_form(form);
	free_form(form);
	free_field(fields[0]);
	free_field(fields[1]);

	endwin();

	return 0;
}
