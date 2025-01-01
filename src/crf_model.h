#ifndef CRF_MODEL_H
#define CRF_MODEL_H

#include <string>
#include <vector>
#include <map>
#include <unordered_map>

class CRFModel {
public:
    enum Label { O=0, AUTHOR=1, AFFILIATION=2 };

    CRFModel();

    void train(const std::vector<std::vector<std::vector<std::string>>>& allFeatures,
               const std::vector<std::vector<Label>>& allLabels,
               int iterations = 10, double lr = 0.1);

    std::vector<Label> predict(const std::vector<std::vector<std::string>>& sequenceFeatures) const;

    static std::string labelToString(Label lbl) {
        switch (lbl) {
            case AUTHOR: return "AUTHOR";
            case AFFILIATION: return "AFFILIATION";
            default: return "O";
        }
    }

private:
    std::vector<Label> labelSet = {O, AUTHOR, AFFILIATION};

    std::unordered_map<std::string, double> featureWeights;

    double computeGradients(const std::vector<std::vector<std::string>>& sequenceFeatures, const std::vector<Label>& goldLabels, std::unordered_map<std::string,double>& grad) const;

    void forwardBackward(const std::vector<std::vector<std::string>>& sequenceFeatures, std::vector<std::vector<double>>& alpha, std::vector<std::vector<double>>& beta, double &logZ) const;

    double scoreTransition(Label prev, Label curr) const;
    double scoreFeatures(Label lbl, const std::vector<std::string>& feats) const;
    
    void sgdUpdate(const std::unordered_map<std::string,double>& grad, double lr);

    std::string featKey(Label lbl, const std::string& feat) const;
};

#endif

