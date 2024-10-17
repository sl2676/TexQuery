#include "fsm.h"
using json = nlohmann::json;

FSM::FSM() : currentState(FSMState::Start), ner() {}



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
        if (child) {  
            traverseAST(child, currentChunk, chunks);
        }
    }

    if (auto dagNode = node->getDAGNode()) {
        for (const auto& childDagNode : dagNode->getChildren()) {
            if (childDagNode) { 
                if (auto linkedAstNode = childDagNode->getASTNode().lock()) {
                    if (linkedAstNode) {
                        traverseAST(linkedAstNode, currentChunk, chunks);
                    }
                }
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

std::shared_ptr<DAGNode> FSM::createOrGetDAGNode(const std::string& content, NodeType type) {
    auto existingNode = dag.getNode(content);
    if (!existingNode) {
        auto newNode = std::make_shared<DAGNode>(content, type);
        dag.addNode(newNode);
        return newNode;
    }
    return existingNode;
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




void FSM::parseAuthors(const std::string& authorBlock, std::vector<Author>& authors, const std::unordered_map<std::string, std::string>& affiliationMap) {
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

        std::regex orcidRegex(R"(\[\d{4}-\d{4}-\d{4}-\d{4}\])");
        std::smatch orcidMatch;
        if (std::regex_search(authorContent, orcidMatch, orcidRegex)) {
            author.orcid = orcidMatch.str();
            authorContent = std::regex_replace(authorContent, orcidRegex, "");  
        }

        author.name = authorContent;

        auto authorDAGNode = dag.getOrCreateNode(author.name, NodeType::Author);
        dag.addNode(authorDAGNode);
        authors.push_back(author);
    }
}


void FSM::parseAffiliations(const std::string& affiliationBlock, std::unordered_map<std::string, std::string>& affiliationMap, std::vector<Author>& authors, std::shared_ptr<DAGNode> currentAuthorDAGNode) {
    if (!currentAuthorDAGNode) {
        std::cerr << "Error: No author found for affiliation: " << affiliationBlock << "\n";
        unlabeledAffiliations.push_back(affiliationBlock);  
        return;
    }

    std::vector<std::string> affiliations = ner.matchRegex(affiliationBlock, ner.getAffiliationPattern());  
    if (affiliations.empty()) {
        std::cerr << "Warning: No affiliations found in block: " << affiliationBlock << "\n";
        return;
    }

    for (const auto& affiliation : affiliations) {
        if (affiliation.empty()) {
            std::cerr << "Warning: Empty affiliation found and skipped.\n";
            continue;
        }

        auto affiliationDAGNode = dag.getOrCreateNode(affiliation, NodeType::Affiliation);
        if (!affiliationDAGNode) {
            std::cerr << "Error: Failed to create or get affiliation node for: " << affiliation << "\n";
            continue;
        }
        currentAuthorDAGNode->addChild(affiliationDAGNode);
        affiliationDAGNode->addParent(currentAuthorDAGNode);
        affiliationMap[affiliation] = affiliation;  
        dag.addAffiliationNode(affiliation);  
    }
}




void FSM::linkUnlabeledAffiliations(std::vector<Author>& authors, std::vector<std::string>& unlabeledAffiliations) {
    size_t numAuthors = authors.size();
    size_t numUnlabeledAffils = unlabeledAffiliations.size();

    for (size_t i = 0; i < numAuthors && i < numUnlabeledAffils; ++i) {
        authors[i].affiliations.push_back(unlabeledAffiliations[i]);

        auto authorNode = dag.getOrCreateNode(authors[i].name, NodeType::Author);
        auto affiliationNode = dag.getOrCreateNode(unlabeledAffiliations[i], NodeType::Affiliation);
        dag.linkAuthorToAffiliation(authors[i].name, unlabeledAffiliations[i]);
    }
}


void FSM::parseAuthorsStyle2(const std::string& content, std::vector<Author>& authors) {
    std::regex authorRegex(R"(\\author(\[[^\]]*\])?\{([^\}]*)\})");
    auto authorIter = std::sregex_iterator(content.begin(), content.end(), authorRegex);
    auto authorEnd = std::sregex_iterator();

    for (; authorIter != authorEnd; ++authorIter) {
        std::smatch match = *authorIter;
        Author author;
        size_t authorEndPos = match.position(0) + match.length(0);

        if (match[1].matched) {
            std::string orcidContent = match[1].str();
            orcidContent = orcidContent.substr(1, orcidContent.length() - 2); 
            author.orcid = orcidContent;
        }
        author.name = match[2].str();

        size_t nextAuthorStart = content.length();
        auto nextAuthorIter = authorIter;
        ++nextAuthorIter;
        if (nextAuthorIter != authorEnd) {
            nextAuthorStart = (*nextAuthorIter).position(0);
        }
        std::string authorBlock = content.substr(authorEndPos, nextAuthorStart - authorEndPos);

        std::regex affiliationRegex(R"(\\affiliation\{([^\}]*)\})");
        auto affilIter = std::sregex_iterator(authorBlock.begin(), authorBlock.end(), affiliationRegex);
        for (; affilIter != std::sregex_iterator(); ++affilIter) {
            std::smatch affilMatch = *affilIter;
            author.affiliations.push_back(affilMatch[1].str());
        }

        std::regex emailRegex(R"(\\email\{([^\}]*)\})");
        auto emailIter = std::sregex_iterator(authorBlock.begin(), authorBlock.end(), emailRegex);
        for (; emailIter != std::sregex_iterator(); ++emailIter) {
            std::smatch emailMatch = *emailIter;
            author.email = emailMatch[1].str();
        }

        std::cout << "Extracted author name: " << author.name << std::endl;
        if (!author.affiliations.empty()) {
            std::cout << "Affiliations: " << std::endl;
            for (const auto& affil : author.affiliations) {
                std::cout << " - " << affil << std::endl;
            }
        }
        if (!author.email.empty()) {
            std::cout << "Email: " << author.email << std::endl;
        }

        authors.push_back(author);
    }
}

void FSM::parseAuthorsStyle1(const std::string& authorBlock, std::vector<Author>& authors, const std::unordered_map<std::string, std::string>& affiliationMap) {
    std::vector<std::string> authorNames = ner.matchRegex(authorBlock, ner.getAuthorPattern());  
    for (const std::string& authorName : authorNames) {
        Author author;
        author.name = ner.cleanText(authorName);

        auto authorDAGNode = dag.getOrCreateNode(author.name, NodeType::Author);
        dag.addAuthorNode(author.name);
        authors.push_back(author);
    }
}

json FSM::chunkDocumentToJson(const std::shared_ptr<ASTNode>& root) {
    json documentJson;
    documentJson["document"]["metadata"]["authors"] = json::array();
    documentJson["document"]["metadata"]["affiliations"] = json::array(); 
    documentJson["document"]["content"] = json::array();

    std::string currentChunk;
    std::vector<std::string> chunks;

    traverseAST(root, currentChunk, chunks);

    const auto& entities = ner.getEntities();

    std::unordered_map<std::string, int> affiliationIds;
    int affiliationCounter = 1;

    if (entities.find("authors") != entities.end()) {
        for (const auto& authorName : entities.at("authors")) {
            json authorJson;
            authorJson["name"] = authorName;
            authorJson["affiliations"] = json::array();

            auto authorNode = dag.getNode(authorName);
            if (authorNode) {
                for (const auto& child : authorNode->getChildren()) {
                    if (child->getNodeType() == NodeType::Affiliation) {
                        std::string affiliation = child->getId();

                        if (affiliationIds.find(affiliation) == affiliationIds.end()) {
                            affiliationIds[affiliation] = affiliationCounter++;

                            json affiliationJson;
                            affiliationJson["id"] = affiliationIds[affiliation];
                            affiliationJson["details"] = affiliation;
                            documentJson["document"]["metadata"]["affiliations"].push_back(affiliationJson);
                        }

                        authorJson["affiliations"].push_back(affiliationIds[affiliation]);
                    }
                }
            }

            documentJson["document"]["metadata"]["authors"].push_back(authorJson);
        }
    } else {
        std::cerr << "Warning: No authors found in the NER entities." << std::endl;
    }

    for (const auto& chunk : chunks) {
        documentJson["document"]["content"].push_back(chunk);
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
    if (authorCommand.empty()) {
        std::cerr << "Error: Empty author command provided to extractAuthorName.\n";
        return "";
    }

    std::string cleanedCommand = removeNewlines(authorCommand);
    std::regex authorRegex(R"(\\author\s*{([^}]*)})");
    std::smatch match;

    if (std::regex_search(cleanedCommand, match, authorRegex)) {
        std::string authorName = match[1].str();
        authorName.erase(0, authorName.find_first_not_of(" \t\n\r\f\v"));
        authorName.erase(authorName.find_last_not_of(" \t\n\r\f\v") + 1);
        return authorName;
    }

    std::cerr << "Error: No valid author name found in command: " << authorCommand << "\n";
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
    
    if (commandContent.find("\\author") != std::string::npos) {
        insideAuthorBlock = true;
        size_t bracePos = commandContent.find('{');
        if (bracePos != std::string::npos) {
            authorBlockContent += commandContent.substr(bracePos + 1) + "\n";
        } else {
            std::cerr << "Error: No opening brace found after \\author command.\n";
        }
        return; 
    }
    
    if (commandContent.find("\\institute") != std::string::npos) {
        insideInstituteBlock = true;
        size_t bracePos = commandContent.find('{');
        if (bracePos != std::string::npos) {
            instituteBlockContent += commandContent.substr(bracePos + 1) + "\n";
        } else {
            std::cerr << "Error: No opening brace found after \\institute command.\n";
        }
        return; 
    }
    
    if (insideAuthorBlock) {
        size_t closingBrace = commandContent.find('}');
        if (closingBrace != std::string::npos) {
            authorBlockContent += commandContent.substr(0, closingBrace);
            ner.parseLaTeX("\\author{" + authorBlockContent + "}", dag, node);
            insideAuthorBlock = false;
            authorBlockContent.clear();
        } else {
            authorBlockContent += commandContent + "\n";
        }
        return; 
    }
    
    if (insideInstituteBlock) {
        size_t closingBrace = commandContent.find('}');
        if (closingBrace != std::string::npos) {
            instituteBlockContent += commandContent.substr(0, closingBrace);
            ner.parseLaTeX("\\institute{" + instituteBlockContent + "}", dag, node);
            insideInstituteBlock = false;
            instituteBlockContent.clear();
        } else {
            instituteBlockContent += commandContent + "\n";
        }
        return; 
    }
    
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

    if (insideAuthorBlock) {
        authorBlockContent += textContent + "\n";
        return; 
    }

    if (insideInstituteBlock) {
        instituteBlockContent += textContent + "\n";
        return; 
    }

    if (currentChunk.find("[Equation Start]") != std::string::npos && currentChunk.find("[Equation End]") == std::string::npos) {
        currentChunk += "Equation Content: " + textContent + "\n";
    } else {
        std::regex inlineMathRegex(R"(\$(.*?)\$)");
        std::smatch matches;
        std::string::const_iterator searchStart(textContent.cbegin());
        std::string remainingText = textContent;
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

DAG& FSM::getDAG() {
    return dag;
}
 
