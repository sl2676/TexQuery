#include "fsm.h"
using json = nlohmann::json;

FSM::FSM() : currentState(FSMState::Start) {}
void FSM::traverseAST(const std::shared_ptr<ASTNode>& node, std::string& currentChunk, std::vector<std::string>& chunks) {
    if (!node) return;

    switch (node->getType()) {
        case ASTNode::NodeType::Document:
            handleDocument(node, currentChunk);
            break;

        case ASTNode::NodeType::Section:
            handleSection(node, currentChunk, chunks);
            break;

        case ASTNode::NodeType::Command:
            handleCommand(node, currentChunk);
            break;

        case ASTNode::NodeType::Text:
            handleText(node, currentChunk);
            break;
        case ASTNode::NodeType::Environment:
            handleEnvironment(node, currentChunk);
            break;
        default:
            break;
    }

    for (const auto& child : node->getChildren()) {
        traverseAST(child, currentChunk, chunks);
    }

    if (auto dagNode = node->getDAGNode()) {
        for (const auto& childDagNode : dagNode->getChildren()) {
            if (auto linkedAstNode = childDagNode->astNode.lock()) {
                traverseAST(linkedAstNode, currentChunk, chunks);
            }
        }
    }
}

std::vector<std::string> FSM::chunkDocument(const std::shared_ptr<ASTNode>& root) {
    std::vector<std::string> chunks;
    std::string currentChunk;

    traverseAST(root, currentChunk, chunks);

    if (!currentChunk.empty()) {
        chunks.push_back(currentChunk);
    }

    return chunks;
}


json FSM::chunkDocumentToJson(const std::shared_ptr<ASTNode>& root) {
    json documentJson;
    documentJson["document"]["metadata"]["authors"] = json::array();
    documentJson["document"]["content"] = json::array();

    std::string currentSection;
    json currentSectionJson;
    json currentAuthor;
    bool authorActive = false;  
    std::string currentAffiliation;

    bool inAbstract = false;
    bool inEquation = false;
    bool inFigure = false;
    std::string currentEquationContent;

    std::function<void(const std::shared_ptr<ASTNode>&)> traverse;
    traverse = [&](const std::shared_ptr<ASTNode>& node) {
        std::string content = node->getContent();

        if (node->getType() == ASTNode::NodeType::Document) {
            documentJson["document"]["title"] = "Untitled Document"; 
        } else if (node->getType() == ASTNode::NodeType::Command) {

            if (content.find("\\title") != std::string::npos) {
                documentJson["document"]["title"] = extractSectionName(content);
            }
            else if (content.find("\\author") != std::string::npos) {
                std::vector<std::string> authors = extractMultipleAuthors(content);
                for (const auto& author : authors) {
                    currentAuthor["name"] = author;
                    currentAuthor["affiliations"] = json::array();
                    documentJson["document"]["metadata"]["authors"].push_back(currentAuthor);
                    currentAuthor.clear();
                    authorActive = true; 
                }
            }
            else if (content.find("\\affiliation") != std::string::npos) {
                processAffiliations(currentAuthor, content, documentJson, authorActive);
            }
            else if (content.find("\\begin{abstract}") != std::string::npos) {
                inAbstract = true;
                currentSectionJson["section"] = "Abstract";
                currentSectionJson["type"] = "text";
                currentSectionJson["content"] = "";
            } else if (content.find("\\end{abstract}") != std::string::npos) {
                inAbstract = false;
                documentJson["document"]["content"].push_back(currentSectionJson);
                currentSectionJson.clear();
            }
            else if (content.find("\\begin{equation}") != std::string::npos) {
                inEquation = true;
                currentSectionJson["type"] = "equation";
                currentEquationContent = "";  
            } else if (content.find("\\label") != std::string::npos && inEquation) {
                currentSectionJson["label"] = extractCitationLabel(content);
            } else if (inEquation && (content.find("\\end{equation}") != std::string::npos)) {
                inEquation = false;
                currentSectionJson["content"] = currentEquationContent; 
                documentJson["document"]["content"].push_back(currentSectionJson);
                currentSectionJson.clear();
            } else if (inEquation) {
                currentEquationContent += content + "\n";  
            }
            else if (content.find("\\begin{figure}") != std::string::npos) {
                inFigure = true;
                currentSectionJson["type"] = "figure";
                currentSectionJson["figure_caption"] = "";
            } else if (content.find("\\includegraphics") != std::string::npos) {
                currentSectionJson["image_file"] = extractGraphicsFile(content);
            } else if (content.find("\\caption") != std::string::npos) {
                currentSectionJson["figure_caption"] = extractFigureCaption(content);
            } else if (content.find("\\end{figure}") != std::string::npos) {
                inFigure = false;
                documentJson["document"]["content"].push_back(currentSectionJson);
                currentSectionJson.clear();
            }
            else if (content.find("\\cite") != std::string::npos) {
                json referenceJson;
                referenceJson["label"] = extractCitationLabel(content);
                referenceJson["link"] = "";
                currentSectionJson["references"].push_back(referenceJson);
            }
            else if (content.find("\\section") != std::string::npos || content.find("\\subsection") != std::string::npos) {
                if (!currentSectionJson.empty()) {
                    documentJson["document"]["content"].push_back(currentSectionJson);
                    currentSectionJson.clear();
                }
                currentSection = extractSectionName(content);
                currentSectionJson["section"] = currentSection;
                currentSectionJson["type"] = "text";
                currentSectionJson["content"] = "";
                currentSectionJson["references"] = json::array();
            }
        }
        else if (node->getType() == ASTNode::NodeType::Text) {
            if (inAbstract || inEquation || inFigure || !currentSection.empty()) {
                currentSectionJson["content"] = currentSectionJson.value("content", "") + node->getContent();
            }
        }

        for (const auto& child : node->getChildren()) {
            traverse(child);
        }
    };

    traverse(root);

    if (!currentSectionJson.empty()) {
        documentJson["document"]["content"].push_back(currentSectionJson);
    }

    return documentJson;
}


std::string FSM::removeInvalidUTF8(const std::string& input) {
    std::string output;
    for (unsigned char c : input) {
        if (c < 0x80) {
            output += c;
        } else {
            output += '?'; 
        }
    }
    return output;
}

std::string FSM::removeNewlines(const std::string& input) {
    std::string output = input;
    output.erase(std::remove(output.begin(), output.end(), '\n'), output.end());
    output.erase(std::remove(output.begin(), output.end(), '\r'), output.end());  
    return output;
}

std::string FSM::extractAuthorName(const std::string& authorCommand) {
    std::string cleanedCommand = removeNewlines(authorCommand);

    std::regex authorRegex(R"(\\author\s*{([^}]*)})");
    std::smatch match;

    if (std::regex_search(cleanedCommand, match, authorRegex)) {
        std::string authorName = match[1].str();
        authorName.erase(0, authorName.find_first_not_of(" \t\n\r\f\v"));
        authorName.erase(authorName.find_last_not_of(" \t\n\r\f\v") + 1);
        return authorName;
    }

    return "";
}

std::string FSM::extractContentBetweenBraces(const std::string& str, size_t start_pos) {
    if (start_pos >= str.length() || str[start_pos] != '{') {
        return "";
    }
    int brace_count = 0;
    size_t pos = start_pos;
    size_t len = str.length();
    std::string content = "";

    while (pos < len) {
        if (str[pos] == '{') {
            brace_count++;
            if (brace_count > 1) { 
                content += str[pos];
            }
        } else if (str[pos] == '}') {
            brace_count--;
            if (brace_count == 0) {
                break;
            } else {
                if (brace_count >= 1)
                    content += str[pos];
            }
        } else {
            if (brace_count >= 1)
                content += str[pos];
        }
        pos++;
    }

    if (brace_count != 0) {
        std::cerr << "Error: Unmatched braces in the input string." << std::endl;
        return "";
    }

    return content;
}



std::string removeInvalidUTF8(const std::string& input) {
    std::string output;
    for (unsigned char c : input) {
        if (c < 0x80) {
            output += c;
        } else {
            output += '?';
        }
    }
    return output;
}


std::vector<std::string> FSM::extractMultipleAuthors(const std::string& authorTexCommand) {
    std::vector<std::string> authors;

    std::string authorCommand = removeInvalidUTF8(authorTexCommand);
    
    size_t authorPos = authorCommand.find("\\author");
    if (authorPos != std::string::npos) {
        size_t braceStart = authorCommand.find('{', authorPos);
        if (braceStart != std::string::npos) {
            std::string authorBlock = extractContentBetweenBraces(authorCommand, braceStart);

            std::cout << "Original Author Block: \n" << authorBlock << "\n";

            std::regex commentRegex(R"(%[^\n]*\n?)");
            authorBlock = std::regex_replace(authorBlock, commentRegex, "");

            std::regex lineBreakRegex(R"(\\newauthor|\\\\|\n|\r)");
            authorBlock = std::regex_replace(authorBlock, lineBreakRegex, " ");

            size_t thanksPos = authorBlock.find("\\thanks{");
            while (thanksPos != std::string::npos) {
                size_t braceStart = authorBlock.find('{', thanksPos);
                std::string thanksContent = extractContentBetweenBraces(authorBlock, braceStart);
                authorBlock.erase(thanksPos, braceStart - thanksPos + thanksContent.length() + 1);
                thanksPos = authorBlock.find("\\thanks{");
            }

            std::regex whitespaceRegex(R"(\s+)");
            authorBlock = std::regex_replace(authorBlock, whitespaceRegex, " ");

            std::regex symbolAndNumberRegex(R"([\$\^\{\}\d])");
            authorBlock = std::regex_replace(authorBlock, symbolAndNumberRegex, " ");

            std::regex splitRegex(R"(,|\s+and\s+)");
            std::sregex_token_iterator iter(authorBlock.begin(), authorBlock.end(), splitRegex, -1);
            std::sregex_token_iterator end;

            for (; iter != end; ++iter) {
                std::string author = cleanAuthor(iter->str());
                if (!author.empty()) {
                    authors.push_back(author);
                }
            }
        } else {
            std::cerr << "No opening brace found after \\author command." << std::endl;
        }
    } else {
        std::cerr << "No \\author command found." << std::endl;
    }

    return authors;
}




std::string FSM::cleanAuthor(const std::string& author) {
    std::string cleaned = author;

    cleaned = std::regex_replace(cleaned, std::regex(R"(\\[a-zA-Z]+\{[^}]*\})"), "");
    cleaned = std::regex_replace(cleaned, std::regex(R"(\$\^\{[^}]*\}\$)"), "");

    cleaned.erase(0, cleaned.find_first_not_of(" \t\n\r\f\v"));
    cleaned.erase(cleaned.find_last_not_of(" \t\n\r\f\v") + 1);

    return cleaned;
}



std::string FSM::extractAffiliation(const std::string& affiliationCommand) {
    size_t start = affiliationCommand.find("{") + 1;
    size_t end = affiliationCommand.find("}");
    std::string affiliation = affiliationCommand.substr(start, end - start);
    
    return cleanAuthor(affiliation); 
}

void FSM::processAffiliations(json& currentAuthor, const std::string& content, json& documentJson, bool& authorActive) {
    if (authorActive) {
        std::string affiliation = extractAffiliation(content);
        currentAuthor["affiliations"].push_back(affiliation);

        if (!currentAuthor["name"].is_null()) {
            documentJson["document"]["metadata"]["authors"].push_back(currentAuthor);
            currentAuthor.clear();
            authorActive = false;
        }
    }
}


std::string FSM::extractSectionName(const std::string& sectionCommand) {
    size_t start = sectionCommand.find("{") + 1;
    size_t end = sectionCommand.find("}");
    return sectionCommand.substr(start, end - start);
}

void FSM::handleDocument(const std::shared_ptr<ASTNode>& node, std::string& currentChunk) {
    currentChunk += "Document Root\n";
}

void FSM::handleSection(const std::shared_ptr<ASTNode>& node, std::string& currentChunk, std::vector<std::string>& chunks) {

	currentChunk += "[Section] " + node->getContent() + "\n";

    if (!currentChunk.empty()) {
        chunks.push_back(currentChunk);
        currentChunk.clear();
    }
}

void FSM::handleCommand(const std::shared_ptr<ASTNode>& node, std::string& currentChunk) {
    std::string commandContent = node->getContent();
    
    if (commandContent.find("\\begin{equation}") != std::string::npos) {
        currentChunk += "[Equation Start]\n";
    }
    
    else if (commandContent.find("\\label") != std::string::npos) {
        std::string label = extractCitationLabel(commandContent);
        currentChunk += "Equation Label: " + label + "\n";
    }
    
    else if (commandContent.find("\\cite") != std::string::npos) {
        std::string citationLabel = extractCitationLabel(commandContent);
        currentChunk += "Citation: " + citationLabel + "\n";
    }
    
    else if (commandContent.find("\\end{equation}") != std::string::npos) {
        currentChunk += "[Equation End]\n";
    } 
    else {
        currentChunk += "Command: " + commandContent + "\n";
    }
}
/*
void FSM::handleText(const std::shared_ptr<ASTNode>& node, std::string& currentChunk) {
    currentChunk += "Text: " + node->getContent() + "\n";
}
*/

void FSM::handleText(const std::shared_ptr<ASTNode>& node, std::string& currentChunk) {
    std::string textContent = node->getContent();

    if (currentChunk.find("[Equation Start]") != std::string::npos && currentChunk.find("[Equation End]") == std::string::npos) {
        currentChunk += "Equation Content: " + textContent + "\n";
    }
    else {
        currentChunk += "Text: " + textContent + "\n";
    }
}

void FSM::handleEnvironment(const std::shared_ptr<ASTNode>& node, std::string& currentChunk) {
    currentChunk += "Environment: " + node->getContent() + "\n";
}

std::string FSM::extractGraphicsFile(const std::string& graphicsCommand) {
    size_t start = graphicsCommand.find("{") + 1;
    size_t end = graphicsCommand.find("}");
    return graphicsCommand.substr(start, end - start);
}

std::string FSM::extractFigureCaption(const std::string& captionCommand) {
    size_t start = captionCommand.find("{") + 1;
    size_t end = captionCommand.find("}");
    return captionCommand.substr(start, end - start);
}

std::string FSM::extractCitationLabel(const std::string& citationCommand) {
    size_t start = citationCommand.find("{") + 1;
    size_t end = citationCommand.find("}");
    return citationCommand.substr(start, end - start);
}
