# main recipes

#CXX = g++
#CXX = clang++

IMGUI_DIR = ../imgui

main_c := main.c
test_sources := test_main.c test.c test_shims.c test_shims_proof.c ui.mm
test_sources += $(IMGUI_DIR)/backends/imgui_impl_sdl3.cpp $(IMGUI_DIR)/backends/imgui_impl_metal.mm
test_sources += $(IMGUI_DIR)/imgui.cpp $(IMGUI_DIR)/imgui_demo.cpp $(IMGUI_DIR)/imgui_draw.cpp $(IMGUI_DIR)/imgui_tables.cpp $(IMGUI_DIR)/imgui_widgets.cpp
other_files := $(main_c) $(test_sources)
common_sources := $(filter-out $(other_files), $(wildcard *.c))
header_files := $(wildcard *.h)

SOURCES = $(common_sources) $(main_c)

CFLAGS = -g -Werror -Wall -Wextra -Wpadded
TEE = > >(tee -a build.log) 2> >(tee -a errors.err >&2)

CXXFLAGS += `pkg-config --cflags sdl3`
CXXFLAGS += -I/usr/local/include -I/opt/local/include
CXXFLAGS += -std=c++11 -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends
CXXFLAGS += -Wall -Wformat

clean_build_logs:
	rm -f build.log errors.err

OBJS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))

%.o:%.c
	bash -c 'clang $(CFLAGS) -c -o $@ $< $(TEE)'

%.o:$(IMGUI_DIR)/backends/%.mm
	$(CXX) $(CXXFLAGS) -ObjC++ -fobjc-weak -fobjc-arc -c -o $@ $<

%.o:$(IMGUI_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(IMGUI_DIR)/backends/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

test.o: test.c
	bash -c 'clang -g -Werror -Wall -Wextra -c -o $@ \
	-I /opt/homebrew/Cellar/cmocka/1.1.7/include $< $(TEE)'

test_shims.o: test_shims.c
	bash -c 'clang -g -Werror -Wall -Wextra -c -o $@ \
	-I /opt/homebrew/Cellar/cmocka/1.1.7/include $< $(TEE)'

ui.o:ui.mm
	bash -c '$(CXX) $(CXXFLAGS) -ObjC++ -fobjc-weak -fobjc-arc -c -o $@ $< $(TEE)'

soup: $(OBJS)
	clang -o soup $^ $(CFLAGS)

soup_args := ops.bin cb_ops.bin DMG_ROM.bin

run: soup
	./soup $(soup_args)

print: soup
	./soup -p $(soup_args)

test_source_files := $(common_sources) $(test_sources)

SOURCES = $(common_sources) $(test_sources)
OBJS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))

LIBS = -framework Metal -framework MetalKit -framework Cocoa -framework IOKit -framework CoreVideo -framework QuartzCore
LIBS += `pkg-config --libs sdl3`
LIBS += -L/usr/local/lib -L/opt/local/lib -l cmocka

test_soup: $(test_source_files) $(header_files)
	bash -c '$(CXX) $(CXXFLAGS) -g -Werror -Wall -Wextra -o test_soup $(test_source_files) \
	-I /opt/homebrew/Cellar/cmocka/1.1.7/include -l cmocka \
	> >(tee -a build.log) 2> >(tee -a errors.err >&2)'

test_soup2: $(OBJS)
	$(CXX) -o test_soup $^ $(CXXFLAGS) $(LIBS)

test: test_soup
	./test_soup $(soup_args)

test2: test_soup
	./test.py ./test_soup $(soup_args)

all: soup test_soup

clean:
	rm -f soup test_soup *.o

clean_all: clean clean_build_logs
