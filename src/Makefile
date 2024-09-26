CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic -g -fsanitize=address

INCLUDES = -I. -I/opt/homebrew/opt/nlohmann-json/include

SRCS = main.cpp lexer.cpp parser.cpp ast.cpp  dag_node.cpp fsm.cpp symbol_table.cpp

OBJS = ${SRCS:.cpp=.o}

EXEC = parser

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJS) $(EXEC)

.PHONY: all clean
