IMGUI_DIR := ../imgui

main_c := main.c
test_sources := test_main.c test.c test_shims.c test_shims_proof.c ui.mm
test_sources += $(IMGUI_DIR)/backends/imgui_impl_sdl3.cpp $(IMGUI_DIR)/backends/imgui_impl_metal.mm
test_sources += $(IMGUI_DIR)/imgui.cpp $(IMGUI_DIR)/imgui_demo.cpp $(IMGUI_DIR)/imgui_draw.cpp $(IMGUI_DIR)/imgui_tables.cpp $(IMGUI_DIR)/imgui_widgets.cpp
other_files := $(main_c) $(test_sources)
common_sources := $(filter-out $(other_files), $(wildcard *.c))
header_files := $(wildcard *.h)

SOURCES := $(common_sources) $(main_c)

CC  := clang
CXX := clang++

CFLAGS := -g -Werror -Wall -Wextra -Wpadded

CXXFLAGS += `pkg-config --cflags sdl3`
CXXFLAGS += -I/usr/local/include -I/opt/local/include
CXXFLAGS += -std=c++11 -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends
CXXFLAGS += -Wall -Wformat

OUT := build

_dummy := $(shell mkdir -p $(OUT))

clean_build_logs:
	rm -f build.log errors.err

OBJS := $(addsuffix .o, $(basename $(notdir $(SOURCES))))
OBJS := $(addprefix $(OUT)/, $(OBJS))

CMOCKA_DIR := /opt/homebrew/Cellar/cmocka/1.1.7/include
$(OUT)/%.o:%.c
	$(CC) $(CFLAGS) -c -o $@ $<

soup: $(OBJS)
	$(CC) -o soup $^ $(CFLAGS)

soup_args := ops.bin cb_ops.bin DMG_ROM.bin

run: soup
	./soup $(soup_args)

print: soup
	./soup -p $(soup_args)

test_source_files := $(common_sources) $(test_sources)

SOURCES := $(common_sources) $(test_sources)
OBJS := $(addsuffix .o, $(basename $(notdir $(SOURCES))))
OBJS := $(addprefix $(OUT)/, $(OBJS))

LIBS := -framework Metal -framework MetalKit -framework Cocoa -framework IOKit -framework CoreVideo -framework QuartzCore
LIBS += `pkg-config --libs sdl3`
LIBS += -L/usr/local/lib -l cmocka

$(OUT)/%.o:$(IMGUI_DIR)/backends/%.mm
	$(CXX) $(CXXFLAGS) -ObjC++ -fobjc-weak -fobjc-arc -c -o $@ $<

$(OUT)/%.o:$(IMGUI_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OUT)/%.o:$(IMGUI_DIR)/backends/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OUT)/test.o: test.c
	$(CC) -g -Werror -Wall -Wextra -c -o $@ -I$(CMOCKA_DIR) $<

$(OUT)/test_shims.o: test_shims.c
	$(CC) -g -Werror -Wall -Wextra -c -o $@ -I$(CMOCKA_DIR) $<

$(OUT)/ui.o: ui.mm
	$(CXX) $(CXXFLAGS) -ObjC++ -fobjc-weak -fobjc-arc -c -o $@ $<

test_soup: $(OBJS)
	$(CXX) -o test_soup $^ $(CXXFLAGS) $(LIBS)

test: test_soup
	./test_soup $(soup_args)

test2: test_soup
	./test.py ./test_soup $(soup_args)

all: soup test_soup

clean:
	rm -f soup test_soup
	rm -rf $(OUT)/

clean_all: clean clean_build_logs

