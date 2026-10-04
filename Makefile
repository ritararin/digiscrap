CXX ?= g++
CXXFLAGS = -std=c++17 -O2 -Wall -Isrc
SRC = $(shell find src -name '*.cpp')

scrapbook: $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o scrapbook

run: scrapbook
	mkdir -p output && ./scrapbook

clean:
	rm -f scrapbook