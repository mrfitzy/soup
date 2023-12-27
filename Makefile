source_files := $(wildcard *.c)
header_files := $(wildcard *.h)

soup: clean_build_logs $(source_files) $(header_files)
	bash -c 'clang -g -Werror -Wall -Wextra -o soup $(source_files) \
	> >(tee -a build.log) 2> >(tee -a errors.err >&2)'

run: soup
	./soup ops.bin cb_ops.bin DMG_ROM.bin

print: soup
	./soup -p ops.bin cb_ops.bin DMG_ROM.bin

clean:
	rm -f soup

clean_build_logs:
	rm -f build.log errors.err

clean_all: clean clean_build_logs
