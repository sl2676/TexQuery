#include <vector>
#include <string>
#include <functional>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <chrono>
#include <cctype>
#include <cstring>


// Insert Time: 0(M) where M is the length of the string
// Search Time: 0(M) where M is the length of the string
/* Space: 0(ALPHABET_SIZE * M * N) where N is the number of 
	keys in trie, ALPHABET_SIZE is 26 if we are only considering upper case latin
	characters
*/
// Deletion time: 0(M)



template <typename S>
void using_index(const std::vector<S>& vector, std::string sep = " ") {

	for (int i = 0; i < vector.size(); i++) {
		std::cout << vector[i] << sep;
	}
	std::cout << std::endl;
}

struct Node {
	std::string substring = "";
	std::vector<int> child;
	Node() {}
	Node(const std::string& substring, std::initializer_list<int> children) : substring(substring) {
		child.insert(child.end(), children);
	}
};

struct SuffixTree {
public:
	size_t num_nodes;
	std::vector<Node> nodes;
	SuffixTree(const std::string& string) {
		nodes.push_back(Node{});
		for (size_t i = 0; i < string.length(); i++) {
			insertSuffix(string.substr(i));
		}
		num_nodes = nodes.size();
	}
	
	void visualizeLatex() {
		if (nodes.size() == 0) {
			std::cout << "\n";
			return;
		}
		std::function<void(int, const std::string&)> f;
		f = [&](int n, const std::string& prefix) {
			auto children = nodes[n].child;
			if (children.size() == 0) {
				std::cout << "- " << nodes[n].substring << '\n';
				return;
			}
			std::cout << "^ " << nodes[n].substring << " " << children.size() << '\n';
			if (nodes[n].substring == "\\") {
				std::cout << " this is root node " << std::endl;
				
			}
			auto iter = std::begin(children);
			if (iter != std::end(children)) do {
				if (std::next(iter) == std::end(children)) break;
				std::cout << prefix << ">-";
				f(*iter, prefix + "| ");
				iter = std::next(iter);
			} while (true);
			std::cout << prefix << ">-";
			f(children[children.size() -1], prefix + "  ");
		};
		f(0, "");
	}
	void searchLatex(const std::string& parentString, const char childLastChar) {
		if (nodes.empty()) {
			std::cout << "\n";
			return;
		}

		std::function<void(int, const std::string&, bool, bool)> f;
		f = [&](int n, const std::string& prefix, bool foundParent, bool foundChildLastChar) {
			if (nodes[n].substring == parentString) {
				foundParent = true;
//				std::cout << prefix << ">-";
//				std::cout << "parent node " << nodes[n].substring <<'\n'; 
			}
				
			if (foundParent) {
				std::cout << prefix << "| >-";
				std::cout << nodes[n].substring << " " << nodes[n].child.size() << '\n'; 
			}

			if (nodes[n].substring.back() == childLastChar) {
				foundChildLastChar = true;
			}
			for (int child : nodes[n].child) {
				f(child, prefix + "| ", foundParent, foundChildLastChar);
			}
		};
		f(0, "", false, false);
	}

	// search latex with less contingency 	
	void sLatex(const std::string& parentString, const char childLastChar) {
		if (nodes.empty()) {
			std::cout << "\n";
			return;
		}
		std::function<void(int, const std::string&, bool, bool)> f;
		f = [&](int n, const std::string& prefix, bool foundParent, bool foundLastChar) {
			if (nodes[n].substring == parentString) {
				std::cout << "^ " << nodes[n].substring << " " << nodes[n].child.size() << "\n";
				foundParent = true;
			}
			if (foundParent) {
				for (int child : nodes[n].child) {
					std::cout << prefix << ">-";
					std::cout << nodes[child].substring << "\n";
					if (!nodes[child].child.empty() && nodes[child].substring.back() == childLastChar) {
						foundLastChar = true;
					}
					if (foundLastChar) {
						return;
					}
				}
			}
			for (int child : nodes[n].child) {
				f(child, prefix + "| ", foundParent, foundLastChar);
			}
		};
		f(0, "", false, false);
	}

	void rLatex (const std::string& parentString, const char childLastChar) {
		if (nodes.empty()) {
			std::cout << "\n";
			return;
		}
		int count = 0;
		std::function<void(int, const std::string&, bool, bool)> f;
		f = [&](int n, const std::string& prefix, bool foundParent, bool foundChildLastChar) {
			if (nodes[n].substring == parentString) {
				foundParent = true;
			}
			if (foundParent) {
				if (nodes[n].substring.find("\n") != std::string::npos) {
					if (nodes[n].substring.back() == childLastChar) {
						foundChildLastChar = true;
						return;
					}
				} else {
					std::cout << prefix << "| >-" << nodes[n].substring << "\n";
					count++;
//					std::cout << count << "\n";
				}
			} 
			for (int child : nodes[n].child) {
				f(child, prefix + "| ", foundParent, foundChildLastChar);
			}
		};
		f(0, "", false, false);
	}
	
	
	// search latex with more contingency
	void bLatex (const std::string& parentString, const char childLastChar) {
		if (nodes.empty()) {
			std::cout << "\n";
			return;
		}
		std::function<void(int, const std::string&, bool, bool)> f;
		f = [&](int n, const std::string& prefix, bool foundParent, bool foundChildLastChar) {
			if (nodes[n].substring == parentString) {
				foundParent = true;
			}	
			if (foundParent) {
				if (nodes[n].substring.find("\n") != std::string::npos) {
					if (nodes[n].substring.back() == childLastChar) {
						foundChildLastChar = true;
						//std::cout << prefix << "| >-" << childLastChar << '\n';
						return;
					}
				} else {
					std::cout << prefix << "| >-" << nodes[n].substring << '\n';
				}
			}
		for (int child : nodes[n].child) {
			f(child, prefix + "| ", foundParent, foundChildLastChar);
		}
		};
		f(0, "", false, false);

	}	
		
	
	void insertExternalSuffix(const std::string& suffix) {
		for (size_t i = 0; i < suffix.length(); i++) {
			insertSuffix(suffix.substr(i));
		}
	}
private:

	void insertSuffix(const std::string& suffix) {
		int n = 0;
		size_t i = 0;
		while (i < suffix.length()) {
			char b = suffix[i];
			int x2 = 0;
			int n2;
			while (true) {
				auto children = nodes[n].child;
				if (x2 == children.size()) {
					n2 = nodes.size();
					nodes.push_back(Node(suffix.substr(i), {}));
					nodes[n].child.push_back(n2);
					return;
				}
				n2 = children[x2];
				if (nodes[n2].substring[0] == b) {
					break;
				}
				x2++;
			}
			auto sub2 = nodes[n2].substring;
			size_t j = 0;
			while (j < sub2.size()) {
				if (suffix[i + j] != sub2[j]) {
					auto n3 = n2;
					n2 = nodes.size();
					nodes.push_back(Node(sub2.substr(0, j), {n3}));
					nodes[n3].substring = sub2.substr(j);
					nodes[n].child[x2] = n2;
					break;
				}
				j++;
			}
			i += j;
			n = n2;
		}
	}
};
std::string contents_of(std::string path_to_file) {
	std::ifstream file(path_to_file);
	return { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>{}};
}
constexpr uint32_t hash(const char* str) noexcept {
	uint32_t hash = 5381;
	int c = 0;
	while ((c = *str++)) {
		hash = ((hash << 5) + hash) + c;
	}
	return hash;
}

constexpr uint32_t operator"" _(const char* p, size_t) {
	return hash(p);
}

int main() {
	auto start = std::chrono::high_resolution_clock::now();
	/*
	std::ifstream inputFile("main.tex");
	if (inputFile) {
		std::ostringstream ss;
		ss << inputFile.rdbuf();
		std::string str = ss.str();
		std::cout << str << std::endl;
		
	//	SuffixTree(str).visualize();
	//	std::cout << sizeof(SuffixTree(str)) << std::endl;
	//	std::cout << SuffixTree(str).num_nodes;
	}
	
	constexpr uint32_t hash(const std::string& str) noexcept {
		uint32_t hash = 5381;
		for (const auto& c: str)
			hash = ((hash << 5) + hash) + (unsigned char)c;
		return hash;
	}
	constexpr inline uint32_t operator"" _(char const* p, size_t) {
		return hash(p);
	}
	
	constexpr auto b = "\\b"_;
	constexpr auto a = "\\a"_;
	constexpr auto f = "\\f"_;
	constexpr auto n = "\\n"_;
	constexpr auto r = "\\r"_;
	constexpr auto t = "\\t"_;
	constexpr auto v = "\\v"_;
	constexpr auto bs = "\\"_;
	constexpr auto bquo = "\\\""_;
	constexpr auto bque = "\\\?"_;
	constexpr auto bsq = "\\\'"_;
	constexpr auto u = "\\u"_;
	constexpr auto db = "\\\\"_;
	*/
	std::ifstream file("two_towers.txt");

	std::ostringstream oss;
	oss << file.rdbuf();
	std::string const file_contents = oss.str();
	
	std::istringstream file_data(file_contents);
	std::string line;
	std::string test_file;
	auto ronan_test = SuffixTree("");
	//uint32_t line_hash = hash(line.substr(0, 2).c_str());
		
	
	while (getline(file_data, line)) {
		ronan_test.insertExternalSuffix(line);
	}
//	std::cout << "test_file length: " << test_file.length() << std::endl;
//	std::cout << "ronan_test size: " << sizeof(ronan_test) << std::endl;
//	std::cout << "ronan num nodes: " << ronan_test.num_nodes << std::endl;
//	ronan_test.rLatex("\\", '}');
	ronan_test.visualizeLatex();
//	ronan_test.rLatex("\\", '}');
	auto stop = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
	std::cout << "Time taken by functions: " << duration.count() << "microseconds" << std::endl;
	std::cout << "Time taken by functions: " << duration.count() / 1000000 << "seconds" << std::endl;
//	std::cout << "ronan num nodes: " << ronan_test.num_nodes << std::endl;
	return 0;

}
