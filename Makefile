all:
	gcc -o notes notes.c -Wall -lncurses -lform -lmenu -g
clean:
	rm notes
