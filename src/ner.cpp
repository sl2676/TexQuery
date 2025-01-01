#include "ner.h"
#include <algorithm>
#include <iostream>

NER::NER() {}

void NER::initializeCRFModel() {
    crfModel = std::make_unique<CRFModel>();

    std::vector<std::vector<std::vector<std::string>>> trainFeatures = {
        {{"WORD=\\author","CAP"}, {"WORD=John","CAP"}, {"WORD=Doe","CAP"}},
        {{"WORD=\\affiliation"}, {"WORD=SomeUniversity","CAP"}},
        {{"WORD=\\institute"}, {"WORD=AnotherInstitute","CAP"}}
    };
    std::vector<std::vector<CRFModel::Label>> trainLabels = {
        {CRFModel::O, CRFModel::AUTHOR, CRFModel::AUTHOR},
        {CRFModel::O, CRFModel::AFFILIATION},
        {CRFModel::O, CRFModel::AFFILIATION}
    };

    crfModel->train(trainFeatures, trainLabels, 5, 0.1);
}

void NER::annotateWithCRF(const std::vector<Token>& tokens, DAG& dag, const std::shared_ptr<ASTNode>& astNode) {
    if (!crfModel) {
        std::cerr << "CRF model not initialized!" << std::endl;
        return;
    }

    auto sequences = extractFeatures(tokens);
    if (sequences.empty()) return;

    auto labels = crfModel->predict(sequences);

    std::string currentEntity;
    CRFModel::Label currentLabel = CRFModel::O;

    for (size_t i = 0; i < tokens.size(); ++i) {
        auto lbl = labels[i];
        if (lbl == CRFModel::AUTHOR) {
            if (currentLabel != CRFModel::AUTHOR) {
                if (currentLabel == CRFModel::AFFILIATION && !currentEntity.empty()) {
                    auto trimmed = currentEntity;
                    trimmed.erase(std::remove_if(trimmed.begin(), trimmed.end(), ::isspace), trimmed.end());
                    entities["affiliations"].push_back(trimmed);
                    auto affNode = dag.getOrCreateNode(trimmed, NodeType::Affiliation);
                    if (affNode) affNode->setASTNode(astNode);
                    currentEntity.clear();
                }
                currentEntity.clear();
            }
            currentEntity += tokens[i].value + " ";
            currentLabel = CRFModel::AUTHOR;
        } else if (lbl == CRFModel::AFFILIATION) {
            if (currentLabel == CRFModel::AUTHOR && !currentEntity.empty()) {
                auto trimmed = currentEntity;
                trimmed.erase(std::remove_if(trimmed.begin(), trimmed.end(), ::isspace), trimmed.end());
                entities["authors"].push_back(trimmed);
                auto authorNode = dag.getOrCreateNode(trimmed, NodeType::Author);
                if (authorNode) authorNode->setASTNode(astNode);
                currentEntity.clear();
            }

            if (currentLabel != CRFModel::AFFILIATION) {
                currentEntity.clear();
            }
            currentEntity += tokens[i].value + " ";
            currentLabel = CRFModel::AFFILIATION;
        } else {
            // lbl == O
            if (currentLabel == CRFModel::AUTHOR && !currentEntity.empty()) {
                auto trimmed = currentEntity;
                trimmed.erase(std::remove_if(trimmed.begin(), trimmed.end(), ::isspace), trimmed.end());
                entities["authors"].push_back(trimmed);
                auto authorNode = dag.getOrCreateNode(trimmed, NodeType::Author);
                if (authorNode) authorNode->setASTNode(astNode);
            } else if (currentLabel == CRFModel::AFFILIATION && !currentEntity.empty()) {
                auto trimmed = currentEntity;
                trimmed.erase(std::remove_if(trimmed.begin(), trimmed.end(), ::isspace), trimmed.end());
                entities["affiliations"].push_back(trimmed);
                auto affNode = dag.getOrCreateNode(trimmed, NodeType::Affiliation);
                if (affNode) affNode->setASTNode(astNode);
            }

            currentEntity.clear();
            currentLabel = CRFModel::O;
        }
    }

    if (currentLabel == CRFModel::AUTHOR && !currentEntity.empty()) {
        auto trimmed = currentEntity;
        trimmed.erase(std::remove_if(trimmed.begin(), trimmed.end(), ::isspace), trimmed.end());
        entities["authors"].push_back(trimmed);
        auto authorNode = dag.getOrCreateNode(trimmed, NodeType::Author);
        if (authorNode) authorNode->setASTNode(astNode);
    } else if (currentLabel == CRFModel::AFFILIATION && !currentEntity.empty()) {
        auto trimmed = currentEntity;
        trimmed.erase(std::remove_if(trimmed.begin(), trimmed.end(), ::isspace), trimmed.end());
        entities["affiliations"].push_back(trimmed);
        auto affNode = dag.getOrCreateNode(trimmed, NodeType::Affiliation);
        if (affNode) affNode->setASTNode(astNode);
    }
}

std::vector<std::vector<std::string>> NER::extractFeatures(const std::vector<Token>& tokens) const {
    std::vector<std::vector<std::string>> featureVectors;
    for (size_t i = 0; i < tokens.size(); ++i) {
        const auto& token = tokens[i];
        std::vector<std::string> fv;

        fv.push_back("WORD=" + token.value);

        bool isCap = !token.value.empty() && std::isupper((unsigned char)token.value[0]);
        fv.push_back(isCap ? "CAP" : "NOT_CAP");

        if (i > 0) {
            fv.push_back("PREV_WORD=" + tokens[i-1].value);
        } else {
            fv.push_back("PREV_WORD=BOS");
        }
        if (i < tokens.size() - 1) {
            fv.push_back("NEXT_WORD=" + tokens[i+1].value);
        } else {
            fv.push_back("NEXT_WORD=EOS");
        }

        featureVectors.push_back(fv);
    }
    return featureVectors;
}


std::vector<std::string> NER::matchRegex(const std::string& content, const std::regex& pattern) const {
    std::vector<std::string> matches;
    std::smatch match;
    std::string::const_iterator searchStart(content.cbegin());
    while (std::regex_search(searchStart, content.cend(), match, pattern)) {
        if (match.size() > 1) {
            matches.push_back(match[1].str());
        }
        searchStart = match.suffix().first;
    }
    return matches;
}

const std::regex& NER::getAffiliationPattern() const {
    static const std::regex affiliationPattern(R"(\\affiliation\s*(?:\[[^\]]*\])?\s*\{([^}]*?)\})");
    return affiliationPattern;
}

