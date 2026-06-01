# main program

OUT    := build
_dummy := $(shell mkdir -p $(OUT))

MAIN_C  := src/main.c
OTHER   := $(MAIN_C)
COMMON  := $(filter-out $(OTHER), $(wildcard src/*.c))
SOURCES := $(COMMON) $(MAIN_C)

XCHAIN := ../xchain/third-party
ZIG := $(XCHAIN)/zig/zig

CC := $(ZIG) cc

CFLAGS := -g -Werror -Wall -Wextra -Wpadded

OBJS := $(addsuffix .o, $(basename $(notdir $(SOURCES))))
OBJS := $(addprefix $(OUT)/, $(OBJS))

$(OUT)/%.o:src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

soup: $(OBJS)
	$(CC) -o soup $^ $(CFLAGS)

SOUP_ARGS := ops.bin cb_ops.bin DMG_ROM.bin
SOUP_ARGS := $(addprefix data/, $(SOUP_ARGS))

run: soup
	./soup $(SOUP_ARGS)

print: soup
	./soup -p $(SOUP_ARGS)

# test program

IMGUI_DIR := ../imgui

TEST_SOURCES := $(wildcard test/*.c)
TEST_SOURCES += ui/ui_loop.mm ui/ui.cpp ui/tile.cpp
TEST_SOURCES += $(IMGUI_DIR)/backends/imgui_impl_sdl3.cpp $(IMGUI_DIR)/backends/imgui_impl_metal.mm
TEST_SOURCES += $(IMGUI_DIR)/imgui.cpp $(IMGUI_DIR)/imgui_demo.cpp $(IMGUI_DIR)/imgui_draw.cpp $(IMGUI_DIR)/imgui_tables.cpp $(IMGUI_DIR)/imgui_widgets.cpp

SOURCES := $(COMMON) $(TEST_SOURCES)

CXX := $(ZIG) c++

CXXFLAGS := `pkg-config --cflags sdl3`
CXXFLAGS += -I/usr/local/include -I/opt/local/include
CXXFLAGS += -std=c++11 -Isrc -Itest -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends
CXXFLAGS += -Wall -Wformat

OBJS := $(addsuffix .o, $(basename $(notdir $(SOURCES))))
OBJS := $(addprefix $(OUT)/, $(OBJS))

LIBS := -framework Metal -framework MetalKit -framework Cocoa -framework IOKit -framework CoreVideo -framework QuartzCore
LIBS += `pkg-config --libs sdl3`
LIBS += -L$(XCHAIN)/cmocka/build/src -l cmocka

CMOCKA_DIR := $(XCHAIN)/cmocka/include

$(OUT)/%.o:test/%.c
	$(CC) $(CFLAGS) -c -Isrc/ -I./ -o $@ $<

$(OUT)/%.o:$(IMGUI_DIR)/backends/%.mm
	$(CXX) $(CXXFLAGS) -ObjC++ -fobjc-weak -fobjc-arc -c -o $@ $<

$(OUT)/%.o:$(IMGUI_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OUT)/%.o:$(IMGUI_DIR)/backends/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OUT)/test.o: test/test.c
	$(CC) -g -Werror -Wall -Wextra -c -o $@ -Isrc/ -I$(CMOCKA_DIR) $<

$(OUT)/test_shims.o: test/test_shims.c
	$(CC) -g -Werror -Wall -Wextra -c -o $@ -Isrc/ -I$(CMOCKA_DIR) $<

$(OUT)/ui_loop.o: ui/ui_loop.mm
	$(CXX) $(CXXFLAGS) -ObjC++ -fobjc-weak -fobjc-arc -c -o $@ $<

$(OUT)/ui.o: ui/ui.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OUT)/tile.o: ui/tile.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

test_soup: $(OBJS)
	$(CXX) -o test_soup $^ $(CXXFLAGS) $(LIBS)

test: test_soup
	./test_soup $(SOUP_ARGS)

test2: test_soup
	./tools/test.py ./test_soup $(SOUP_ARGS)

# all

all: soup test_soup

clean:
	rm -f soup test_soup
	rm -rf $(OUT)/ *.dSYM

