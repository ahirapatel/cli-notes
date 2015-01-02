// TODO: Everything really shouldn't be in a single file...
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <ncurses.h>
#include <form.h>
#include <menu.h>

typedef enum {TAG, NOTE} filter_type;
typedef struct
{
	char *str;
	filter_type type;
} filter_item;

void trim_trailing_whitespace(char *str);
void view_note_form(char *tag, char *note, bool init);
void draw_list_menu(void);
ITEM ** load_items(int *items_len);
void store_items(ITEM **items, int items_len);
void shift_items_left(ITEM **items, int start_index, int length);
int filter_on(ITEM **src, ITEM **dst, int length, filter_item f);
void draw_menu_win(WINDOW *win);
void draw_help_win(WINDOW *win, char **help_array, int help_array_len);
int find_item_index(ITEM **items, ITEM *key);
int refilter(ITEM **src, ITEM **dst, int items_len, filter_item *seq, int seq_len);
char ** generate_help_str(int *rows_return);
void get_user_str(WINDOW *win, char *prompt, char *retval);

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

void view_note_form(char *tag, char *note, bool init)
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

	// Make new fields and initialize them
	//new_field(rows,cols,starty,startx,offscreen,buffers)
	fields[0] = new_field(TAG_ROWS,  TAG_COLS,  0,                0, 0, 0);
	fields[1] = new_field(NOTE_ROWS, NOTE_COLS, 0+TAG_ROWS+GAP-1, 0, 0, 0);
	fields[2] = NULL;
	set_field_back(fields[0], A_UNDERLINE);
	field_opts_off(fields[0], O_AUTOSKIP);
	field_opts_on(fields[0], O_BLANK);
	set_field_back(fields[1], A_UNDERLINE);
	field_opts_off(fields[1], O_AUTOSKIP);
	field_opts_off(fields[1], O_WRAP);
	field_opts_on(fields[1], O_BLANK);
	// Make a new form with the fields.
	form = new_form(fields);

	// Make a new window for the form, with the appropriate size.
	scale_form(form, &rows, &cols);		// Get minimum size needed for form.
	getmaxyx(stdscr, maxrows, maxcols);	// This is a macro, so no pointers.
	winrows = rows + FORM_START_Y + GAP;
	wincols = cols + FORM_START_X + GAP;
	// Center the form+window in the center of the screen.
	//newwin(rows,cols,starty,startx)
	window = newwin(winrows, wincols, (maxrows-winrows)/2, (maxcols-wincols)/2);
	keypad(window, TRUE);	// TODO: Needed?

	// Draw a border around the form and print form info text.
	box(window, 0, 0);
	mvwaddstr(window, 0,       (wincols-strlen(NOTE_HEADER))/2, NOTE_HEADER);
	mvwaddstr(window, FORM_START_Y,     GAP, TAG_STR);
	mvwaddstr(window, FORM_START_Y+TAG_ROWS+GAP-1, GAP, NOTE_STR);
	mvwaddstr(window, winrows-1, (wincols-strlen(DONE_STR))/2, DONE_STR);

	// Set form window and subwindow.
	set_form_win(form, window);
	// derwin(window,rows,cols,rely,relx);
	set_form_sub(form, derwin(window, rows, cols, FORM_START_Y, FORM_START_X));

	// Initialize the form with the strings passed in if needed.
	if(init)
	{
		set_field_buffer(fields[0], 0, tag);
		set_field_buffer(fields[1], 0, note);
	}

	// Draw form and window.
	post_form(form);
	form_driver(form, REQ_END_LINE);	// if(init) then move to end of buffer.
	curs_set(1);		// Normal cursor.
	wrefresh(window);

	// TODO: Why is the enter key defined in ncurses not working for me?
	// Exit the form with enter, else process the request.
	while((inp = wgetch(window)) != 13)		// 13 is enter with nonl(), else 10;
	{
		switch(inp)
		{
			// TODO: Check for ncurses tab definition.
			// Tab, up arrow and down arrow are equal in a two field form.
			case 9:			// 9 is tab.
			case KEY_DOWN:
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
			case KEY_BACKSPACE:		// TODO: Why does this define not work on my system?
			case 127:	// 127 is Backspace on my system.
				form_driver(form, REQ_PREV_CHAR);
				form_driver(form, REQ_DEL_CHAR);
				break;
			default:		// Pass anything else to the field.
				form_driver(form, inp);
				break;
		}
	}

	// Need to force validation so that all values are actually stored into
	// the buffers.
	form_driver(form, REQ_VALIDATION);

	// The string obtained from field buffer always has the same length
	// and is padded with spaces if there was not enough user input to fill it.
	strcpy(tag, field_buffer(fields[0], 0));
	strcpy(note, field_buffer(fields[1], 0));

	// Trim the excess whitespace at the end.
	trim_trailing_whitespace(tag);
	trim_trailing_whitespace(note);

	// Clean up.
	unpost_form(form);
	free_form(form);
	free_field(fields[0]);
	free_field(fields[1]);
	delwin(window);

	curs_set(0);		// Invisible cursor.
}

void trim_trailing_whitespace(char *str)
{
	int i = strlen(str)-1;
	for( ; i >= 0 && isspace(str[i]); i--)
		;
	str[i+1] = '\0';
}

// TODO: Better resizing.
#define NOTE_WINDOW_STR " Notes "
#define HELP_WINDOW_STR " HELP "
#define HELP_DELIM " | | "
#define ADD_NOTE "'a' to add note" HELP_DELIM
#define EDIT_NOTE "'e' to edit note" HELP_DELIM
#define DELETE_NOTE "'d' to delete note" HELP_DELIM
#define TAG_FILTER_NOTE "'f' to filter on tag" HELP_DELIM
#define NOTE_FILTER_NOTE "'F' to filter on note" HELP_DELIM
#define CLEAR_NOTE "'c' to clear filter" HELP_DELIM
#define UNDO_NOTE "'u' to undo 1 filter level"
#define FILTER_TAG_STRING   "Enter text to filter tags: "
#define FILTER_NOTE_STRING   "Enter text to filter notes: "
void draw_list_menu(void)
{
	WINDOW *menu_win, *help_win;
	MENU *menu;
	ITEM **items, **filtered = NULL;
	ITEM *temp;
	char *tag, *note, **help_array;
	filter_item *filter_seq = NULL;
	int maxrows, maxcols, rows_for_help, help_array_len;
	int i, inp, idx, menu_idx;
	int items_len, filter_len, filter_seq_len = 0;

	curs_set(0);		// Invisible cursor.
	getmaxyx(stdscr, maxrows, maxcols);	// This is a macro, so no pointers.
	help_array = generate_help_str(&help_array_len);
	rows_for_help = help_array_len + GAP;
	menu_win = newwin(maxrows-rows_for_help, maxcols, 0, 0);
	help_win = newwin(rows_for_help, maxcols, maxrows-rows_for_help, 0);

	keypad(menu_win, TRUE);

	// Load items from the text file where we store it.
	items = load_items(&items_len);

	menu = new_menu(items);
	set_menu_win(menu, menu_win);
	set_menu_sub(menu, derwin(menu_win, maxrows-rows_for_help-GAP, maxcols-GAP, GAP, GAP));
	set_menu_format(menu, maxrows-rows_for_help-2*GAP, 1);
	set_menu_pad(menu, '|');
	set_menu_spacing(menu, 4, 0, 0);
	set_menu_mark(menu, " o ");

	post_menu(menu);
	draw_menu_win(menu_win);
	draw_help_win(help_win, help_array, help_array_len);

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
			case 'a':	// Add note.
				tag = malloc(TAG_MAX_SIZE * sizeof(char));
				note = malloc(NOTE_MAX_SIZE * sizeof(char));
				temp = current_item(menu);

				// TODO: Validate tag and note, inside the function, or here.
				view_note_form(tag, note, FALSE);

				unpost_menu(menu);
				set_menu_items(menu, NULL);

				// TODO: Call realloc less often.
				items = realloc(items, (items_len+1) * sizeof(ITEM *));
				items[items_len-1] = new_item(tag, note);
				items[items_len] = NULL;
				items_len++;

				// These pointers will be freed by looping through items later.
				tag = note = NULL;

				if(filtered)
				{
					filtered = realloc(filtered, items_len * sizeof(ITEM *));
					filter_len = refilter(items, filtered, items_len, filter_seq, filter_seq_len);
					set_menu_items(menu, filtered);
				}
				else
				{
					set_menu_items(menu, items);
				}

				// Restore cursor position.
				set_current_item(menu, temp);
				post_menu(menu);

				draw_menu_win(menu_win);
				break;
			case 'e':	// Edit a note.
				temp = current_item(menu);
				tag = (char *) item_name(temp);
				note = (char *) item_description(temp);
				// menu_idx and idx only differ when filtered.
				menu_idx = item_index(temp);		// index in menu.
				idx = find_item_index(items, temp);	// index in items array.
				// If no item is selected do nothing.
				if(!temp)
					continue;

				unpost_menu(menu);
				set_menu_items(menu, NULL);

				// TODO: Validate tag and note, inside the function, or here.
				view_note_form(tag, note, TRUE);
				free_item(items[idx]);
				items[idx] = new_item(tag, note);

				// If filtered, need to refilter when we edit an item.
				if(filtered)
				{
					filter_len = refilter(items, filtered, items_len, filter_seq, filter_seq_len);
					set_menu_items(menu, filtered);
					// Restore cursor position, unless it was removed when filtering.
					if(set_current_item(menu, filtered[menu_idx]) != E_OK)
						set_current_item(menu, filtered[menu_idx == 0 ? 0 : --menu_idx]);
				}
				else
				{
					set_menu_items(menu, items);
					// Restore cursor position.
					set_current_item(menu, items[menu_idx]);
				}

				post_menu(menu);

				draw_menu_win(menu_win);
				break;
			case 'd':	// Delete a note.
				temp = current_item(menu);
				tag = (char *) item_name(temp);
				note = (char *) item_description(temp);
				// menu_idx and idx only differ when filtered.
				menu_idx = item_index(temp);		// index in menu.
				idx = find_item_index(items, temp);	// index in items array.
				// If no item is selected do nothing.
				if(!temp)
					continue;

				unpost_menu(menu);
				set_menu_items(menu, NULL);
				// Delete old item information.
				free(tag);
				free(note);
				free_item(items[idx]);

				items_len--;
				// TODO: Stop reallocing so much, keep track of free space.
				items = realloc(items, items_len * sizeof(ITEM *));
				// Shift items left to take up space from deletion.
				shift_items_left(items, idx, items_len);

				// If filtered, need to refilter when we delete an item.
				if(filtered)
				{
					filter_len = refilter(items, filtered, items_len, filter_seq, filter_seq_len);
					set_menu_items(menu, filtered);
					// Restore cursor position.
					set_current_item(menu, filtered[menu_idx == 0 ? 0 : --menu_idx]);
				}
				else
				{
					set_menu_items(menu, items);
					// Restore cursor position.
					set_current_item(menu, items[menu_idx == 0 ? 0 : --menu_idx]);
				}
				post_menu(menu);

				draw_menu_win(menu_win);
				break;
			case 'F':		// Filter by description.
			case 'f':		// Filter by tags.
				// TODO: Filter by description.

				// We may need to redo our filtering, after an edit for example, so
				// store all our searches.
				filter_seq = realloc(filter_seq, (filter_seq_len+1) * sizeof(filter_item));
				filter_seq[filter_seq_len].str = malloc(TAG_MAX_SIZE * sizeof(char));
				filter_seq[filter_seq_len].type = inp == 'f' ? TAG : NOTE;

				get_user_str(help_win, inp == 'f' ? FILTER_TAG_STRING : FILTER_NOTE_STRING,
						filter_seq[filter_seq_len].str);

				unpost_menu(menu);
				// Don't want to modify something attached to the menu, so detach it.
				set_menu_items(menu, NULL);
				// Filter the original notes list or an already filtered one.
				if(!filtered)
				{
					filtered = calloc(items_len, sizeof(ITEM *));
					filter_len = filter_on(items, filtered, items_len, filter_seq[filter_seq_len]);
				}
				else
				{
					filter_len = filter_on(filtered, filtered, filter_len, filter_seq[filter_seq_len]);
				}
				filter_seq_len++;

				set_menu_items(menu, filtered);
				post_menu(menu);

				draw_help_win(help_win, help_array, help_array_len);
				draw_menu_win(menu_win);
				break;
			case 'u':	// Undo a filter level.
				if(filter_seq_len >= 2)
				{
					free(filter_seq[filter_seq_len-1].str);
					filter_seq = realloc(filter_seq, (filter_seq_len-1) * sizeof(filter_item));
					filter_seq_len--;

					unpost_menu(menu);
					set_menu_items(menu, NULL);

					filter_len = refilter(items, filtered, items_len, filter_seq, filter_seq_len);
					set_menu_items(menu, filtered);

					post_menu(menu);
					draw_menu_win(menu_win);
					break;
				}		// else fallthrough
			case 'c':		// Clear filter.
				if(filtered)
				{
					// Repost all notes.
					unpost_menu(menu);
					set_menu_items(menu, items);
					post_menu(menu);
					draw_menu_win(menu_win);

					// Free all memory.
					free(filtered);
					for(i = 0; i < filter_seq_len; i++)
						free(filter_seq[i].str);
					free(filter_seq);
					// Set to initial values.
					filter_seq_len = 0;
					filtered = NULL;
					filter_seq = NULL;
				}
				break;
			default:
				break;
		}
	}

	// Writeback to text file.
	store_items(items, items_len);
	// Free all allocated resources.
	unpost_menu(menu);
	free_menu(menu);
	for(i = 0; i < items_len-1; i++)
	{
		// TODO: Just use those nice item_name/desc functions.
		free((void *)items[i]->name.str);
		free((void *)items[i]->description.str);
		free_item(items[i]);
	}
	free(items);
	if(filtered)
	{
		free(filtered);
		for(i = 0; i < filter_seq_len; i++)
			free(filter_seq[i].str);
		free(filter_seq);
	}
	for(i = 0; i < help_array_len; i++)
		free(help_array[i]);
	free(help_array);
	delwin(help_win);
	delwin(menu_win);
}

// TODO: Change later. Used for testing now.
#define NOTES_FILE "testfile.txt"
// TODO: Assumes test file has not been modified by outside sources.
// TODO: Should probably handle errors.
ITEM ** load_items(int *items_len)
{
	FILE *f;
	char ch;
	char *tag, *note, *line;
	ITEM **items = NULL;
	int line_count, i;

	// Get the line count, so we can initialize the size of the items array.
	f = fopen(NOTES_FILE, "r");
	if(f == NULL)	// No file present, start with nothing.
	{
		*items_len = 1;
		items = realloc(items, sizeof(ITEM *));
		items[0] = NULL;
		return items;
	}
	line_count = 0;
	while((ch=fgetc(f)) != EOF)
	{
		if(ch == '\n')
			line_count++;
	}
	rewind(f);

	// Allocate items array and create items.
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

// TODO: Should probably handle errors.
void store_items(ITEM **items, int items_len)
{
	int i;
	FILE *f = fopen(NOTES_FILE, "w");
	for(i = 0; i < items_len-1; i++)
		// TODO: Just use those nice item_name/desc functions.
		fprintf(f, "%s\t%s\n", items[i]->name.str, items[i]->description.str);
	fclose(f);
}

void shift_items_left(ITEM **items, int start_index, int length)
{
	int i;
	for(i = start_index; i < length-1; i++)
		items[i] = items[i+1];
	items[length-1] = NULL;
}

int refilter(ITEM **src, ITEM **dst, int items_len, filter_item *seq, int seq_len)
{
	int i, filter_len;
	filter_len = filter_on(src, dst, items_len, seq[0]);
	// Filter the filtered data.
	for(i = 1; i < seq_len; i++)
		filter_len = filter_on(dst, dst, filter_len, seq[i]);
	return filter_len;
}

int filter_on(ITEM **src, ITEM **dst, int length, filter_item f)
{
	int i, filter_idx;
	const char *(*item_info)(const ITEM *) = f.type == TAG ? item_name : item_description;
	for(i = 0, filter_idx = 0; i < length-1; i++)
	{
		if(strstr(item_info(src[i]), f.str))
			dst[filter_idx++] = src[i];
	}
	dst[filter_idx] = NULL;
	return filter_idx+1;
}

/* TODO: I have no idea why, but reposting the menu breaks the
 * window, even if you do wrefresh(), so just redraw the border
 * and the title */
void draw_menu_win(WINDOW *win)
{
	int rows, cols;
	getmaxyx(win, rows, cols);
	box(win, 0, 0);
	mvwprintw(win, 0, (cols-strlen(NOTE_WINDOW_STR))/2, NOTE_WINDOW_STR);
	wrefresh(win);
}

char ** generate_help_str(int *rows_return)
{
	char *help_options[] = {ADD_NOTE, EDIT_NOTE, DELETE_NOTE, TAG_FILTER_NOTE,
							NOTE_FILTER_NOTE, CLEAR_NOTE, UNDO_NOTE};
	int help_options_len = sizeof(help_options)/sizeof(char *);
	int rows_needed=1, curr_col=0, i=0;
	unsigned maxrows, maxcols;
	char **res=NULL;

	getmaxyx(stdscr, maxrows, maxcols);
	maxcols -= 2*GAP; // Padding on each side.
	res = realloc(res, rows_needed * sizeof(char *));
	res[0] = calloc(maxcols, sizeof(char));

	// NOTE: Will print potentially nothing on small windows and will
	//       waste space at the bottom too.
	while(i < help_options_len && rows_needed < help_options_len)
	{
		if((curr_col + strlen(help_options[i])) > maxcols)
		{
			rows_needed++;
			res = realloc(res, rows_needed * sizeof(char *));
			res[rows_needed-1] = calloc(maxcols, sizeof(char));
			curr_col = GAP;
		}
		else
		{
			curr_col += strlen(help_options[i]);
			strcat(res[rows_needed-1], help_options[i]);
			i++;
		}
	}

	*rows_return = rows_needed;
	return res;
}

void draw_help_win(WINDOW *win, char **help_array, int help_array_len)
{
	int rows, cols, i;
	getmaxyx(win, rows, cols);
	box(win, 0, 0);
	mvwprintw(win, 0, (cols-strlen(HELP_WINDOW_STR))/2, HELP_WINDOW_STR);
	for(i = 0; i < help_array_len; i++)
		mvwprintw(win, i+1, GAP, help_array[i]);
	wrefresh(win);
}

// TODO: Or just store the index in the items list with user pointers.
int find_item_index(ITEM **items, ITEM *needle)
{
	int i;
	for(i = 0; *items; items++, i++)
	{
		if(*items == needle)
			return i;
	}
	return -1;
}

void get_user_str(WINDOW *win, char *prompt, char *retval)
{
	wclear(win);

	mvwprintw(win, 0, GAP, prompt);
	curs_set(1);
	echo();
	wgetnstr(win, retval, (TAG_MAX_SIZE-1) * sizeof(char));
	noecho();
	curs_set(0);
}

