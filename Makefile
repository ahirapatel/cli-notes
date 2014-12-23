all:
	gcc -o form_win form_win.c -Wall -lncurses -lform -lmenu -g
clean:
	rm form_win
