#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <ncurses.h>
#include <form.h>

void trim_trailing_whitespace(char *str);
void view_note_form(char *tag, char *note);

#define TAG_ROWS  1
#define TAG_COLS  10
#define NOTE_ROWS 4
#define NOTE_COLS 20

#define TAG_MAX_SIZE     TAG_ROWS*TAG_COLS + 1
#define NOTE_MAX_SIZE    NOTE_ROWS*NOTE_COLS + 1
#define GAP 2

#define FORM_START_Y (GAP)
#define FORM_START_X (strlen(NOTE_STR)+GAP)

#define NOTE_HEADER "New Note"
#define TAG_STR     "Tag (Optional): "
#define NOTE_STR    "Note (Required): "
#define DONE_STR    "Hit Enter When Done"

int main()
{
	char *note, *tag;
	tag = malloc(sizeof(char)*TAG_MAX_SIZE);
	note = malloc(sizeof(char)*NOTE_MAX_SIZE);

	initscr();
	start_color();
	cbreak();
	/* Enter is 13 with it, and 10 without it. This should have made a check
	 *  against KEY_ENTER work, but it didn't. */
	nonl();
	noecho();
	keypad(stdscr, TRUE);	// TODO: Needed?

	init_pair(1, COLOR_WHITE, COLOR_BLUE);

	view_note_form(tag, note);

	endwin();

	printf("%s %s\n", note, tag);
	printf("%s %s\n", tag, note);

	return 0;
}

void view_note_form(char *tag, char *note)
{
	FIELD *fields[3];
	FORM *form;
	WINDOW *window;
	int rows, cols;
	int maxrows, maxcols;
	int winrows, wincols;
	int inp;

	/* TODO: This would probably be better if we I didn't destroy the
	 * form everytime, but rather just hide it (and clear the contents). */

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

	//while((inp = wgetch(window)) != 3)		// 3 is Ctrl-C.
	while((inp = wgetch(window)) != 13)		// 13 is enter with nonl(), else 10;
	{
		mvprintw(LINES-4, 0, "hey:%d", inp);
		refresh();
		switch(inp)
		{
			case 9:			// 9 is tab.
			case KEY_DOWN:
				form_driver(form, REQ_NEXT_FIELD);
				// Go to end of the buffer, in case text is already entered.
				form_driver(form, REQ_END_LINE);
				break;
			case KEY_UP:
				form_driver(form, REQ_NEXT_FIELD);
				// Go to end of the buffer, in case text is already entered.
				form_driver(form, REQ_END_LINE);
				break;
			case KEY_RIGHT:
				form_driver(form, REQ_NEXT_CHAR);
				break;
			case KEY_LEFT:
				form_driver(form, REQ_PREV_CHAR);
				break;
			case KEY_DC:	// Delete key.
				form_driver(form, REQ_DEL_CHAR);
				break;
			case 127:	// 127 is Backspace.
				form_driver(form, REQ_PREV_CHAR);
				form_driver(form, REQ_DEL_CHAR);
				break;
			default:
				form_driver(form, inp);
				break;
		}
	}

	// Need to force validation so that all values are actually stored into
	// the buffers.
	form_driver(form, REQ_VALIDATION);

	// The string obtained from field buffer always has the same length
	// and is padded with spaces if there was not enough user input.
	strcpy(tag, field_buffer(fields[0], 0));
	strcpy(note, field_buffer(fields[1], 0));

	// Trim the excess whitespace at the end.
	trim_trailing_whitespace(tag);
	trim_trailing_whitespace(note);

	unpost_form(form);
	free_form(form);
	free_field(fields[0]);
	free_field(fields[1]);

}

void trim_trailing_whitespace(char *str)
{
	int i = strlen(str)-1;
	for( ; i >= 0 && isspace(str[i]); i--)
		;
	str[i+1] = '\0';
}

