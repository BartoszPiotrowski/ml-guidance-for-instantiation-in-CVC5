#include "theory/quantifiers/quantifier_logger.h"

#include <cmath>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <unordered_set>
#include <utility>

#include "base/map_util.h"
#include "expr/skolem_manager.h"
#include "options/quantifiers_options.h"
#include "theory/quantifiers/featurize.h"
#include "theory/quantifiers/quantifiers_attributes.h"
#include "theory/quantifiers/term_tuple_enumerator_utils.h"
#include "theory/quantifiers/term_util.h"

namespace cvc5 {
namespace theory {
namespace quantifiers {

#define ASSERT_EXISTENCE(termsInfo, term, varIx, quantifier)      \
  Assert(ContainsKey(termsInfo[varIx], term))                     \
      << "missing info about: '" << term << "' for var " << varIx \
      << " in quant '" << quantifier << "'" << std::endl;

static Cvc5ostream& operator<<(
    Cvc5ostream& out, const QuantifierLogger::InstantiationExplanation& t)
{
  return out << "[" << t.d_quantifier << " <- " << t.d_instantiation << "]";
}

static std::ostream& printSample(std::ostream& out,
                                 int label,
                                 const FeatureVector& features)

{
  out << label;
  /* Assert(features.isFull()); */
  const auto& values = features.values();
  for (size_t i = 0; i < values.size(); i++)
  {
    const auto value = values[i];
    if (FP_ZERO != std::fpclassify(value))
    {
      out << " " << i << ":" << value;
    }
  }
  return out << std::endl;
}

QuantifierLogger QuantifierLogger::s_logger;

QuantifierLogger::QuantifierInfo& QuantifierLogger::getQuantifierInfo(
    Node quantifier)
{
  auto [it, wasInserted] = d_infos.insert({quantifier, QuantifierInfo()});
  auto& qi = it->second;
  if (wasInserted)
  {
    qi.d_infos.resize(quantifier[0].getNumChildren());
  }
  Assert(qi.d_infos.size() == quantifier[0].getNumChildren());
  return qi;
}

size_t QuantifierLogger::getCurrentPhase(Node quantifier) const
{
  const auto i = d_infos.find(quantifier);
  return i != d_infos.end() ? i->second.d_currentPhase : 0;
}

void QuantifierLogger::increasePhase(Node quantifier)
{
  if (!ContainsKey(d_infos, quantifier))
  {
    d_infos[quantifier].d_infos.resize(quantifier[0].getNumChildren());
    d_infos[quantifier].d_currentPhase = 1;
  }
  else
  {
    d_infos[quantifier].d_currentPhase++;
  }
}

void QuantifierLogger::registerUsefulInstantiation(
    const InstantiationList& instantiations)
{
  auto& qi = getQuantifierInfo(instantiations.d_quant);
  for (const auto& instantiation : instantiations.d_inst)
  {
    NodeVector v(instantiation);
    Assert(ContainsKey(qi.d_allInstantiations, v));
    qi.d_usefulInstantiations.insert(v);
  }
}

void QuantifierLogger::registerInstantiation(Node quantifier,
                                             bool successful,
                                             const NodeVector& instantiation)
{
  Assert(!successful || quantifier == d_currentInstantiationQuantifier);
  auto& qi = getQuantifierInfo(quantifier);
  Assert(ContainsKey(qi.d_allInstantiations, instantiation));
  if (successful)
  {
    d_instantiationBodies.push_back(
        {quantifier, instantiation, d_currentInstantiationBody});
    qi.d_successfulInstantiations.insert(instantiation);
    d_currentInstantiationBody = Node::null();
    d_currentInstantiationQuantifier = Node::null();
  }
  else
  {
    qi.d_rejectedInstantiations.insert(instantiation);
  }
}

QuantifierLogger::NodeVector QuantifierLogger::registerInstantiationAttempt(
    Node quantifier, const std::vector<Node>& instantiation)
{
  NodeVector v(instantiation);
  getQuantifierInfo(quantifier).d_allInstantiations.insert(v);
  for (size_t vx = instantiation.size(); vx--;)
  {
    registerTryCandidate(quantifier, vx, instantiation[vx]);
  }
  return v;
}

void QuantifierLogger::registerCurrentInstantiationBody(Node quantifier,
                                                        Node body)
{
  Assert(d_currentInstantiationBody.isNull()
         && d_currentInstantiationQuantifier.isNull());
  d_currentInstantiationQuantifier = quantifier;
  d_currentInstantiationBody = body;
}
bool QuantifierLogger::registerCandidate(Node quantifier,
                                         size_t varIx,
                                         Node candidate,
                                         bool relevant)
{
  auto& qi = getQuantifierInfo(quantifier);
  auto& candidates = qi.d_infos[varIx];
  /* auto [it, wasInserted] = candidates.emplace({candidate,
   * TermCandidateInfo()}); */
  auto [it, wasInserted] = candidates.try_emplace(candidate);
  if (wasInserted)
  {
    TermCandidateInfo& info = it->second;
    info.d_age = candidates.size();
    info.d_phase = qi.d_currentPhase;
    info.d_relevant = relevant;
    info.d_initialized = true;
  }
  return wasInserted;
}

void QuantifierLogger::registerTryCandidate(Node quantifier,
                                            size_t varIx,
                                            Node candidate)
{
  auto& qi = getQuantifierInfo(quantifier);
  auto& vinfos = qi.d_infos;
  Assert(vinfos.size() == quantifier[0].getNumChildren());
  auto& vinfo = qi.d_infos[varIx];

  ASSERT_EXISTENCE(vinfos, candidate, varIx, quantifier);
  /* if (!ContainsKey(vinfo, candidate)) */
  /* { */
  /*   vinfo[candidate]; */
  /* } */

  auto& cinfo = vinfo.at(candidate);
  Assert(cinfo.d_initialized);
  cinfo.d_tried++;
}

static std::ostream& printTupleSet(std::ostream& out,
                                   const QuantifierLogger::TupleSet& tuples)
{
  for (const auto& ns : tuples)
  {
    out << "    ( ";
    std::copy(ns.begin(), ns.end(), std::ostream_iterator<Node>(out, " "));
    out << ")" << std::endl;
  }
  return out;
}
std::ostream& QuantifierLogger::printTupleSample(
    std::ostream& out,
    Node quantifier,
    const QuantifierLogger::NodeVector& instantiation,
    FeatureVector& featureVector,
    const Featurize& quantifierFeatures,
    bool isUseful,
    std::vector<std::map<Node, TermCandidateInfo>> termsInfo)
{
  out << "; " << isUseful << " : " << instantiation << std::endl;
  Assert(instantiation.size() == termsInfo.size());
  featureVector.push();
  // featurize each term separately
  for (size_t varIx = instantiation.size(); varIx--;)
  {
    const Node& term = instantiation[varIx];
    ASSERT_EXISTENCE(termsInfo, term, varIx, quantifier);
    const auto& candidateInfo = termsInfo[varIx].at(term);
    featurizeTerm(
        &featureVector, term, varIx, candidateInfo, quantifierFeatures);
  }
  printSample(out, isUseful, featureVector);
  featureVector.pop();
  return out;
}

std::ostream& QuantifierLogger::printTupleSamplesQuantifier(
    std::ostream& out, Node quantifier, const QuantifierInfo& info)
{
  const auto& allInstantiations = info.d_allInstantiations;
  const auto& termsInfo = info.d_infos;
  const auto& usefulInstantiations = info.d_usefulInstantiations;
  // calculate features for quantifier just once
  Featurize quantifierFeatures(true);
  quantifierFeatures.count(quantifier);
  FeatureVector featureVector(&TermTupleFeatureProperties::s_features);
  featurizeQuantifier(&featureVector, quantifierFeatures);
  out << "; Q : " << quantifier << std::endl;
  // featurize all the tuples
  for (const auto& instantiation : allInstantiations)
  {
    const bool useful =
        ContainsKey(usefulInstantiations, NodeVector(instantiation));
    printTupleSample(out,
                     quantifier,
                     instantiation,
                     featureVector,
                     quantifierFeatures,
                     useful,
                     termsInfo);
  }
  return out;
}

std::ostream& QuantifierLogger::printTupleSamples(std::ostream& out)
{
  out << "; TUPLE SAMPLES" << std::endl;
  size_t maxSize = 0;
  for (const auto& entry : d_infos)  // go through all quantifiers
  {
    printTupleSamplesQuantifier(out, entry.first, entry.second);
    maxSize = std::max(maxSize, entry.second.d_infos.size());
  }
  out << "; FEATURE_NAMES up to " << maxSize << " vars" << std::endl;
  const auto& properties = TermTupleFeatureProperties::s_features;
  const auto& names = properties.names();
  for (size_t index = 0; index < properties.size(maxSize); index++)
  {
    out << " " << index << ":" << names[index];
  }

  return out << std::endl;
}

bool isFromInstantiation(Node n)
{
  return !n.hasAttribute(SkolemFormAttribute())
         && (!n.hasAttribute(InstLevelAttribute())
             || n.getAttribute(InstLevelAttribute()) > 0);
  /* return !n.hasAttribute(InstLevelAttribute()) */
  /*        && n.getAttribute(InstLevelAttribute()) > 0; */
}

void QuantifierLogger::transitiveExplanation()
{
  Trace("ml") << "transitive explanation" << std::endl;
  std::unordered_set<TNode, TNodeHashFunction> needsExplaining;
  for (const auto& entry : d_infos)
    for (const auto& i : entry.second.d_usefulInstantiations)
      for (const auto& n : i)
        if (isFromInstantiation(n)) needsExplaining.insert(n);

  Trace("ml") << "Needs explanation:" << needsExplaining << std::endl;
  size_t lastExplainedSize = 0;
  while (!needsExplaining.empty()
         && needsExplaining.size() != lastExplainedSize)
  {
    lastExplainedSize = needsExplaining.size();
    for (const auto& [quantifier, instantiation, body] : d_instantiationBodies)
      findExplanation(
          {quantifier, instantiation}, body, needsExplaining, d_reasons);
  }
  if (!needsExplaining.empty())
    Trace("ml") << "Warning, some things remained unexplained" << std::endl;
  // join instantiations from the proof and reasons
  for (auto [n, e] : d_reasons)
  {
    d_infos.at(e.d_quantifier).d_usefulInstantiations.insert(e.d_instantiation);
  }
  Trace("ml") << "Done explaining" << std::endl;
}

void QuantifierLogger::findExplanation(
    InstantiationExplanation reason,
    Node body,
    std::unordered_set<TNode, TNodeHashFunction>& needsExplaining,
    std::map<Node, InstantiationExplanation>& reasons)
{
  std::vector<TNode> todo({body});
  bool explainsSomething = false;
  while (!needsExplaining.empty() && !todo.empty())
  {
    auto top(todo.back());
    todo.pop_back();
    if (!isFromInstantiation(top) || top.getKind() == Kind::FORALL) continue;
    if (needsExplaining.erase(top) > 0)
    {
      Trace("ml") << top << " explained by " << reason << std::endl;
      const auto it = reasons.insert({top, reason});
      if (it.second) explainsSomething = true;
    }
    todo.insert(todo.end(), top.begin(), top.end());
  }
  // mark the reason as something to be explained
  if (explainsSomething)
  {
    for (auto& n : reason.d_instantiation)
      if (isFromInstantiation(n) && !ContainsKey(reasons, n))
        needsExplaining.insert(n);
  }
}

std::ostream& QuantifierLogger::printTermSamples(std::ostream& out)
{
  out << "; SAMPLES" << std::endl;
  for (const auto& entry : d_infos)  // going through all quantifiers
  {
    const auto& quantifier = entry.first;
    const auto& usefulInstantiations = entry.second.d_usefulInstantiations;
    const auto& infos = entry.second.d_infos;
    const auto variableCount = infos.size();
    // calculate features for quantifier just once
    Featurize quantifierFeatures(true);
    quantifierFeatures.count(quantifier);
    FeatureVector featureVector(&TermFeatureProperties::s_features);
    featurizeQuantifier(&featureVector, quantifierFeatures);

    // for each variable calculate if a term was ever useful
    std::vector<std::set<Node>> usefulPerVariable(variableCount);
    for (const auto& instantiation : usefulInstantiations)
    {
      Assert(instantiation.size() == variableCount);
      for (size_t varIx = 0; varIx < variableCount; varIx++)
      {
        usefulPerVariable[varIx].insert(instantiation[varIx]);
      }
    }
    // go through all the variables and terms
    for (size_t varIx = 0; varIx < variableCount; varIx++)
    {
      for (const auto& term_index : infos[varIx])
      {
        const Node& term = term_index.first;
        const auto& candidateInfo = term_index.second;
        if (candidateInfo.d_tried == 0)
        {  // ignoring any terms that were never tried
          continue;
        }

        const auto useful = ContainsKey(usefulPerVariable[varIx], term) ? 1 : 0;
        featureVector.push();
        featurizeTerm(
            &(featureVector), term, varIx, candidateInfo, quantifierFeatures);
        printSample(out, useful, featureVector);
        featureVector.pop();
      }
    }
  }

  out << "; FEATURE_NAMES";
  const auto& names = TermFeatureProperties::s_features.names();
  for (size_t index = 0; index < names.size(); index++)
  {
    out << " " << index << ":" << names[index];
  }
  return out << std::endl;
}
std::ostream& QuantifierLogger::printExtensive(std::ostream& out)
{
  std::set<Node> useful_terms;
  std::set<Node> all_candidates;
  out << "(quantifier_candidates " << std::endl;
  for (const auto& entry : d_infos)
  {
    if (entry.second.d_allInstantiations.empty()) continue;
    const auto& quantifier = entry.first;
    const auto& usefulInstantiations = entry.second.d_usefulInstantiations;
    const auto& successfulInstantiations =
        entry.second.d_successfulInstantiations;
    const auto& rejectedInstantiations = entry.second.d_rejectedInstantiations;
    const auto name = quantifier;
    const auto& infos = entry.second.d_infos;
    const auto variableCount = infos.size();

    // count how many times a term was ever useful for each variable
    std::vector<std::map<Node, int32_t>> usefulPerVariable(variableCount);
    for (const auto& instantiation : usefulInstantiations)
    {
      Assert(instantiation.size() == variableCount);
      for (size_t varIx = 0; varIx < variableCount; varIx++)
      {
        const auto& term = instantiation[varIx];
        auto [it, wasInserted] = usefulPerVariable[varIx].insert({term, 1});
        if (!wasInserted)
        {
          it->second++;
        }
      }
    }

    out << "(candidates " << name << " " << std::endl;
    for (size_t varIx = 0; varIx < variableCount; varIx++)
    {
      out << "  (variable " << varIx;
      for (const auto& term_index : infos[varIx])
      {
        const Node& term = term_index.first;
        const auto& candidateInfo = term_index.second;
        if (candidateInfo.d_tried == 0)
        {
          continue;
        }

        all_candidates.insert(term);
        const auto i = usefulPerVariable[varIx].find(term);
        const auto termUseful =
            (i == usefulPerVariable[varIx].end()) ? 0 : i->second;
        out << " (candidate " << term;
        out << " (age " << candidateInfo.d_age << ")";
        out << " (phase " << candidateInfo.d_phase << ")";
        out << " (relevant " << candidateInfo.d_relevant << ")";
        out << " (depth " << quantifiers::TermUtil::getTermDepth(term) << ")";
        out << " (tried " << candidateInfo.d_tried << ")";
        out << " (useful " << termUseful << ")";
        out << ")";  // close candidate
      }
      out << "  )" << std::endl;  // close variable
    }

    printTupleSet(out << "  (rejected_instantiations " << std::endl,
                  rejectedInstantiations)
        << "  ) " << std::endl;
    printTupleSet(out << "  (successful_instantiations " << std::endl,
                  successfulInstantiations)
        << "  ) " << std::endl;
    printTupleSet(out << "  (useful_instantiations " << std::endl,
                  usefulInstantiations)
        << "  )" << std::endl;
    out << ")" << std::endl;  // close candidates
  }
  return out << ")" << std::endl;  //  close everything
}

std::ostream& QuantifierLogger::print(std::ostream& out)
{
  if (options::mlParents()) transitiveExplanation();
  printExtensive(out);
  printTermSamples(out);
  printTupleSamples(out);
  clear();
  return out;
}
}  // namespace quantifiers
}  // namespace theory
}  // namespace cvc5
