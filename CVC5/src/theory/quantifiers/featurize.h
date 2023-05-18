#ifndef THEORY_QUANTIFIERS_FEATURIZE_H_9495
#define THEORY_QUANTIFIERS_FEATURIZE_H_9495
#include <cmath>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/check.h"
#include "expr/node.h"
namespace cvc5 {
/**\brief Information we store about each term that appears as a candidate for
 * qualifier instantiation.**/
struct TermCandidateInfo
{
  bool d_initialized = false;
  size_t d_age = -1, d_phase = -1;
  bool d_relevant;
  size_t d_tried = 0;
  static TermCandidateInfo mk(size_t age, size_t phase, bool relevant)
  {
    return TermCandidateInfo{true, age, phase, relevant, 0};
  }
};

/**\brief Maintaining the set of features we are using.*/
class FeaturePropertiesBase
{
 public:
  const std::vector<std::string>& names() const { return d_names; }
  size_t count() const { return d_names.size(); }

 protected:
  std::vector<std::string> d_names;
  FeaturePropertiesBase() {}
  virtual ~FeaturePropertiesBase(){};
  void addName(const std::string& name) { d_names.push_back(name); }
  void addKinds(const char* prefix);
};
/**\brief Maintaining the set of features we are using for a single term.*/
class TermFeatureProperties : public FeaturePropertiesBase
{
 public:
  static TermFeatureProperties s_features;

 private:
  TermFeatureProperties();
  virtual ~TermFeatureProperties(){};
};
/**\brief Maintaining the set of features we are using for a tuples of terms.*/
class TermTupleFeatureProperties : public FeaturePropertiesBase
{
 public:
  static TermTupleFeatureProperties s_features;
  size_t size(size_t variableCount) const
  {
    return d_variableSizes[variableCount];
  }

 private:
  std::vector<size_t> d_variableSizes;
  TermTupleFeatureProperties();
  virtual ~TermTupleFeatureProperties(){};
};
/**\brief  A feature vector, which should respect features set up in
 * FeatureProperties::s_features.*/
class FeatureVector
{
 public:
  const FeaturePropertiesBase* d_featureProperties;
  FeatureVector(const FeaturePropertiesBase* featureProperties);
  virtual ~FeatureVector() = default;
  bool isFull() const
  {
    return d_featureProperties->count() == d_values.size();
  }
  void addValue(float value)
  {
    Assert(!isFull());
    d_values.push_back(value);
  }
  /** When called, the next pop will return to the state.*/
  void push() { d_markers.push_back(d_values.size()); }
  /**Return to the state marked by last push.*/ void pop();
  const std::vector<float>& values() const { return d_values; }
  const float* rawValues() const { return d_values.data(); }

 private:
  std::vector<float> d_values;
  std::vector<size_t> d_markers;
};

Cvc5ostream& operator<<(Cvc5ostream& out, const FeatureVector& features);

/**\brief  A class  used to featurize.
 *
 * Currently we support back of words (BOW).
 * To calculate BOW, run count and then either obtain each frequency bite
 * getFrequency or run addBOWToVector
 * to add all the features to a feature vector.*/
class Featurize
{
 public:
  Featurize(bool trackBoundVariables);
  virtual ~Featurize() = default;
  void addBOWToVector(FeatureVector& vec) const;
  void count(TNode n);
  int getFrequency(Kind feature) const
  {
    return feature >= 0 && static_cast<size_t>(feature) < d_frequencies.size()
               ? d_frequencies[feature]
               : 0;
  }
  int getVariableFrequency(size_t variableIndex) const
  {
    Assert(d_trackBoundVariables);
    return variableIndex < d_boundFrequencies.size()
               ? d_boundFrequencies[variableIndex]
               : 0;
  }

 private:
  const bool d_trackBoundVariables;
  std::vector<int> d_frequencies, d_boundFrequencies;
  std::set<Node> d_visited;
  Node d_quantifier;  // current quantifier being visited, we are assuming we
                      // cannot visit more than one at a time
  void visit(TNode n);
  void touch(TNode n);
  int increaseFeature(int id)
  {
    AlwaysAssert(id >= 0);
    Assert(id >= 0);
    const auto index = static_cast<size_t>(id);
    if (index >= d_frequencies.size())
    {
      d_frequencies.resize(index + 1, 0);
    }
    return ++d_frequencies[index];
  }
};
namespace theory {
namespace quantifiers {

void featurizeQuantifier(/*out*/ FeatureVector* dest,
                         const Featurize& quantifierFeatures);
void featurizeTerm(/*out*/ FeatureVector* dest,
                   const Node term,
                   size_t variableIx,
                   const TermCandidateInfo& termInfo,
                   const Featurize& quantifierFeatures);

}  // namespace quantifiers
}  // namespace theory
}  // namespace cvc5
#endif /* THEORY_QUANTIFIERS_FEATURIZE_H_9495 */
