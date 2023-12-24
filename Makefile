soup: main.c
	clang -g -Werror -Wall -Wextra -o soup main.c

run: soup
	./soup ops.bin cb_ops.bin DMG_ROM.bin

clean:
	rm -f soup
