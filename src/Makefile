CC = clang++
CFLAGS = -std=c++17 -I/opt/homebrew/opt/nlohmann-json/include

all: suffix_tree

suffix_tree: suffix_tree.cpp
	$(CC) $(CFLAGS) suffix_tree.cpp -o suffix_tree

clean:
	rm -rf suffix_tree

