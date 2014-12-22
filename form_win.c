#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <ncurses.h>
#include <form.h>
#include <menu.h>

void trim_trailing_whitespace(char *str);
void view_note_form(char *tag, char *note);
void draw_list_menu(void);
ITEM ** load_items(int *items_len);

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
	initscr();
	start_color();
	cbreak();
	/* Enter is 13 with it, and 10 without it. This should have made a check
	 *  against KEY_ENTER work, but it didn't. */
	nonl();
	noecho();
	keypad(stdscr, TRUE);	// TODO: Needed?

	init_pair(1, COLOR_WHITE, COLOR_BLUE);

	draw_list_menu();

	endwin();

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

	post_form(form);
	wrefresh(window);

	//while((inp = wgetch(window)) != 3)		// 3 is Ctrl-C.
	while((inp = wgetch(window)) != 13)		// 13 is enter with nonl(), else 10;
	{
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
	delwin(window);

}

void trim_trailing_whitespace(char *str)
{
	int i = strlen(str)-1;
	for( ; i >= 0 && isspace(str[i]); i--)
		;
	str[i+1] = '\0';
}

#define ROWS_FOR_HELP 3
#define NOTE_WINDOW_STR " Notes "
#define HELP_WINDOW_STR " HELP "
#define HELP_OPTIONS    "'a' to add note"
void draw_list_menu(void)
{
	WINDOW *menu_win, *help_win;
	MENU *menu;
	ITEM **items;
	char *tag, *note;
	int maxrows, maxcols;
	int i, inp;
	int items_len;

	curs_set(0);		// Invisible cursor.
	getmaxyx(stdscr, maxrows, maxcols);	// This is a macro, so no pointers.
	menu_win = newwin(maxrows-ROWS_FOR_HELP, maxcols, 0, 0);
	help_win = newwin(ROWS_FOR_HELP, maxcols, maxrows-ROWS_FOR_HELP, 0);
	box(menu_win, 0, 0);
	box(help_win, 0, 0);
	mvwprintw(menu_win, 0, (maxcols-strlen(NOTE_WINDOW_STR))/2, NOTE_WINDOW_STR);
	mvwprintw(help_win, 0, (maxcols-strlen(HELP_WINDOW_STR))/2, HELP_WINDOW_STR);
	mvwprintw(help_win, ROWS_FOR_HELP/2, GAP, HELP_OPTIONS);

	keypad(menu_win, TRUE);

	// Load items from the text file where we store it.
	items = load_items(&items_len);

	menu = new_menu(items);
	set_menu_win(menu, menu_win);
	set_menu_sub(menu, derwin(menu_win, maxrows-ROWS_FOR_HELP-GAP, maxcols-GAP, GAP, GAP));
	set_menu_format(menu, maxrows-ROWS_FOR_HELP-2*GAP, 1);
	set_menu_pad(menu, '|');
	set_menu_spacing(menu, 4, 0, 0);
	set_menu_mark(menu, " o ");

	post_menu(menu);
	wrefresh(menu_win);
	wrefresh(help_win);

	// TODO: Make it exit on an actual value, maybe ctrl-c or esc.
	while((inp = wgetch(menu_win)) != 13)		// This is enter.
	{
		switch(inp)
		{
			case KEY_DOWN:
				menu_driver(menu, REQ_DOWN_ITEM);
				break;
			case KEY_UP:
				menu_driver(menu, REQ_UP_ITEM);
				break;
			case KEY_NPAGE:
				menu_driver(menu, REQ_SCR_DPAGE);
				break;
			case KEY_PPAGE:
				menu_driver(menu, REQ_SCR_UPAGE);
				break;
			case 'a':
				tag = malloc(TAG_MAX_SIZE * sizeof(char));
				note = malloc(NOTE_MAX_SIZE * sizeof(char));

				curs_set(1);		// Normal cursor.
				// TODO: Validate tag and note, inside the function, or here.
				view_note_form(tag, note);
				curs_set(0);		// Invisible cursor.

				unpost_menu(menu);
				set_menu_items(menu, NULL);

				// TODO: Call realloc less often.
				items = realloc(items, (items_len+1) * sizeof(ITEM *));
				items[items_len-1] = new_item(tag, note);
				items[items_len] = NULL;
				items_len++;

				// These pointers will be freed by looping through items later.
				tag = note = NULL;

				set_menu_items(menu, items);
				post_menu(menu);

				/* TODO: I have no idea why, but reposting the menu breaks the
				 * window, even if you do wrefresh(), so just redraw the border
				 * and the title */
				box(menu_win, 0, 0);
				mvwprintw(menu_win, 0, (maxcols-strlen(NOTE_WINDOW_STR))/2, NOTE_WINDOW_STR);
				wrefresh(menu_win);
				break;
			default:
				// TODO: This will become the edit functionality. Soon.
				unpost_menu(menu);
				set_menu_items(menu, NULL);
				free_item(items[2]);
				items[2] = NULL;
				items[2] = new_item("BBBBBB", "SADGHd");
				set_menu_items(menu, items);
				post_menu(menu);
				/* TODO: I have no idea why, but reposting the menu breaks the
				 * window, even if you do wrefresh(), so just redraw the border
				 * and the title */
				box(menu_win, 0, 0);
				mvwprintw(menu_win, 0, (maxcols-strlen(NOTE_WINDOW_STR))/2, NOTE_WINDOW_STR);
				wrefresh(menu_win);
				break;
		}
	}

	// TODO: Writeback to the NOTES_FILE.
	unpost_menu(menu);
	free_menu(menu);
	for(i = 0; i < items_len-1; i++)
	{
		free((void *)items[i]->name.str);
		free((void *)items[i]->description.str);
		free_item(items[i]);
	}
	free(items);
	delwin(help_win);
	delwin(menu_win);
}

// TODO: Change later. Used for testing now.
#define NOTES_FILE "testfile.txt"
ITEM ** load_items(int *items_len)
{
	FILE *f;
	char ch;
	char *tag, *note, *line;
	ITEM **items = NULL;
	int line_count, i;

	// Get the line count, so we can initialize the size of the items array.
	f = fopen(NOTES_FILE, "r+");
	line_count = 0;
	while((ch=fgetc(f)) != EOF)
	{
		if(ch == '\n')
			line_count++;
	}
	rewind(f);

	items = realloc(items, (line_count+1) * sizeof(ITEM *));
	line = malloc(sizeof(char) * (TAG_MAX_SIZE + NOTE_MAX_SIZE));
	for(i = 0; i < line_count; i++)
	{
		tag = malloc(TAG_MAX_SIZE * sizeof(char));
		note = malloc(NOTE_MAX_SIZE * sizeof(char));
		// Read in all characters on the line, but the newline.
		fscanf(f, "%[^\n]\n", line);
		strcpy(tag, strtok(line, "\t"));
		strcpy(note, strtok(NULL, "\t"));
		items[i] = new_item(tag, note);
	}
	free(line);
	items[line_count] = NULL;
	fclose(f);

	// entries + NULL at the end.
	*items_len = line_count + 1;
	return items;
}
