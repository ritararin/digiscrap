CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall
CPPFLAGS += -Isrc
MODULE_SRC = src/lines/lines.cpp src/polygons/polygons.cpp \
             src/clipping/clipping.cpp src/circles/circles.cpp \
             src/curves/curves.cpp src/colour/colour.cpp
HEADERS = $(wildcard src/*/*.h)

.PHONY: all run test clean
all: scrapbook polygons_test clipping_colour_test

scrapbook: src/main.cpp $(MODULE_SRC) $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) src/main.cpp $(MODULE_SRC) -o $@

polygons_test: src/polygons/test_polygons.cpp $(MODULE_SRC) $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) src/polygons/test_polygons.cpp $(MODULE_SRC) -o $@

clipping_colour_test: src/clipping/test_clipping_colour.cpp $(MODULE_SRC) $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) src/clipping/test_clipping_colour.cpp $(MODULE_SRC) -o $@

run: scrapbook
	mkdir -p output && ./scrapbook

test: polygons_test clipping_colour_test
	mkdir -p output
	./polygons_test
	./clipping_colour_test

clean:
	rm -f scrapbook polygons_test clipping_colour_test scrapbook.exe polygons_test.exe clipping_colour_test.exe