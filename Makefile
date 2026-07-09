#
# main program
#

OUT    := build
_dummy := $(shell mkdir -p $(OUT))

OTHER   := src/main.c src/ui_loop.c
COMMON  := $(filter-out $(OTHER), $(wildcard src/*.c))
SOURCES := $(COMMON) $(OTHER)

XCHAIN := ../xchain/third-party
ZIG := $(XCHAIN)/zig/zig

CC := $(ZIG) cc

CFLAGS := -std=c23 -g -Werror -Wall -Wextra -Wpadded `pkg-config --cflags sdl3`
LIBS := `pkg-config --libs sdl3`

OBJS := $(addsuffix .o, $(basename $(notdir $(SOURCES))))
OBJS := $(addprefix $(OUT)/, $(OBJS))

$(OUT)/%.o:src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

soup: $(OBJS)
	$(CC) -o soup $^ $(CFLAGS) $(LIBS)

SOUP_ARGS := ops.bin cb_ops.bin DMG_ROM.bin
SOUP_ARGS := $(addprefix data/, $(SOUP_ARGS))

run: soup
	./soup $(SOUP_ARGS)

print: soup
	./soup -p $(SOUP_ARGS)

#
# test program
#

IMGUI_DIR := ../imgui

TEST_SOURCES := $(wildcard test/*.c)
TEST_SOURCES += ui/ui.cpp ui/tile.cpp test/ui_loop_dbg.cpp
TEST_SOURCES += $(IMGUI_DIR)/backends/imgui_impl_sdl3.cpp $(IMGUI_DIR)/backends/imgui_impl_sdlrenderer3.cpp
TEST_SOURCES += $(IMGUI_DIR)/imgui.cpp $(IMGUI_DIR)/imgui_demo.cpp $(IMGUI_DIR)/imgui_draw.cpp $(IMGUI_DIR)/imgui_tables.cpp $(IMGUI_DIR)/imgui_widgets.cpp

SOURCES := $(COMMON) $(TEST_SOURCES)

CXX := $(ZIG) c++

CXXFLAGS := `pkg-config --cflags sdl3`
CXXFLAGS += -I/usr/local/include -I/opt/local/include
CXXFLAGS += -std=c++11 -Isrc -Itest -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends
CXXFLAGS += -Wall -Wformat

OBJS := $(addsuffix .o, $(basename $(notdir $(SOURCES))))
OBJS := $(addprefix $(OUT)/, $(OBJS))

LIBS := `pkg-config --libs sdl3`
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

$(OUT)/%.o:test/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OUT)/test.o: test/test.c
	$(CC) -g -Werror -Wall -Wextra -c -o $@ -Isrc/ -I$(CMOCKA_DIR) $<

$(OUT)/test_shims.o: test/test_shims.c
	$(CC) -g -Werror -Wall -Wextra -c -o $@ -Isrc/ -I$(CMOCKA_DIR) $<

$(OUT)/ui.o: ui/ui.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OUT)/tile.o: ui/tile.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

test_soup: $(OBJS)
	$(CXX) -o test_soup $^ $(CXXFLAGS) $(LIBS)

test: test_soup
	./test_soup $(SOUP_ARGS)
#
# all
#

all: soup test_soup

clean:
	rm -f soup test_soup
	rm -rf $(OUT)/ *.dSYM

