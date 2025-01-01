#ifndef NER_H
#define NER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <regex>
#include <memory>
#include "dag_node.h"
#include "ast.h"
#include "lexer.h"
#include "crf_model.h"

class NER {
public:
    NER();
    void initializeCRFModel();

    void annotateWithCRF(const std::vector<Token>& tokens, DAG& dag, const std::shared_ptr<ASTNode>& astNode);

    const std::unordered_map<std::string, std::vector<std::string>>& getEntities() const { return entities; }
    std::vector<std::string> matchRegex(const std::string& content, const std::regex& pattern) const;
    const std::regex& getAffiliationPattern() const;


private:
    std::vector<std::vector<std::string>> extractFeatures(const std::vector<Token>& tokens) const;

    std::unordered_map<std::string, std::vector<std::string>> entities;

    std::unique_ptr<CRFModel> crfModel;
};

#endif

