#include "crf_model.h"
#include <cmath>
#include <iostream>
#include <limits>
#include <algorithm>

CRFModel::CRFModel() {
}

std::string CRFModel::featKey(Label lbl, const std::string& feat) const {
    return std::to_string((int)lbl) + "_" + feat;
}

double CRFModel::scoreTransition(Label prev, Label curr) const {
    return (prev == curr) ? 0.1 : 0.0;
}

double CRFModel::scoreFeatures(Label lbl, const std::vector<std::string>& feats) const {
    double score = 0.0;
    for (auto& f : feats) {
        auto key = featKey(lbl, f);
        auto it = featureWeights.find(key);
        if (it != featureWeights.end()) {
            score += it->second;
        }
    }
    return score;
}

void CRFModel::forwardBackward(const std::vector<std::vector<std::string>>& sequenceFeatures,
                               std::vector<std::vector<double>>& alpha,
                               std::vector<std::vector<double>>& beta,
                               double &logZ) const {
    size_t T = sequenceFeatures.size();
    size_t L = labelSet.size();

    alpha.assign(T, std::vector<double>(L, -std::numeric_limits<double>::infinity()));
    for (size_t l = 0; l < L; l++) {
        double s = scoreFeatures(labelSet[l], sequenceFeatures[0]);
        alpha[0][l] = s;
    }

    for (size_t t = 1; t < T; t++) {
        for (size_t l = 0; l < L; l++) {
            double s = scoreFeatures(labelSet[l], sequenceFeatures[t]);
            double maxVal = -std::numeric_limits<double>::infinity();
            for (size_t pl = 0; pl < L; pl++) {
                double val = alpha[t-1][pl] + scoreTransition(labelSet[pl], labelSet[l]) + s;
                if (val > maxVal) maxVal = val;
            }
            double sumExp = 0.0;
            for (size_t pl = 0; pl < L; pl++) {
                double val = alpha[t-1][pl] + scoreTransition(labelSet[pl], labelSet[l]) + s;
                sumExp += std::exp(val - maxVal);
            }
            alpha[t][l] = maxVal + std::log(sumExp);
        }
    }

    beta.assign(T, std::vector<double>(L, -std::numeric_limits<double>::infinity()));
    for (size_t l = 0; l < L; l++) {
        beta[T-1][l] = 0.0;
    }

    // Backward pass (log-sum-exp)
    for (int t = (int)T-2; t >= 0; t--) {
        for (size_t l = 0; l < L; l++) {
            double maxVal = -std::numeric_limits<double>::infinity();
            for (size_t nl = 0; nl < L; nl++) {
                double s = scoreFeatures(labelSet[nl], sequenceFeatures[t+1]) +
                           scoreTransition(labelSet[l], labelSet[nl]) +
                           beta[t+1][nl];
                if (s > maxVal) maxVal = s;
            }
            double sumExp = 0.0;
            for (size_t nl = 0; nl < L; nl++) {
                double s = scoreFeatures(labelSet[nl], sequenceFeatures[t+1]) +
                           scoreTransition(labelSet[l], labelSet[nl]) +
                           beta[t+1][nl];
                sumExp += std::exp(s - maxVal);
            }
            beta[t][l] = maxVal + std::log(sumExp);
        }
    }

    double maxVal = -std::numeric_limits<double>::infinity();
    for (size_t l = 0; l < L; l++) {
        if (alpha[T-1][l] > maxVal) maxVal = alpha[T-1][l];
    }
    double sumExp = 0.0;
    for (size_t l = 0; l < L; l++) {
        sumExp += std::exp(alpha[T-1][l] - maxVal);
    }
    logZ = maxVal + std::log(sumExp);
}

double CRFModel::computeGradients(const std::vector<std::vector<std::string>>& sequenceFeatures,
                                  const std::vector<Label>& goldLabels,
                                  std::unordered_map<std::string,double>& grad) const {
    size_t T = sequenceFeatures.size();
    size_t L = labelSet.size();

    std::vector<std::vector<double>> alpha, beta;
    double logZ;
    forwardBackward(sequenceFeatures, alpha, beta, logZ);

    // Gold score
    double goldScore = scoreFeatures(goldLabels[0], sequenceFeatures[0]);
    for (size_t t = 1; t < T; t++) {
        goldScore += scoreFeatures(goldLabels[t], sequenceFeatures[t]);
        goldScore += scoreTransition(goldLabels[t-1], goldLabels[t]);
    }

    for (size_t t = 0; t < T; t++) {
        for (auto& f : sequenceFeatures[t]) {
            grad[featKey(goldLabels[t], f)] += 1.0;
        }
    }

    for (size_t t = 0; t < T; t++) {
        double maxVal = -std::numeric_limits<double>::infinity();
        for (size_t l = 0; l < L; l++) {
            if (alpha[t][l] + beta[t][l] > maxVal) maxVal = alpha[t][l] + beta[t][l];
        }
        double sumExp = 0.0;
        std::vector<double> posteriors(L,0.0);
        for (size_t l = 0; l < L; l++) {
            double val = alpha[t][l] + beta[t][l];
            double e = std::exp(val - maxVal);
            sumExp += e;
            posteriors[l] = e;
        }
        for (size_t l = 0; l < L; l++) {
            double posterior = posteriors[l] / sumExp;
            for (auto& f : sequenceFeatures[t]) {
                grad[featKey(labelSet[l], f)] -= posterior;
            }
        }
    }

    // Negative log-likelihood
    double loss = logZ - goldScore; 
    return loss;
}

void CRFModel::sgdUpdate(const std::unordered_map<std::string,double>& grad, double lr) {
    for (auto& g : grad) {
        featureWeights[g.first] += (lr * g.second); 
    }
}

void CRFModel::train(const std::vector<std::vector<std::vector<std::string>>>& allFeatures,
                     const std::vector<std::vector<Label>>& allLabels,
                     int iterations, double lr) {
    for (int iter = 0; iter < iterations; iter++) {
        double totalLoss = 0.0;
        for (size_t i = 0; i < allFeatures.size(); i++) {
            std::unordered_map<std::string,double> grad;
            double loss = computeGradients(allFeatures[i], allLabels[i], grad);
            totalLoss += loss;
            // Update weights: descending gradient => w = w - lr * grad
            // computeGradients gives observed - expected directly subtract lr*grad
            sgdUpdate(grad, -lr); 
        }
        std::cout << "Iteration " << iter << ": Avg Loss = " << (totalLoss/allFeatures.size()) << std::endl;
    }
}

std::vector<CRFModel::Label> CRFModel::predict(const std::vector<std::vector<std::string>>& sequenceFeatures) const {
    size_t T = sequenceFeatures.size();
    if (T == 0) return {};

    size_t L = labelSet.size();
    // Viterbi
    std::vector<std::vector<double>> viterbi(L, std::vector<double>(T, -std::numeric_limits<double>::infinity()));
    std::vector<std::vector<int>> backpointers(L, std::vector<int>(T, -1));

    for (size_t l = 0; l < L; l++) {
        viterbi[l][0] = scoreFeatures(labelSet[l], sequenceFeatures[0]);
    }

    for (size_t t = 1; t < T; t++) {
        for (size_t l = 0; l < L; l++) {
            double s = scoreFeatures(labelSet[l], sequenceFeatures[t]);
            double bestScore = -std::numeric_limits<double>::infinity();
            int bestLabel = -1;
            for (size_t pl = 0; pl < L; pl++) {
                double val = viterbi[pl][t-1] + scoreTransition(labelSet[pl], labelSet[l]) + s;
                if (val > bestScore) {
                    bestScore = val;
                    bestLabel = (int)pl;
                }
            }
            viterbi[l][t] = bestScore;
            backpointers[l][t] = bestLabel;
        }
    }

    double bestFinal = -std::numeric_limits<double>::infinity();
    int bestFinalLabel = -1;
    for (size_t l = 0; l < L; l++) {
        if (viterbi[l][T-1] > bestFinal) {
            bestFinal = viterbi[l][T-1];
            bestFinalLabel = (int)l;
        }
    }

    // Backtrack
    std::vector<Label> result(T);
    int curr = bestFinalLabel;
    for (int t = (int)T-1; t >= 0; --t) {
        result[t] = labelSet[curr];
        curr = backpointers[curr][t];
    }
    return result;
}

