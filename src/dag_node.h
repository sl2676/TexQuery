#ifndef DAG_NODE_H
#define DAG_NODE_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <set>
#include <atomic>
#include <chrono>
#include <nlohmann/json.hpp>
#include "ast.h"
#include <regex>
using json = nlohmann::json;

class ASTNode;
class DAGNode;
class RelationshipObserver;
class RelationshipManager;

struct PathInfo {
    std::vector<std::shared_ptr<DAGNode>> nodes;
    double strength;
    std::string description;
};

enum class NodeType {
    Document,
    Section,
    Author,
    Abstract,
    Affiliation,
    Citation,
    Reference,
    Figure,
    Table,
    Equation,
    Environment,
    Text,
    Command,
    Math,
    Unknown
};

enum class EdgeType {
    Hierarchical,     
    Citation,         
    CrossReference,   
    AuthorAffiliation,
    FigureReference,  
    TableReference,   
    EquationReference,
    RelatedWork,      
    AuthorMetadata,   

    // Semantic Types
    ConceptDependency,    
    MethodologyFlow,      
    DataDependency,       
    ResultSupports,       
    TheoremDependency,    
    ProofStep,           
    Definition,          
    Assumption,          
    
    // Research Context Relationships
    PriorWork,           
    FutureWork,          
    AlternativeApproach, 
    Limitation,          
    
    // Data and Resource Relationships
    DatasetUsage,        
    CodeReference,       
    SupplementaryLink,   
    ReproducibilityInfo, 
    
    // Domain-Specific Relationships
    ExperimentalSetup,   
    VariableRelation,    
    StatisticalLink,     
    CausalLink,          
    
    // Validation Relationships
    ValidationMethod,    
    ComparisonLink,     
    BenchmarkReference, 
    
    // Contribution Relationships
    MainContribution,   
    SubContribution,    
    NoveltyLink,       
    
    // Temporal Relationships
    VersionHistory,     
    TimelineLink,       
    
    // Semantic Structure
    ArgumentFlow,       
    EvidenceSupport,    
    CounterArgument,    
    
    // Collaboration Relationships
    AuthorContribution, 
    TeamCollaboration, 
    InstitutionalLink, 
    
    // Impact Relationships
    ApplicationDomain,  
    ImpactMeasure,     
    SocialImpact,      
    
    // Review Relationships
    PeerReviewLink,    
    RevisionHistory,   
    QualityMetric     
};

enum class EdgeEvent {
    Added,
    Removed,
    Modified
};

struct Edge {
    std::weak_ptr<DAGNode> target;
    EdgeType type;
    std::string label;
};

struct EdgeRule {
    NodeType sourceType;
    NodeType targetType;
    EdgeType edgeType;
    bool bidirectional;
    double weight;
    std::string description;
};

struct RelationshipMetadata {
    double confidence;
    std::string evidence;
    std::string context;
    json properties;
    std::chrono::system_clock::time_point timestamp;
};

class RelationshipObserver {
public:
    virtual ~RelationshipObserver() = default;
    virtual void onRelationshipChanged(EdgeEvent event, const Edge& edge, 
                                     const RelationshipMetadata& metadata) = 0;
};

struct SemanticInfo {
    std::string conceptType;
    std::vector<std::string> keywords;
    std::vector<std::string> topics;
    double importance;
    json semanticProperties;
};

class RelationshipManager {
public:
    RelationshipManager() = default;
    ~RelationshipManager() = default;

    void addRelationship(const std::shared_ptr<DAGNode>& target,
                    EdgeType type,
                    const RelationshipMetadata& metadata) {
    Edge edge{target, type, ""};  
    relationships[type][target] = metadata;
    notifyObservers(EdgeEvent::Added, edge, metadata);
}

void removeRelationship(const std::shared_ptr<DAGNode>& target, EdgeType type) {
    auto it = relationships.find(type);
    if (it != relationships.end()) {
        auto target_it = it->second.find(target);
        if (target_it != it->second.end()) {
            Edge edge{target, type, ""};  
            notifyObservers(EdgeEvent::Removed, edge, target_it->second);
            it->second.erase(target_it);
        }
    }
}

std::vector<std::pair<Edge, RelationshipMetadata>> 
findRelationsByType(EdgeType type) const {
    std::vector<std::pair<Edge, RelationshipMetadata>> results;
    auto it = relationships.find(type);
    if (it != relationships.end()) {
        for (const auto& [target, metadata] : it->second) {
            results.push_back({Edge{target, type, ""}, metadata});  
        }
    }
    return results;
}

    void addObserver(const std::shared_ptr<RelationshipObserver>& observer) {
        observers.push_back(observer);
    }

    void removeObserver(const std::shared_ptr<RelationshipObserver>& observer) {
        observers.erase(
            std::remove_if(observers.begin(), observers.end(),
                [&observer](const std::weak_ptr<RelationshipObserver>& weak) {
                    auto shared = weak.lock();
                    return !shared || shared == observer;
                }),
            observers.end()
        );
    }

    std::vector<std::shared_ptr<DAGNode>> 
    findRelatedConcepts(const std::string& concept, double minConfidence = 0.5);
    
    std::vector<std::shared_ptr<DAGNode>> 
    trackMethodologyFlow(const std::string& startPoint);
    
    std::vector<std::pair<std::shared_ptr<DAGNode>, double>> 
    findSupportingEvidence(const std::string& claim);
    
    double calculateRelationshipStrength(const std::shared_ptr<DAGNode>& other) const;
    std::vector<PathInfo> findAllPaths(const std::shared_ptr<DAGNode>& target) const;

    const std::unordered_map<EdgeType, 
        std::unordered_map<std::shared_ptr<DAGNode>, RelationshipMetadata>>& 
    getRelationships() const { return relationships; }

private:
    std::unordered_map<EdgeType, 
    std::unordered_map<std::shared_ptr<DAGNode>, RelationshipMetadata>> relationships;
    
    std::vector<std::weak_ptr<RelationshipObserver>> observers;

    void notifyObservers(EdgeEvent event, const Edge& edge, 
                        const RelationshipMetadata& metadata) {
        for (auto it = observers.begin(); it != observers.end();) {
            if (auto observer = it->lock()) {
                observer->onRelationshipChanged(event, edge, metadata);
                ++it;
            } else {
                it = observers.erase(it);
            }
        }
    }
};


class DAGNode : public std::enable_shared_from_this<DAGNode> {
protected:
    DAGNode(const std::string& id, NodeType type);
    
public:
    static std::shared_ptr<DAGNode> create(const std::string& id, NodeType type);

    std::string getId() const { return id; }
    NodeType getType() const { return nodeType; }
    std::string getContent() const { return content; }
    std::weak_ptr<ASTNode> getASTNode() const;
    
    void addEdge(const std::shared_ptr<DAGNode>& target, EdgeType type, 
                 const std::string& label = "");
    const std::vector<Edge>& getOutgoingEdges() const { return outgoingEdges; }
    const std::vector<Edge>& getIncomingEdges() const { return incomingEdges; }
    
    void setContent(const std::string& content) { this->content = content; }
    void setASTNode(const std::shared_ptr<ASTNode>& node) { astNode = node; }

    void addChild(const std::shared_ptr<DAGNode>& child);
    void addParent(const std::shared_ptr<DAGNode>& parent);
    const std::vector<std::shared_ptr<DAGNode>>& getChildren() const;
    const std::vector<std::shared_ptr<DAGNode>>& getParents() const { return parents; }
    NodeType getNodeType() const;

    std::vector<std::shared_ptr<DAGNode>> 
    findSemanticallySimilarNodes(double similarityThreshold = 0.7) const;
    
    std::vector<std::shared_ptr<DAGNode>> findContributingEvidence() const;
    std::vector<std::shared_ptr<DAGNode>> findMethodologyChain() const;

    const RelationshipManager& getRelationshipManager() const { 
        return relationshipManager; 
    }

    std::shared_ptr<SemanticInfo> getSemanticInfo() const { return semanticInfo; }
    std::shared_ptr<RelationshipMetadata> getRelationshipMetadata(
        const std::shared_ptr<DAGNode>& target, EdgeType type) const;

private:
    std::string id;
    NodeType nodeType;
    std::string content;
    std::vector<std::shared_ptr<DAGNode>> children;
    std::vector<std::shared_ptr<DAGNode>> parents;
    
    std::vector<Edge> outgoingEdges;
    std::vector<Edge> incomingEdges;
    std::weak_ptr<ASTNode> astNode;
    
    RelationshipManager relationshipManager;
    std::shared_ptr<SemanticInfo> semanticInfo;
    std::vector<std::string> annotations;
    
    bool validateEdge(const std::shared_ptr<DAGNode>& target, EdgeType type) const;
    bool wouldCreateCycle(const std::shared_ptr<DAGNode>& target) const;
    double calculateSemanticSimilarity(const std::shared_ptr<DAGNode>& other) const;
    
    static std::atomic<size_t> nodeCounter;

};

class DAG {
public:
    std::shared_ptr<DAGNode> getOrCreateNode(const std::string& id, NodeType type);
    std::shared_ptr<DAGNode> createNode(NodeType type, const std::string& content = "");
    std::shared_ptr<DAGNode> getNode(const std::string& id) const;
    void addNode(const std::shared_ptr<DAGNode>& node);
    
    void buildFromAST(const std::shared_ptr<ASTNode>& root);
    
    void generateDOT(const std::string& filename) const;
    void exportToKnowledgeGraph(const std::string& filename) const;
    void generateSemanticMap(const std::string& filename) const;
    void generateMethodologyFlow(const std::string& filename) const;

    struct PaperStructureAnalysis {
        std::vector<std::shared_ptr<DAGNode>> mainContributions;
        std::vector<std::shared_ptr<DAGNode>> supportingEvidence;
        std::vector<std::shared_ptr<DAGNode>> methodologySteps;
        std::vector<std::shared_ptr<DAGNode>> criticalAssumptions;
        std::map<std::string, double> topicDistribution;
    };

    struct CitationAnalysis {
        std::vector<std::pair<std::string, int>> citationCounts;
        std::vector<std::string> mostInfluentialPapers;
        std::map<std::string, std::vector<std::string>> citationContexts;
        std::map<int, std::vector<std::string>> citationsByYear;
    };

    PaperStructureAnalysis analyzePaperStructure() const;
    CitationAnalysis analyzeCitations() const;
    std::vector<std::string> identifyResearchGaps() const;
    std::vector<std::string> findContradictions() const;
    
    std::vector<std::vector<std::shared_ptr<DAGNode>>> findStronglyConnectedComponents() const;
    std::map<std::shared_ptr<DAGNode>, double> calculateNodeCentrality() const;
    std::vector<std::shared_ptr<DAGNode>> findBridgingConcepts() const;
    std::vector<std::shared_ptr<DAGNode>> findNodesByType(NodeType type) const;
    std::vector<std::shared_ptr<DAGNode>> findConnectedNodes(
        const std::string& nodeId, EdgeType type) const;
    
    std::vector<std::pair<std::string, double>> extractKeyThemes() const;
    std::map<std::string, std::vector<std::string>> buildConceptHierarchy() const;
    std::vector<std::pair<std::shared_ptr<DAGNode>, double>> rankNodesByImportance() const;
    
    bool validate() const;
    bool validateMethodologyCompleteness() const;
    bool validateEvidenceChain() const;
    std::vector<std::string> identifyLogicalGaps() const;
    
    void dumpStructure() const;
    size_t getNodeCount() const { return nodes.size(); }
    size_t getEdgeCount() const;
    std::string getNodeTypeName(NodeType type) const;



private:
    std::unordered_map<std::string, std::shared_ptr<DAGNode>> nodes;
    
    void processASTNode(const std::shared_ptr<ASTNode>& astNode, const std::shared_ptr<DAGNode>& parentDagNode);
    void processSpecialRelationships(const std::shared_ptr<ASTNode>& astNode, const std::shared_ptr<DAGNode>& dagNode);
    std::string generateSubgraph(const std::vector<std::shared_ptr<DAGNode>>& nodes,const std::string& name) const;
    std::string getEdgeStyle(EdgeType type) const;
    bool validateNode(const std::shared_ptr<DAGNode>& node) const;
    void dumpNode(const std::shared_ptr<DAGNode>& node, int depth = 0) const;
    
    double calculateSemanticSimilarity(const std::shared_ptr<DAGNode>& node1, const std::shared_ptr<DAGNode>& node2) const;
    std::vector<PathInfo> findAllPaths(const std::shared_ptr<DAGNode>& source, const std::shared_ptr<DAGNode>& target) const;
    bool validateRelationshipChain(const std::vector<std::shared_ptr<DAGNode>>& chain, EdgeType relationType) const;

    bool isMethodologyComponent(const std::shared_ptr<DAGNode>& node) const;
    bool isSemanticEdgeType(EdgeType type) const;
    std::string getSemanticEdgeStyle(EdgeType type) const;
};

#endif 
