CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic -g -fsanitize=address -arch arm64

INCLUDES = -I./include \
           -I/opt/homebrew/opt/nlohmann-json/include \
           -I/opt/homebrew/opt/icu4c/include \
           -I/opt/homebrew/opt/uchardet/include \
           -I/opt/homebrew/Cellar/googletest/1.15.2/include

SRCS = lexer.cpp parser.cpp ast.cpp dag_node.cpp fsm.cpp symbol_table.cpp ner.cpp crf_model.cpp
TEST_SRCS = tests/test_fsm.cpp

OBJS = ${SRCS:.cpp=.o}
TEST_OBJS = ${TEST_SRCS:.cpp=.o}

EXEC = parser
TEST_EXEC = run_tests

LDFLAGS = -L/opt/homebrew/opt/icu4c/lib \
          -L/opt/homebrew/opt/uchardet/lib \
          -L/opt/homebrew/Cellar/googletest/1.15.2/lib \
          -lgtest -lgtest_main -pthread -liconv -luchardet

all: $(EXEC)

$(EXEC): $(OBJS) main.o
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LDFLAGS) -o $@ $^

tests: $(TEST_OBJS) $(OBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LDFLAGS) -o $(TEST_EXEC) $(TEST_OBJS) $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJS) main.o $(TEST_OBJS) $(EXEC) $(TEST_EXEC)

.PHONY: all clean tests

