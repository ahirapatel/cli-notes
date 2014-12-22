all:
	gcc -Wall -lncurses -lform -lmenu -g -o form_win form_win.c
clean:
	rm form_win
