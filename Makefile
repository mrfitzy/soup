soup: main.c
	clang -Werror -Wall -Wextra -o soup main.c

run: soup
	./soup ops.bin cb_ops.bin

clean:
	rm -f soup
