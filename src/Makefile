# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic -g -fsanitize=address -arch arm64

# Include directories
INCLUDES = -I. \
           -I/opt/homebrew/opt/nlohmann-json/include \
           -I/opt/homebrew/opt/icu4c/include \
           -I/opt/homebrew/opt/uchardet/include

# Source files
SRCS = main.cpp lexer.cpp parser.cpp ast.cpp dag_node.cpp fsm.cpp symbol_table.cpp

# Object files
OBJS = ${SRCS:.cpp=.o}

# Executable name
EXEC = parser

# Library paths and linking
# Using system's default libiconv and linking with uchardet
LDFLAGS = -L/opt/homebrew/opt/icu4c/lib \
          -L/opt/homebrew/opt/uchardet/lib \
          -liconv -luchardet

# Build target
all: $(EXEC)

$(EXEC): $(OBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LDFLAGS) -o $@ $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Clean up
clean:
	rm -f $(OBJS) $(EXEC)

.PHONY: all clean

