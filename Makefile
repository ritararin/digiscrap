CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall
CPPFLAGS += -Isrc -Iextern/glad/include -D_USE_MATH_DEFINES

MODULE_SRC = src/lines/lines.cpp src/polygons/polygons.cpp \
             src/clipping/clipping.cpp src/circles/circles.cpp \
             src/curves/curves.cpp src/colour/colour.cpp \
             src/easing/easing.cpp src/paths/paths.cpp \
             src/transitions/transitions.cpp \
             src/textures/image_loader.cpp src/textures/font_atlas.cpp \
             src/textures/filter_mirror.cpp \
             src/calendar/calendar.cpp src/calendar/day_storage.cpp \
             src/interaction/edit_history.cpp src/interaction/layer_ops.cpp \
             src/interaction/snapping.cpp

ANIM_SRC = src/animation/exporter.cpp src/animation/demo_sequences.cpp
STICKER_SRC = src/stickers/demo_drag.cpp
GL_SRC = extern/glad/src/glad.c src/opengl/window.cpp src/opengl/shader.cpp \
         src/opengl/renderer.cpp src/opengl/texture_renderer.cpp \
         src/calendar/calendar_gl.cpp

HEADERS = $(wildcard src/*/*.h) $(wildcard src/opengl/*.h)

ifeq ($(OS),Windows_NT)
  GL_LIBS = -lglfw3 -lopengl32 -lgdi32
  MKDIR_OUTPUT = if not exist output mkdir output
  RUN_SCRAPBOOK = scrapbook.exe
  RUN_POLYGONS = polygons_test.exe
  RUN_CLIPPING = clipping_colour_test.exe
  RUN_LINES_CIRCLES = lines_circles_test.exe
  RUN_ANIM = animation_test.exe
  RUN_STICKER = sticker_test.exe
  RUN_EASING = easing_paths_test.exe
  RUN_OPENGL = opengl_test.exe
  RUN_CALENDAR = calendar_test.exe
  RUN_INTERACTION = interaction_test.exe
  RUN_INTERACTION_APP = interaction_app_test.exe
else
  UNAME_S := $(shell uname -s)
  ifeq ($(UNAME_S),Darwin)
    GL_LIBS = -lglfw -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
  else
    GL_LIBS = -lglfw -lGL -lX11 -lpthread -lXrandr -lXi -ldl
  endif
  MKDIR_OUTPUT = mkdir -p output
  RUN_SCRAPBOOK = ./scrapbook
  RUN_POLYGONS = ./polygons_test
  RUN_CLIPPING = ./clipping_colour_test
  RUN_LINES_CIRCLES = ./lines_circles_test
  RUN_ANIM = ./animation_test
  RUN_STICKER = ./sticker_test
  RUN_EASING = ./easing_paths_test
  RUN_OPENGL = ./opengl_test
  RUN_CALENDAR = ./calendar_test
  RUN_INTERACTION = ./interaction_test
  RUN_INTERACTION_APP = ./interaction_app_test
endif

.PHONY: all run test clean

all: scrapbook polygons_test clipping_colour_test lines_circles_test animation_test sticker_test easing_paths_test opengl_test calendar_test interaction_test interaction_app_test

scrapbook: src/main.cpp $(MODULE_SRC) $(ANIM_SRC) $(STICKER_SRC) $(GL_SRC) $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) src/main.cpp $(MODULE_SRC) $(ANIM_SRC) $(STICKER_SRC) $(GL_SRC) $(GL_LIBS) -o $@

polygons_test: src/polygons/test_polygons.cpp $(MODULE_SRC) $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) src/polygons/test_polygons.cpp $(MODULE_SRC) -o $@

clipping_colour_test: src/clipping/test_clipping_colour.cpp $(MODULE_SRC) $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) src/clipping/test_clipping_colour.cpp $(MODULE_SRC) -o $@

lines_circles_test: src/circles/test_lines_circles.cpp $(MODULE_SRC) $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) src/circles/test_lines_circles.cpp $(MODULE_SRC) -o $@

animation_test: src/animation/test_animation_core.cpp src/animation/exporter.cpp $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) src/animation/test_animation_core.cpp src/animation/exporter.cpp -o $@

sticker_test: src/stickers/test_stickers.cpp src/polygons/polygons.cpp src/lines/lines.cpp $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) src/stickers/test_stickers.cpp src/polygons/polygons.cpp src/lines/lines.cpp -o $@

easing_paths_test: src/easing/test_easing_paths.cpp $(MODULE_SRC) $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) src/easing/test_easing_paths.cpp $(MODULE_SRC) -o $@

opengl_test: src/opengl/test_opengl.cpp $(GL_SRC) $(MODULE_SRC) $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) src/opengl/test_opengl.cpp $(GL_SRC) $(MODULE_SRC) $(GL_LIBS) -o $@

calendar_test: src/calendar/test_calendar.cpp src/animation/exporter.cpp $(GL_SRC) $(MODULE_SRC) $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) src/calendar/test_calendar.cpp src/animation/exporter.cpp $(GL_SRC) $(MODULE_SRC) $(GL_LIBS) -o $@

interaction_test: src/interaction/test_interaction.cpp $(MODULE_SRC) $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) src/interaction/test_interaction.cpp $(MODULE_SRC) -o $@

interaction_app_test: src/interaction/test_interaction_app.cpp src/animation/exporter.cpp $(GL_SRC) $(MODULE_SRC) $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) src/interaction/test_interaction_app.cpp src/animation/exporter.cpp $(GL_SRC) $(MODULE_SRC) $(GL_LIBS) -o $@

run: scrapbook
	$(MKDIR_OUTPUT)
	$(RUN_SCRAPBOOK)

test: polygons_test clipping_colour_test lines_circles_test animation_test sticker_test easing_paths_test opengl_test calendar_test interaction_test interaction_app_test
	$(MKDIR_OUTPUT)
	$(RUN_POLYGONS)
	$(RUN_CLIPPING)
	$(RUN_LINES_CIRCLES)
	$(RUN_ANIM)
	$(RUN_STICKER)
	$(RUN_EASING)
	$(RUN_OPENGL)
	$(RUN_CALENDAR)
	$(RUN_INTERACTION)
	$(RUN_INTERACTION_APP)

clean:
ifeq ($(OS),Windows_NT)
	-cmd /c del /f /q scrapbook.exe polygons_test.exe clipping_colour_test.exe lines_circles_test.exe animation_test.exe sticker_test.exe easing_paths_test.exe opengl_test.exe calendar_test.exe interaction_test.exe interaction_app_test.exe 2>nul
else
	rm -f scrapbook polygons_test clipping_colour_test lines_circles_test animation_test sticker_test easing_paths_test opengl_test calendar_test interaction_test interaction_app_test \
	      scrapbook.exe polygons_test.exe clipping_colour_test.exe lines_circles_test.exe animation_test.exe sticker_test.exe easing_paths_test.exe opengl_test.exe calendar_test.exe interaction_test.exe interaction_app_test.exe
endif
