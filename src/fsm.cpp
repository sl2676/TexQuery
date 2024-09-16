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
    std::string currentAffiliation;

    bool inAbstract = false;
    bool inEquation = false;
    bool inFigure = false;

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
                currentAuthor["name"] = extractAuthorName(content);
                currentAuthor["affiliations"] = json::array();
            }
            else if (content.find("\\affiliation") != std::string::npos) {
                std::string affiliation = extractAffiliation(content);
                currentAuthor["affiliations"].push_back(affiliation);
                if (!currentAuthor["name"].is_null()) {
                    documentJson["document"]["metadata"]["authors"].push_back(currentAuthor);
                    currentAuthor.clear();
                }
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
            else if (content.find("\\begin{equation}") != std::string::npos || content.find("$") != std::string::npos) {
                inEquation = true;
                currentSectionJson["type"] = "equation";
                currentSectionJson["content"] = "";
            } else if (content.find("\\end{equation}") != std::string::npos || content.find("$") != std::string::npos) {
                inEquation = false;
                documentJson["document"]["content"].push_back(currentSectionJson);
                currentSectionJson.clear();
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

std::string FSM::extractAuthorName(const std::string& authorCommand) {
    size_t start = authorCommand.find("{") + 1;
    size_t end = authorCommand.find("}");
    return authorCommand.substr(start, end - start);
}

std::string FSM::extractAffiliation(const std::string& affiliationCommand) {
    size_t start = affiliationCommand.find("{") + 1;
    size_t end = affiliationCommand.find("}");
    return affiliationCommand.substr(start, end - start);
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
    if (!currentChunk.empty()) {
        chunks.push_back(currentChunk);
        currentChunk.clear();
    }
    currentChunk += "[Section] " + node->getContent() + "\n";
}

void FSM::handleCommand(const std::shared_ptr<ASTNode>& node, std::string& currentChunk) {
    currentChunk += "Command: " + node->getContent() + "\n";
}

void FSM::handleText(const std::shared_ptr<ASTNode>& node, std::string& currentChunk) {
    currentChunk += "Text: " + node->getContent() + "\n";
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

