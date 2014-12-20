#include <stdlib.h>
#include <ncurses.h>
#include <form.h>

// TODO: The tutorial I used didn't use nice math just magic numbers. So I
//       foolishly followed along with that. Fix it.
int main()
{
	FIELD *fields[3];
	FORM *form;
	WINDOW *window;
	int rows, cols;
	int maxrows, maxcols;

	initscr();
	start_color();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);	// TODO: Needed?

	init_pair(1, COLOR_WHITE, COLOR_BLUE);

	//new_field(rows,cols,starty,startx,offscreen,buffers)
	fields[0] = new_field(1, 10, 0, 0, 0, 0);
	fields[1] = new_field(4, 20, 2, 0, 0, 0);
	fields[2] = NULL;	// TODO: Examples do this, check why.

	set_field_back(fields[0], A_UNDERLINE);
	field_opts_off(fields[0], O_AUTOSKIP);
	set_field_back(fields[1], A_UNDERLINE);
	field_opts_off(fields[1], O_AUTOSKIP);

	form = new_form(fields);
	scale_form(form, &rows, &cols);
	getmaxyx(stdscr, maxrows, maxcols);	// This is a macro, so no pointers.

	window = newwin(rows+4, cols+14, (maxrows-(rows+6))/2, (maxcols-(cols+6))/2);
	keypad(window, TRUE);	// TODO: Needed?

	box(window, 0, 0);
	mvwaddstr(window, 0, cols/2, "New Note");
	mvwaddstr(window, 2, 2, "Tag: ");
	mvwaddstr(window, 4, 2, "Note: ");

	set_form_win(form, window);
	// derwin(window,rows,cols,rely,relx);
	set_form_sub(form, derwin(window, rows, cols, 2, 10));

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
