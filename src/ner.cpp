#include "ner.h"
#include <iostream>
#include <sstream>


NER::NER()
    : authorPattern(R"(\\author\s*(?:\[[^\]]*\])?\s*\{([^}]*?)\})"),
      affiliationPattern(R"(\\affiliation\s*(?:\[[^\]]*\])?\s*\{([^}]*?)\})"),
      affilPattern(R"(\\affil\s*(?:\[[^\]]*\])?\s*\{([^}]*?)\})"),
      instPattern(R"(\\inst\{([^}]*?)\})"),
      institutePattern(R"(\\institute\s*(?:\[[^\]]*\])?\s*\{([\s\S]*?)\})")
{
    try {
        std::regex test(authorPattern);
        std::regex testAffil(affiliationPattern);
        std::regex testAffilShort(affilPattern);
        std::regex testInst(instPattern);
        std::regex testInstitute(institutePattern);
    } catch (const std::regex_error& e) {
        std::cerr << "Regex compilation error: " << e.what() << std::endl;
    }
}

const std::unordered_map<std::string, std::vector<std::string>>& NER::getEntities() const {
    return entities;
}

std::vector<std::string> NER::matchRegex(const std::string& content, const std::regex& pattern) const {
    std::vector<std::string> matches;
    std::smatch match;

    std::string::const_iterator searchStart(content.cbegin());
    while (std::regex_search(searchStart, content.cend(), match, pattern)) {
        if (match.size() > 1) {
            matches.push_back(match[1].str());
        }
        auto oldPosition = searchStart;  
        searchStart = match.suffix().first;  

        if (searchStart == oldPosition || searchStart == content.cend()) {
            break;
        }
    }

    return matches;
}


std::string NER::cleanText(const std::string& input) const {
    std::string cleaned = input;
    cleaned = std::regex_replace(cleaned, std::regex(R"(\\[a-zA-Z]+\{[^}]*\})"), "");
    cleaned = std::regex_replace(cleaned, std::regex(R"(~)"), " ");
    cleaned = std::regex_replace(cleaned, std::regex(R"(\\&)"), "&");
    cleaned = std::regex_replace(cleaned, std::regex(R"(\')"), "'");
    cleaned = std::regex_replace(cleaned, std::regex(R"(\^)"), "^");
    cleaned = std::regex_replace(cleaned, std::regex(R"(\{)"), "{");
    cleaned = std::regex_replace(cleaned, std::regex(R"(\})"), "}");
    cleaned.erase(0, cleaned.find_first_not_of(" \t\n\r\f\v"));
    cleaned.erase(cleaned.find_last_not_of(" \t\n\r\f\v") + 1);
    return cleaned;
}


void NER::parseLaTeX(const std::string& content, DAG& dag, const std::shared_ptr<ASTNode>& astNode) {
    extractAuthors(content, dag, astNode);
    extractAffiliations(content, dag, astNode);
    mapAuthorsToAffiliations(dag);  
}



void NER::extractAuthors(const std::string& content, DAG& dag, const std::shared_ptr<ASTNode>& astNode) {
    std::cout << "\n\nNER EXTRACT AUTHORS\nContent:\n" << content << "\n\n" << std::endl;
    
    authorInstMap.clear();

    auto hierarchicalAuthors = matchRegex(content, authorPattern);
    std::cout << "Extracted authors (hierarchical): " << hierarchicalAuthors.size() << std::endl;
    for (const auto& authorName : hierarchicalAuthors) {
        std::cout << "Hierarchical Author: " << authorName << std::endl;
        std::string cleanedAuthorName = cleanText(authorName);
        auto authorNode = dag.getOrCreateNode(cleanedAuthorName, NodeType::Author);
        if (authorNode) {
            authorNode->setASTNode(astNode);
            entities["authors"].push_back(cleanedAuthorName);
        }
    }

    std::regex authorMapRegex(R"(\\author\s*\{([\s\S]*?)\})");
    std::smatch authorMapMatch;
    if (std::regex_search(content, authorMapMatch, authorMapRegex)) {
        if (authorMapMatch.size() > 1) {
            std::string authorsBlock = authorMapMatch[1].str();
            std::cout << "Map-like Authors Block: " << authorsBlock << std::endl;

            std::regex andRegex(R"(\\and)");
            std::sregex_token_iterator iter(authorsBlock.begin(), authorsBlock.end(), andRegex, -1);
            std::sregex_token_iterator end;

            int mapLikeAuthorsCount = 0;

            for (; iter != end; ++iter) {
                std::string authorSegment = iter->str();
                std::cout << "Author Segment after splitting by \\and: " << authorSegment << std::endl;

                std::regex mboxRegex(R"(\\mbox\{([\s\S]*?)\})");
                std::smatch mboxMatch;
                if (std::regex_search(authorSegment, mboxMatch, mboxRegex)) {
                    if (mboxMatch.size() > 1) {
                        std::string mboxContent = mboxMatch[1].str();
                        std::cout << "Extracted \\mbox{...} Content: " << mboxContent << std::endl;

                        std::regex instRegex(R"(\\inst\{([^}]*?)\})");
                        std::smatch instMatch;
                        std::string authorName = mboxContent;
                        std::vector<std::string> instLabels;

                        while (std::regex_search(authorName, instMatch, instRegex)) {
                            if (instMatch.size() > 1) {
                                std::string instContent = instMatch[1].str();
                                std::regex commaRegex(R"(,)");
                                std::sregex_token_iterator commaIter(instContent.begin(), instContent.end(), commaRegex, -1);
                                std::sregex_token_iterator commaEnd;
                                for (; commaIter != commaEnd; ++commaIter) {
                                    std::string label = commaIter->str();
                                    std::regex refRegex(R"(\\ref\{([^}]+)\})");
                                    std::smatch refMatch;
                                    if (std::regex_search(label, refMatch, refRegex)) {
                                        if (refMatch.size() > 1) {
                                            instLabels.push_back(refMatch[1].str());
                                        }
                                    } else {
                                        instLabels.push_back(label);
                                    }
                                }
                            }
                            authorName = std::regex_replace(authorName, instRegex, "");
                        }

                        authorName = cleanText(authorName);
                        std::cout << "Map-like Author Name: " << authorName << std::endl;

                        auto authorNode = dag.getOrCreateNode(authorName, NodeType::Author);
                        if (authorNode) {
                            authorNode->setASTNode(astNode);
                            entities["authors"].push_back(authorName);
                            if (!instLabels.empty()) {
                                authorInstMap[authorName] = instLabels;
                                std::cout << "Inst labels for " << authorName << ": ";
                                for (const auto& label : instLabels) {
                                    std::cout << label << " ";
                                }
                                std::cout << std::endl;
                            }
                            mapLikeAuthorsCount++;
                        }
                    }
                }
            }

            std::cout << "Total Map-like Authors Extracted: " << mapLikeAuthorsCount << std::endl;
        }
    } else {
        std::cout << "No map-like \\author{...} block found." << std::endl;
    }
}

void NER::extractAffiliations(const std::string& content, DAG& dag, const std::shared_ptr<ASTNode>& astNode) {
    instAffilMap.clear();

    auto affiliationMatches = matchRegex(content, affiliationPattern);
    std::cout << "Extracted affiliations (\\affiliation): " << affiliationMatches.size() << std::endl;
    for (const auto& affiliationContent : affiliationMatches) {
        std::cout << "Affiliation: " << affiliationContent << std::endl;
        std::string cleanedAffiliation = cleanText(affiliationContent);
        auto affiliationNode = dag.getOrCreateNode(cleanedAffiliation, NodeType::Affiliation);
        if (affiliationNode) {
            affiliationNode->setASTNode(astNode);
            entities["affiliations"].push_back(cleanedAffiliation);
        }
    }

    auto affilMatches = matchRegex(content, affilPattern);
    std::cout << "Extracted affiliations (\\affil): " << affilMatches.size() << std::endl;
    for (const auto& affilContent : affilMatches) {
        std::cout << "Affiliation (short): " << affilContent << std::endl;
        std::string cleanedAffil = cleanText(affilContent);
        auto affiliationNode = dag.getOrCreateNode(cleanedAffil, NodeType::Affiliation);
        if (affiliationNode) {
            affiliationNode->setASTNode(astNode);
            entities["affiliations"].push_back(cleanedAffil);
        }
    }

    std::regex instituteRegex(R"(\\institute\s*(?:\[[^\]]*\])?\s*\{([\s\S]*?)\})");
    std::smatch instituteMatch;
    auto instituteIter = std::sregex_iterator(content.begin(), content.end(), institutePattern);
    auto instituteEnd = std::sregex_iterator();

    for (; instituteIter != instituteEnd; ++instituteIter) {
        std::smatch match = *instituteIter;
        if (match.size() > 1) {
            std::string institutesContent = match[1].str();
            std::cout << "Extracted institutes: " << institutesContent << std::endl;
            std::regex andRegex(R"(\\and)");
            std::sregex_token_iterator iter(institutesContent.begin(), institutesContent.end(), andRegex, -1);
            std::sregex_token_iterator end;

            while (iter != end) {
                std::string instituteBlock = iter->str();
                std::cout << "Institute Block: " << instituteBlock << std::endl;

                std::regex labelRegex(R"(\\label\{([^}]+)\})");
                std::smatch labelMatch;
                if (std::regex_search(instituteBlock, labelMatch, labelRegex)) {
                    if (labelMatch.size() > 1) {
                        std::string label = labelMatch[1].str();
                        std::string affiliation = std::regex_replace(instituteBlock, labelRegex, "");
                        affiliation = cleanText(affiliation);
                        std::cout << "Affiliation with label " << label << ": " << affiliation << std::endl;
                        auto affiliationNode = dag.getOrCreateNode(affiliation, NodeType::Affiliation);
                        if (affiliationNode) {
                            affiliationNode->setASTNode(astNode);
                            entities["affiliations"].push_back(affiliation);
                            instAffilMap[label] = affiliation;
                        }
                    }
                } else {
                    std::string affiliation = cleanText(instituteBlock);
                    std::cout << "Affiliation without label: " << affiliation << std::endl;
                    auto affiliationNode = dag.getOrCreateNode(affiliation, NodeType::Affiliation);
                    if (affiliationNode) {
                        affiliationNode->setASTNode(astNode);
                        entities["affiliations"].push_back(affiliation);
                    }
                }

                ++iter;
            }
        }
    }
}

void NER::mapAuthorsToAffiliations(DAG& dag) {
    linkAuthorsToAffiliations(dag);
}

void NER::linkAuthorsToAffiliations(DAG& dag) {
    for (const auto& authorPair : authorInstMap) {
        std::string authorName = authorPair.first;
        std::vector<std::string> instLabels = authorPair.second;
        auto authorNode = dag.getNode(authorName);

        if (authorNode) {
            for (const auto& label : instLabels) {
                auto it = instAffilMap.find(label);
                if (it != instAffilMap.end()) {
                    std::string affiliationName = it->second;
                    auto affiliationNode = dag.getNode(affiliationName);

                    if (affiliationNode) {
                        authorNode->addChild(affiliationNode);
                        affiliationNode->addParent(authorNode);
                        std::cout << "Linked author " << authorName << " to affiliation " << affiliationName << std::endl;
                    } else {
                        std::cerr << "Warning: Affiliation node not found for affiliation: " << affiliationName << std::endl;
                    }
                } else {
                    std::cerr << "Warning: Label not found in instAffilMap: " << label << std::endl;
                }
            }
        } else {
            std::cerr << "Warning: Author node not found for author: " << authorName << std::endl;
        }
    }
}
