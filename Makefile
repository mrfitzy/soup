main_c := main.c
test_sources := test_main.c test.c test_shims.c test_shims_proof.c
other_files := $(main_c) $(test_sources)
common_sources := $(filter-out $(other_files), $(wildcard *.c))
header_files := $(wildcard *.h)

soup_source_files := $(common_sources) $(main_c)

soup: clean_build_logs $(soup_source_files) $(header_files)
	bash -c 'clang -g -Werror -Wall -Wextra -Wpadded -o soup $(soup_source_files) \
	> >(tee -a build.log) 2> >(tee -a errors.err >&2)'

soup_args := ops.bin cb_ops.bin DMG_ROM.bin

run: soup
	./soup $(soup_args)

print: soup
	./soup -p $(soup_args)

test_source_files := $(common_sources) $(test_sources)

test_soup: clean_build_logs $(test_source_files) $(header_files)
	bash -c 'clang -g -Werror -Wall -Wextra -o test_soup $(test_source_files) \
	-I /opt/homebrew/Cellar/cmocka/1.1.7/include -l cmocka \
	> >(tee -a build.log) 2> >(tee -a errors.err >&2)'

test: test_soup
	./test_soup $(soup_args)

test2: test_soup
	./test.py ./test_soup $(soup_args)

all: soup test_soup

clean:
	rm -f soup test_soup

clean_build_logs:
	rm -f build.log errors.err

clean_all: clean clean_build_logs
