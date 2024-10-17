#ifndef NER_H
#define NER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <regex>
#include "dag_node.h"
#include "ast.h"


class NER {
public:
    NER();

    void parseLaTeX(const std::string& content, DAG& dag, const std::shared_ptr<ASTNode>& astNode);

    const std::unordered_map<std::string, std::vector<std::string>>& getEntities() const;

    std::string cleanText(const std::string& input) const;

    const std::regex& getAuthorPattern() const { return authorPattern; }
    const std::regex& getAffiliationPattern() const { return affiliationPattern; }
    const std::regex& getAffilPattern() const { return affilPattern; }
    const std::regex& getInstPattern() const { return instPattern; }
    const std::regex& getInstitutePattern() const { return institutePattern; }

    std::vector<std::string> matchRegex(const std::string& content, const std::regex& pattern) const;

private:
    void extractAuthors(const std::string& content, DAG& dag, const std::shared_ptr<ASTNode>& astNode);
    void extractAffiliations(const std::string& content, DAG& dag, const std::shared_ptr<ASTNode>& astNode);
    void linkAuthorsToAffiliations(DAG& dag);
    void mapAuthorsToAffiliations(DAG& dag);  

    std::unordered_map<std::string, std::vector<std::string>> entities; 
    std::unordered_map<std::string, std::vector<std::string>> authorInstMap;
    std::unordered_map<std::string, std::string> instAffilMap; 

    std::regex authorPattern;
    std::regex affiliationPattern;
    std::regex affilPattern;        
    std::regex instPattern;
    std::regex institutePattern;    
};

#endif 

