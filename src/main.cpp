#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <filesystem>
#include <vector>
#include <string>
#include <map>
#include <regex>
#include <algorithm>
#include <iconv.h>
#include <uchardet/uchardet.h>
#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "fsm.h"
#include "nlohmann/json.hpp"

namespace fs = std::filesystem;

std::string make_safe_filename(const fs::path& path) {
    std::string filename = path.string();
    std::replace(filename.begin(), filename.end(), '/', '_');
    std::replace(filename.begin(), filename.end(), '\\', '_');
    return filename;
}

void collect_tex_files(const fs::path& directory, std::vector<fs::path>& tex_files) {
    for (const auto& entry : fs::recursive_directory_iterator(directory)) {
        if (entry.is_regular_file() && entry.path().extension() == ".tex") {
            tex_files.push_back(entry.path());
        }
    }
}

std::string detect_encoding_from_latex(const fs::path& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        return "UTF-8"; 
    }
    std::string line;
    size_t max_lines = 50; 
    size_t line_count = 0;
    while (std::getline(file, line) && line_count++ < max_lines) {
        std::regex inputenc_regex(R"(\\usepackage\[(.*?)\]\{inputenc\})");
        std::smatch match;
        if (std::regex_search(line, match, inputenc_regex)) {
            std::string encoding = match[1].str();
            encoding.erase(std::remove_if(encoding.begin(), encoding.end(), ::isspace), encoding.end());
            return encoding;
        }
        std::regex inputencoding_regex(R"(\\inputencoding\{(.*?)\})");
        if (std::regex_search(line, match, inputencoding_regex)) {
            std::string encoding = match[1].str();
            encoding.erase(std::remove_if(encoding.begin(), encoding.end(), ::isspace), encoding.end());
            return encoding;
        }
        std::regex cjk_regex(R"(\\begin\{CJK\*\}\{(.*?)\}\{.*\})");
        if (std::regex_search(line, match, cjk_regex)) {
            std::string encoding = match[1].str();
            encoding.erase(std::remove_if(encoding.begin(), encoding.end(), ::isspace), encoding.end());
            return encoding;
        }
    }
    return "UTF-8"; 
}

std::string detect_encoding(const std::string& content) {
    uchardet_t ud = uchardet_new();
    if (uchardet_handle_data(ud, content.c_str(), content.length()) != 0) {
        uchardet_delete(ud);
        return "UNKNOWN";
    }
    uchardet_data_end(ud);
    const char* charset_cstr = uchardet_get_charset(ud);
    std::string charset = charset_cstr ? charset_cstr : "UNKNOWN";
    uchardet_delete(ud);
    return charset;
}

std::string convert_encoding(const std::string& input, const std::string& from_encoding, const std::string& to_encoding) {
    iconv_t cd = iconv_open(to_encoding.c_str(), from_encoding.c_str());
    if (cd == (iconv_t)-1) {
        std::cerr << "Error: iconv_open failed for encoding conversion from " << from_encoding << " to " << to_encoding << "." << std::endl;
        return "";
    }

    size_t in_bytes_left = input.size();
    size_t out_bytes_left = in_bytes_left * 4; 
    const char* in_buf = input.c_str();
    std::string output;
    output.resize(out_bytes_left);
    char* out_buf = &output[0];

    while (in_bytes_left > 0) {
        size_t result = iconv(cd, const_cast<char**>(&in_buf), &in_bytes_left, &out_buf, &out_bytes_left);
        if (result == (size_t)-1) {
            if (errno == EILSEQ || errno == EINVAL) {
                std::cerr << "Warning: Invalid multibyte sequence encountered during conversion." << std::endl;
                ++in_buf;
                --in_bytes_left;
                continue;
            } else if (errno == E2BIG) {
                size_t out_buf_used = output.size() - out_bytes_left;
                output.resize(output.size() * 2);
                out_buf = &output[0] + out_buf_used;
                out_bytes_left = output.size() - out_buf_used;
                continue;
            } else {
                std::cerr << "Error: iconv conversion failed from " << from_encoding << " to " << to_encoding << "." << std::endl;
                iconv_close(cd);
                return "";
            }
        }
    }
    iconv_close(cd);

    size_t out_buf_used = output.size() - out_bytes_left;
    output.resize(out_buf_used); 
    return output;
}

bool is_valid_utf8(const std::string& string) {
    int c, i, ix, n, j;
    for (i = 0, ix = string.length(); i < ix; i++) {
        c = (unsigned char)string[i];
        if (0x00 <= c && c <= 0x7F) {
            continue; 
        } else if ((c & 0xE0) == 0xC0) {
            n = 1; 
        } else if ((c & 0xF0) == 0xE0) {
            n = 2; 
        } else if ((c & 0xF8) == 0xF0) {
            n = 3; 
        } else {
            return false; 
        }
        if (i + n >= ix) {
            return false; 
        }
        for (j = 0; j < n; j++) {
            i++;
            c = (unsigned char)string[i];
            if ((c & 0xC0) != 0x80) {
                return false; 
            }
        }
    }
    return true;
}

std::string read_file_as_utf8(const fs::path& file_path) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Could not open the file: " << file_path << "\n";
        return "";
    }
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();

    std::string encoding = detect_encoding_from_latex(file_path);
    std::cout << "Detected LaTeX encoding: " << encoding << "\n";

    if (encoding == "UTF-8") {
        std::string detected_encoding = detect_encoding(content);
        std::cout << "Detected encoding from content: " << detected_encoding << "\n";
        if (detected_encoding != "UNKNOWN") {
            encoding = detected_encoding;
        }
    }

    if (is_valid_utf8(content)) {
        return content;
    }

    std::string utf8_content = convert_encoding(content, encoding, "UTF-8");
    if (utf8_content.empty()) {
        std::cerr << "Failed to convert file to UTF-8: " << file_path << "\n";
        return "";
    }
    return utf8_content;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: ./parser <input_directory>\n";
        return 1;
    }

    fs::path inputPath(argv[1]);

    if (!fs::exists(inputPath) || !fs::is_directory(inputPath)) {
        std::cerr << "Invalid input directory: " << inputPath << "\n";
        return 1;
    }

    fs::path outputDir = "json_output";
    fs::create_directories(outputDir);

	std::vector<std::string> fs_paths;	

    for (const auto& arxiv_dir_entry : fs::directory_iterator(inputPath)) {
        if (arxiv_dir_entry.is_directory()) {
            fs::path arxiv_dir = arxiv_dir_entry.path();
			fs_paths.push_back(arxiv_dir_entry.path());
            std::cout << "Processing arXiv directory: " << arxiv_dir << "\n";

            std::vector<fs::path> tex_files;
            collect_tex_files(arxiv_dir, tex_files);

            if (tex_files.empty()) {
                std::cerr << "No .tex files found in directory: " << arxiv_dir << "\n";
                continue;
            }

            std::string combined_input;
            for (const auto& tex_file : tex_files) {
                std::cout << "Including file: " << tex_file << "\n";
                std::string content = read_file_as_utf8(tex_file);
                if (content.empty()) {
                    std::cerr << "Could not read the file: " << tex_file << "\n";
                    continue;
                }
                combined_input += content;
                combined_input += "\n"; 
            }

            if (combined_input.empty()) {
                std::cerr << "No valid content to parse for directory: " << arxiv_dir << "\n";
                continue;
            }

            Lexer lexer(combined_input);
            Parser parser(lexer);

            try {
                std::shared_ptr<AST> ast = parser.parseDocument();
                std::cout << "Printing AST structure for arXiv directory: " << arxiv_dir << "\n";
                ast->print();

                FSM fsm;
                nlohmann::json jsonDocument = fsm.chunkDocumentToJson(ast->root);

                fs::path relativeDir = fs::relative(arxiv_dir, inputPath);
                std::string jsonFileName = make_safe_filename(relativeDir) + ".json";
                fs::path jsonFilePath = outputDir / jsonFileName;

                std::ofstream jsonFile(jsonFilePath);
                if (jsonFile.is_open()) {
                    jsonFile << jsonDocument.dump(4);
                    jsonFile.close();
                    std::cout << "Document successfully written to " << jsonFilePath << "\n";
					fs_paths.push_back(jsonFilePath);
                } else {
                    std::cerr << "Error opening file for writing JSON output: " << jsonFilePath << "\n";
                }
            } catch (const std::exception& e) {
                std::cerr << "Error processing arXiv directory " << arxiv_dir << ": " << e.what() << "\n";
                continue;
            }
        }
    }
    for (const auto& path : fs_paths) std::cout << "PATH " << path << std::endl;
	return 0;
}

