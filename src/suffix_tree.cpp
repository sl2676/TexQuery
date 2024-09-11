#include <vector>
#include <string>
#include <functional>
#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>
#include <unordered_set>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include <chrono>

// Insert Time: 0(M) where M is the length of the string
// Search Time: 0(M) where M is the length of the string
/* Space: 0(ALPHABET_SIZE * M * N) where N is the number of 
	keys in trie, ALPHABET_SIZE is 26 if we are only considering upper case latin
	characters
*/
// Deletion time: 0(M)



struct Node {
    std::string substring;
    std::vector<int> child;
    Node() {}
    Node(const std::string& substring, std::initializer_list<int> children) : substring(substring) {
        child.insert(child.end(), children);
    }
};

struct SuffixTree {
    size_t num_nodes;
    std::vector<Node> nodes;
    std::unordered_set<std::string> latexTagsSet;
    std::vector<std::pair<std::string, std::string>> chunks;  

    SuffixTree(const std::string& string) {
        nodes.push_back(Node{});
        for (size_t i = 0; i < string.length(); i++) {
            insertSuffix(string.substr(i));
        }
        num_nodes = nodes.size();
    }

    void insertSuffix(const std::string& suffix) {
        int n = 0;
        size_t i = 0;
        while (i < suffix.length()) {
            char current_char = suffix[i];
            bool found = false;
            int next_node = -1;

            for (int child : nodes[n].child) {
                if (nodes[child].substring[0] == current_char) {
                    next_node = child;
                    found = true;
                    break;
                }
            }

            if (!found) {
                next_node = nodes.size();
                nodes.push_back(Node(suffix.substr(i), {}));
                nodes[n].child.push_back(next_node);
                return;
            }

            std::string node_substring = nodes[next_node].substring;
            size_t j = 0;
            while (j < node_substring.length() && i + j < suffix.length() && node_substring[j] == suffix[i + j]) {
                j++;
            }

            if (j < node_substring.length()) {
                int split_node = nodes.size();
                nodes.push_back(Node(node_substring.substr(0, j), {next_node}));
                nodes[next_node].substring = node_substring.substr(j);
                nodes[n].child.push_back(split_node);
                nodes[n].child.erase(std::remove(nodes[n].child.begin(), nodes[n].child.end(), next_node), nodes[n].child.end());
                return;
            }
            i += j;
            n = next_node;
        }
    }

    void searchLatexTags() {
        std::regex latexTagPattern(R"(\\[a-zA-Z]+\{[^}]*\})");
        std::function<void(int)> search;
        search = [&](int n) {
            std::smatch match;
            std::string text = nodes[n].substring;
            if (std::regex_search(text, match, latexTagPattern)) {
                latexTagsSet.insert(match.str());
            }
            for (int child : nodes[n].child) {
                search(child);
            }
        };
        search(0);
    }

    void chunkLatexContent(const std::string& content) {
        std::regex latexTagPattern(R"(\\[a-zA-Z]+\{[^}]*\}|\\begin\{[^}]*\}|\\end\{[^}]*\})");
        std::smatch match;
        std::string remaining_content = content;
        std::string current_chunk = "";

        while (std::regex_search(remaining_content, match, latexTagPattern)) {
            std::string tag = match.str();
            size_t tag_position = match.position();
            
            current_chunk += remaining_content.substr(0, tag_position);
            chunks.emplace_back(tag, current_chunk);  
            current_chunk = "";  
            
            remaining_content = remaining_content.substr(tag_position + tag.length());  
        }
        
        if (!remaining_content.empty()) {
            chunks.emplace_back("Remaining", remaining_content);
        }
    }

    void printChunks() const {
        std::cout << "\nChunks Found:\n";
        for (const auto& chunk : chunks) {
            std::cout << "Tag: " << chunk.first << "\nContent: " << chunk.second << "\n";
        }
    }

    void printLatexTagSet() const {
        std::cout << "Unique LaTeX Tags Found:\n";
        for (const auto& tag : latexTagsSet) {
            std::cout << tag << "\n";
        }
    }

    void createFSM() const {
        std::cout << "\nCreating FSM from LaTeX tags:\n";
        for (const auto& tag : latexTagsSet) {
            std::cout << "State for tag: " << tag << "\n";
        }
    }
	
	void exportChunksToFile(const std::string& filename) const {
		nlohmann::json json_output;
		for (const auto& chunk : chunks) {
			json_output.push_back({
				{"tag", chunk.first},
				{"content", chunk.second}
			});
		}
		std::ofstream output_file(filename);
		output_file << json_output.dump(4);  
	}


};

std::string contents_of(const std::string& path_to_file) {
    std::ifstream file(path_to_file);
    return { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>{} };
}

int main() {
    std::ifstream file("main_ex.tex");
    std::ostringstream oss;
    oss << file.rdbuf();
    std::string file_contents = oss.str();

	auto start_time = std::chrono::high_resolution_clock::now();

    SuffixTree suffixTree(file_contents);
    suffixTree.searchLatexTags();
    suffixTree.printLatexTagSet();
    suffixTree.chunkLatexContent(file_contents);
    suffixTree.printChunks();
    suffixTree.createFSM();
	suffixTree.exportChunksToFile("latex_chunks.json");
	
	auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

    std::cout << "Time taken: " << duration << " milliseconds" << std::endl;	

    return 0;
}

