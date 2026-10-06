CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall
CPPFLAGS += -Isrc -D_USE_MATH_DEFINES
MODULE_SRC = src/lines/lines.cpp src/polygons/polygons.cpp \
             src/clipping/clipping.cpp src/circles/circles.cpp \
             src/curves/curves.cpp src/colour/colour.cpp
HEADERS = $(wildcard src/*/*.h)

.PHONY: all run test clean
all: scrapbook polygons_test clipping_colour_test lines_circles_test

scrapbook: src/main.cpp $(MODULE_SRC) $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) src/main.cpp $(MODULE_SRC) -o $@

polygons_test: src/polygons/test_polygons.cpp $(MODULE_SRC) $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) src/polygons/test_polygons.cpp $(MODULE_SRC) -o $@

clipping_colour_test: src/clipping/test_clipping_colour.cpp $(MODULE_SRC) $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) src/clipping/test_clipping_colour.cpp $(MODULE_SRC) -o $@

lines_circles_test: src/circles/test_lines_circles.cpp $(MODULE_SRC) $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) src/circles/test_lines_circles.cpp $(MODULE_SRC) -o $@

ifeq ($(OS),Windows_NT)
  MKDIR_OUTPUT = if not exist output mkdir output
  RUN_SCRAPBOOK = scrapbook.exe
  RUN_POLYGONS = polygons_test.exe
  RUN_CLIPPING = clipping_colour_test.exe
  RUN_LINES_CIRCLES = lines_circles_test.exe
else
  MKDIR_OUTPUT = mkdir -p output
  RUN_SCRAPBOOK = ./scrapbook
  RUN_POLYGONS = ./polygons_test
  RUN_CLIPPING = ./clipping_colour_test
  RUN_LINES_CIRCLES = ./lines_circles_test
endif

run: scrapbook
	$(MKDIR_OUTPUT)
	$(RUN_SCRAPBOOK)

test: polygons_test clipping_colour_test lines_circles_test
	$(MKDIR_OUTPUT)
	$(RUN_POLYGONS)
	$(RUN_CLIPPING)
	$(RUN_LINES_CIRCLES)

clean:
	rm -f scrapbook polygons_test clipping_colour_test lines_circles_test scrapbook.exe polygons_test.exe clipping_colour_test.exe lines_circles_test.exe