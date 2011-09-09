all:
	gcc -g3 `pkg-config --cflags --libs webkitgtk-3.0` main.c -std=c99 -o icongen

