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


std::string convertSpecialSequences(const std::string& input) {
    std::string output;
    std::regex specialSeqRegex(R"(\\([A-Fa-f0-9]{2}))");
    auto iter = std::sregex_iterator(input.begin(), input.end(), specialSeqRegex);
    auto end = std::sregex_iterator();
    size_t lastPos = 0;

    for (; iter != end; ++iter) {
        std::smatch match = *iter;
        size_t matchPos = match.position(0);
        output += input.substr(lastPos, matchPos - lastPos);

        std::string hexCode = match[1].str();
        try {
            int code = std::stoi(hexCode, nullptr, 16);
            if (code >= 0 && code <= 255) {
                char ch = static_cast<char>(code);
                output += ch;
            } else {
                std::cerr << "Warning: Hex code out of range ('" << hexCode << "') in convertSpecialSequences.\n";
                output += match.str(0);
            }
        } catch (const std::invalid_argument& e) {
            std::cerr << "Warning: Invalid hex code ('" << hexCode << "') in convertSpecialSequences.\n";
            output += match.str(0);
        } catch (const std::out_of_range& e) {
            std::cerr << "Warning: Hex code out of range ('" << hexCode << "') in convertSpecialSequences.\n";
            output += match.str(0);
        }

        lastPos = matchPos + match.length(0);
    }

    output += input.substr(lastPos);
    return output;
}


void FSM::parseAffiliations(const std::string& instituteBlock, std::vector<Affiliation>& affiliations) {
    std::string content = instituteBlock;

    std::regex commentRegex(R"(%[^\n]*\n?)");
    content = std::regex_replace(content, commentRegex, "");

    content = std::regex_replace(content, std::regex(R"(\r?\n)"), " ");

    std::regex splitRegex(R"(\\and)");
    std::sregex_token_iterator iter(content.begin(), content.end(), splitRegex, -1);
    std::sregex_token_iterator end;
    int index = 1;

    for (; iter != end; ++iter) {
        std::string affilContent = iter->str();
        affilContent = std::regex_replace(affilContent, std::regex(R"(^\s+|\s+$)"), "");

        if (!affilContent.empty()) {
            Affiliation affiliation;
            affiliation.index = index++;
            affiliation.details = affilContent;
            affiliations.push_back(affiliation);
        }
    }
}


void FSM::parseAuthors(const std::string& authorBlock, std::vector<Author>& authors) {
    std::string content = authorBlock;

    std::regex commentRegex(R"(%[^\n]*\n?)");
    content = std::regex_replace(content, commentRegex, "");

    content = std::regex_replace(content, std::regex(R"(\r?\n)"), " ");

    std::regex andRegex(R"(\\and)");
    std::sregex_token_iterator iter(content.begin(), content.end(), andRegex, -1);
    std::sregex_token_iterator end;

    for (; iter != end; ++iter) {
        std::string authorContent = iter->str();
        Author author;

        authorContent = std::regex_replace(authorContent, std::regex(R"(\s+)"), " ");
        authorContent = std::regex_replace(authorContent, std::regex(R"(^\s+|\s+$)"), "");

        authorContent = std::regex_replace(authorContent, std::regex(R"(\\fnmsep|\\!)"), "");

        std::regex nameRegex(R"(^([^\\"{]+))");
        std::smatch nameMatch;
        if (std::regex_search(authorContent, nameMatch, nameRegex)) {
            author.name = nameMatch[1].str();
            author.name = std::regex_replace(author.name, std::regex(R"(^\s+|\s+$)"), "");
        }

        std::regex instRegex(R"(\\inst\{([^\}]*)\})");
        auto instIter = std::sregex_iterator(authorContent.begin(), authorContent.end(), instRegex);
        for (; instIter != std::sregex_iterator(); ++instIter) {
            std::string indicesStr = (*instIter)[1].str();
            std::regex commaSpaceRegex(R"([\s,]+)");
            std::sregex_token_iterator indexIter(indicesStr.begin(), indicesStr.end(), commaSpaceRegex, -1);
            std::sregex_token_iterator indexEnd;
			for (; indexIter != indexEnd; ++indexIter) {
                std::string index = indexIter->str();
                index = std::regex_replace(index, std::regex(R"(\s+)"), "");
                if (!index.empty()) {
                    try {
                        int affilIndex = std::stoi(index);
                        author.affiliationIndices.push_back(affilIndex);
                    } catch (const std::invalid_argument& e) {
                        std::cerr << "Warning: Invalid affiliation index '" << index << "' for author '" << author.name << "'. Skipping this index.\n";
                    } catch (const std::out_of_range& e) {
                        std::cerr << "Warning: Affiliation index out of range '" << index << "' for author '" << author.name << "'. Skipping this index.\n";
                    }
                }
            }
        }

        std::regex emailRegex(R"(\\thanks\{\\email\{([^\}]*)\}\})");
        std::smatch emailMatch;
        if (std::regex_search(authorContent, emailMatch, emailRegex)) {
            author.email = emailMatch[1].str();
        }

        std::regex orcidRegex(R"(\\orcidlink\{([^\}]*)\})");
        std::smatch orcidMatch;
        if (std::regex_search(authorContent, orcidMatch, orcidRegex)) {
            author.orcid = orcidMatch[1].str();
        }

        authors.push_back(author);
    }
}

json FSM::chunkDocumentToJson(const std::shared_ptr<ASTNode>& root) {
    json documentJson;
    documentJson["document"]["metadata"]["authors"] = json::array();
    documentJson["document"]["content"] = json::array();

    std::vector<Author> authors;
    std::vector<Affiliation> affiliations;

    std::string currentSection;
    json currentSectionJson;
    currentSectionJson["content"] = json::array(); 
    bool inAbstract = false;
    bool inEquation = false;
    bool inFigure = false;
    std::string currentEquationContent;

    std::function<void(const std::shared_ptr<ASTNode>&)> traverse;
    traverse = [&](const std::shared_ptr<ASTNode>& node) {
        if (!node) return;
        std::string content = node->getContent();

        if (node->getType() == ASTNode::NodeType::Document) {
            documentJson["document"]["title"] = "Untitled Document";
        } else if (node->getType() == ASTNode::NodeType::Command) {

            if (content.find("\\title") != std::string::npos) {
                documentJson["document"]["title"] = extractSectionName(content);
            } else if (content.find("\\author") != std::string::npos) {
                std::string authorBlock = extractContentBetweenBraces(content, content.find("{"));
                parseAuthors(authorBlock, authors);
            } else if (content.find("\\institute") != std::string::npos || content.find("\\affil") != std::string::npos) {
                std::string instituteBlock = extractContentBetweenBraces(content, content.find("{"));
                parseAffiliations(instituteBlock, affiliations);
            }
            std::regex beginEnvRegex(R"(\\begin\{(equation|align|eqnarray|multline|gather|align\*)\})");
            std::regex endEnvRegex(R"(\\end\{(equation|align|eqnarray|multline|gather|align\*)\})");

            if (std::regex_search(content, beginEnvRegex)) {
                inEquation = true;
                currentSectionJson["type"] = "equation";
                currentEquationContent.clear();
            } else if (std::regex_search(content, endEnvRegex)) {
                inEquation = false;
                currentSectionJson["content"] = currentEquationContent;
                documentJson["document"]["content"].push_back(currentSectionJson);
                currentSectionJson.clear();
                currentSectionJson["content"] = json::array(); 
            } else if (content.find("$") != std::string::npos) {
                currentSectionJson["content"].push_back(content);
            }
            else if (content.find("\\section") != std::string::npos || content.find("\\subsection") != std::string::npos) {
                if (!currentSectionJson.empty()) {
                    documentJson["document"]["content"].push_back(currentSectionJson);
                    currentSectionJson.clear();
                    currentSectionJson["content"] = json::array(); 
                }
                currentSection = extractSectionName(content);
                currentSectionJson["section"] = currentSection;
                currentSectionJson["type"] = "text";
                currentSectionJson["references"] = json::array();
            }
        } else if (node->getType() == ASTNode::NodeType::Text) {
            if (inEquation) {
                currentEquationContent += node->getContent();
            } else if (inAbstract || !currentSection.empty()) {
                std::string textContent = node->getContent();
                std::regex inlineMathRegex(R"(\$(.*?)\$)");
                std::smatch matches;
                std::string::const_iterator searchStart(textContent.cbegin());
                while (std::regex_search(searchStart, textContent.cend(), matches, inlineMathRegex)) {
                    std::string beforeMath = matches.prefix().str();
                    std::string mathExpression = matches[1].str();

                    if (!beforeMath.empty()) {
                        currentSectionJson["content"].push_back(beforeMath);
                    }

                    json inlineMathJson;
                    inlineMathJson["type"] = "inline_math";
                    inlineMathJson["content"] = mathExpression;

                    currentSectionJson["content"].push_back(inlineMathJson);

                    searchStart = matches.suffix().first;
                }
                std::string remainingText = std::string(searchStart, textContent.cend());
                if (!remainingText.empty()) {
                    currentSectionJson["content"].push_back(remainingText);
                }
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

    for (const auto& author : authors) {
        json authorJson;
        authorJson["name"] = author.name;
        if (!author.email.empty()) {
            authorJson["email"] = author.email;
        }
        if (!author.orcid.empty()) {
            authorJson["orcid"] = author.orcid;
        }
        authorJson["affiliations"] = json::array();
        for (int idx : author.affiliationIndices) {
            if (idx - 1 >= 0 && idx - 1 < affiliations.size()) {
                authorJson["affiliations"].push_back(affiliations[idx - 1].details);
            }
        }
        documentJson["document"]["metadata"]["authors"].push_back(authorJson);
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
    std::string content;

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
                content += str[pos];
            }
        } else {
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
    size_t braceStart = sectionCommand.find("{");
    if (braceStart != std::string::npos) {
        return extractContentBetweenBraces(sectionCommand, braceStart);
    }
    return "";
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

void FSM::handleText(const std::shared_ptr<ASTNode>& node, std::string& currentChunk) {
    std::string textContent = node->getContent();

    if (currentChunk.find("[Equation Start]") != std::string::npos && currentChunk.find("[Equation End]") == std::string::npos) {
        currentChunk += "Equation Content: " + textContent + "\n";
    } else {
        std::regex inlineMathRegex(R"(\$(.*?)\$)");
        std::smatch matches;
        std::string::const_iterator searchStart(textContent.cbegin());
        while (std::regex_search(searchStart, textContent.cend(), matches, inlineMathRegex)) {
            currentChunk += "Inline Math: " + matches[1].str() + "\n";
            searchStart = matches.suffix().first;
        }
        currentChunk += "Text: " + std::string(searchStart, textContent.cend()) + "\n";
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
