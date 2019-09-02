.PHONY: all clean debug run

# include directories
INCLUDE = inc/ lib/termcolor/

# compiler options
INCLUDE_OPTS = $(foreach d, $(INCLUDE), -I$d)
CXX = g++
CXX_FLAGS = -std=c++11 -Wall -Werror $(INCLUDE_OPTS)

# output file and dependency list
OUT_FILE = main.out
OUT_DEPS = src/*.cpp inc/*.hpp


# make targets
all: $(OUT_FILE)

$(OUT_FILE): $(OUT_DEPS)
	@echo "Building '$(OUT_FILE)' ..."
	@$(CXX) $(CXX_FLAGS) -o $(OUT_FILE) $^

clean:
	@echo "Cleaning built files ..."
	rm -f *.o *.out

debug: $(OUT_DEPS)
	@echo "Building '$(OUT_FILE)' with debug info ..."
	@$(CXX) -g $(CXX_FLAGS) -o $(OUT_FILE) $^
	

run: $(OUT_FILE)
	@./$(OUT_FILE)
