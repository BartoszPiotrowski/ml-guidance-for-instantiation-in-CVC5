#include "theory/quantifiers/featurize.h"

#include <sstream>

#include "theory/quantifiers/term_util.h"

namespace cvc5 {
namespace theory {
namespace quantifiers {

void featurizeQuantifier(/*out*/ FeatureVector* dest,
                         const Featurize& quantifierFeatures)
{
  quantifierFeatures.addBOWToVector(*dest);
}

void featurizeTerm(/*out*/ FeatureVector* dest,
                   const Node term,
                   size_t variableIx,
                   const TermCandidateInfo& termInfo,
                   const Featurize& quantifierFeatures)
{  // add features for the term
  Featurize termFeatures(false);
  termFeatures.count(term);
  termFeatures.addBOWToVector(*dest);
  dest->addValue(quantifierFeatures.getVariableFrequency(variableIx));
  dest->addValue(termInfo.d_age);
  dest->addValue(termInfo.d_phase);
  dest->addValue(termInfo.d_relevant);
  dest->addValue(TermUtil::getTermDepth(term));
  dest->addValue(termInfo.d_tried);
}
}  // namespace quantifiers
}  // namespace theory
TermFeatureProperties TermFeatureProperties::s_features;
TermTupleFeatureProperties TermTupleFeatureProperties::s_features;
void FeaturePropertiesBase::addKinds(const char* prefix)
{
  for (int i = kind::NULL_EXPR; i < kind::LAST_KIND; i++)
  {
    const auto k = static_cast<Kind>(i);
    std::stringstream ss;
    ss << prefix << k;
    addName(ss.str());
  }
}
TermFeatureProperties::TermFeatureProperties()
{
  addKinds("Q_");           //  BOW for the encompassing quantifier
  addKinds("T_");           //  BOW for the candidate term
  addName("varFrequency");  //  The number of occurrences of the variable in
  addName("age");           //  The age of the candidate term
  addName("phase");         //  The phase of the candidate term
  addName("relevant");      //  The relevancy of the candidate term
  addName("depth");         //  The depth of the candidate term
  addName("tried");  //  The number of times the candidate term has been tried
                     //  the  quantifier
}
TermTupleFeatureProperties::TermTupleFeatureProperties()
{
  addKinds("Q_");  //  BOW for the encompassing quantifier
  d_variableSizes.push_back(d_names.size());
  for (size_t varIx = 0; varIx < 100; varIx++)
  {
    std::string prefix = "T" + std::to_string(varIx) + "_";
    addKinds(prefix.c_str());  //  BOW for the candidate term
    addName(prefix
            + "varFrequency");  //  The number of occurrences of the variable in
    addName(prefix + "age");    //  The age of the candidate term
    addName(prefix + "phase");  //  The phase of the candidate term
    addName(prefix + "relevant");  //  The relevancy of the candidate term
    addName(prefix + "depth");     //  The depth of the candidate term
    addName(
        prefix
        + "tried");  //  The number of times the candidate term has been tried
    //  the  quantifier
    d_variableSizes.push_back(d_names.size());
  }
}
FeatureVector::FeatureVector(const FeaturePropertiesBase* featureProperties)
    : d_featureProperties(featureProperties)
{
  d_values.reserve(d_featureProperties->count());
}
Cvc5ostream& operator<<(Cvc5ostream& out, const FeatureVector& features)
{
  const auto names = features.d_featureProperties->names();
  const auto values = features.values();
  for (size_t i = 0; i < values.size(); i++)
  {
    const auto value = values[i];
    if (FP_ZERO != std::fpclassify(value))
    {
      out << (i ? " " : "") << names[i] << "(" << i << "):" << value;
    }
  }
  return out;
}

void FeatureVector::pop()
{
  Assert(!d_markers.empty());
  while (d_values.size() > d_markers.back())
  {
    d_values.pop_back();
  }
  d_markers.pop_back();
}
Featurize::Featurize(bool trackBoundVariables)
    : d_trackBoundVariables(trackBoundVariables)
{
}
void Featurize::count(TNode n)
{
  Trace("featurize") << "[featurize] featurize: " << n << std::endl;
  if (d_trackBoundVariables && n.getKind() == Kind::FORALL)
  {
    Assert(d_quantifier.isNull());
    d_quantifier = n;
    Trace("featurize") << "[featurize] quantifier: " << n << std::endl;
  }
  std::vector<TNode> todo;
  TNode cur;
  todo.push_back(n);
  do
  {
    cur = todo.back();
    todo.pop_back();
    touch(cur);
    const auto confirmation = d_visited.insert(cur);
    if (!confirmation.second)
    {
      continue;
    }
    visit(cur);
    todo.insert(todo.end(), cur.begin(), cur.end());
  } while (!todo.empty());
  d_quantifier = Node::null();
}

void Featurize::touch(TNode n)
{
  const auto kind = n.getKind();
  if (d_trackBoundVariables && kind == Kind::BOUND_VARIABLE
      && !d_quantifier.isNull())
  {
    Trace("featurize") << "[featurize] touching a bound variable: " << n
                       << std::endl;
    Node::iterator it =
        std::find(d_quantifier[0].begin(), d_quantifier[0].end(), n);
    if (it != d_quantifier[0].end())
    {
      const size_t i = it - d_quantifier[0].begin();
      if (d_boundFrequencies.size() <= i)
      {
        d_boundFrequencies.resize(i + 1, 0);
      }
      d_boundFrequencies[i]++;
    }
  }
}
void Featurize::visit(TNode n) { increaseFeature(n.getKind()); }
void Featurize::addBOWToVector(FeatureVector& vec) const
{
  for (int i = kind::NULL_EXPR; i < kind::LAST_KIND; i++)
  {
    const auto k = static_cast<Kind>(i);
    const auto frequency = getFrequency(k);
    vec.addValue(frequency);
  }
}
}  // namespace cvc5
