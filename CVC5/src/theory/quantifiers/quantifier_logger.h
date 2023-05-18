#ifndef QUANTIFIER_LOGGER_H_15196
#define QUANTIFIER_LOGGER_H_15196
#include <fstream>
#include <map>
#include <set>
#include <unordered_map>

#include "base/map_util.h"
#include "theory/quantifiers/featurize.h"
#include "theory/quantifiers/inst_match_trie.h"
#include "theory/quantifiers/instantiation_list.h"
#include "theory/quantifiers/term_tuple_enumerator_utils.h"
#include "theory/quantifiers_engine.h"

namespace cvc5 {
namespace theory {
namespace quantifiers {
class QuantifierLogger
{
 public:
  static QuantifierLogger s_logger;  // TODO: get rid of singleton
 public:
  typedef ImmutableVector<Node, NodeHashFunction> NodeVector;
  typedef std::unordered_set<NodeVector,
                             ImmutableVector_hash<Node, NodeHashFunction>,
                             ImmutableVector_equal<Node, NodeHashFunction>>
      TupleSet;

  struct QuantifierInfo
  {
    std::vector<std::map<Node, TermCandidateInfo>> d_infos;
    TupleSet d_usefulInstantiations;
    TupleSet d_successfulInstantiations;
    TupleSet d_rejectedInstantiations;
    TupleSet d_allInstantiations;
    size_t d_currentPhase = 0;
  };

  struct InstantiationExplanation
  {
    Node d_quantifier;
    NodeVector d_instantiation;
  };
  struct InstantiationInfo
  {
    Node d_quantifier;
    NodeVector d_instantiation;
    Node d_body;
  };

  std::ostream& print(std::ostream& out);

  Node d_currentInstantiationBody;
  Node d_currentInstantiationQuantifier;
  void registerCurrentInstantiationBody(Node quantifier, Node body);
  bool registerCandidate(Node quantifier,
                         size_t varIx,
                         Node candidate,
                         bool relevant);

  NodeVector registerInstantiationAttempt(Node quantifier,
                                          const std::vector<Node>& inst);

  void registerInstantiation(Node quantifier,
                             bool successful,
                             const NodeVector& inst);

  void registerUsefulInstantiation(const InstantiationList& instantiations);
  /* void registerInstantiations(Node quantifier, QuantifiersEngine*); */

  size_t getCurrentPhase(Node quantifier) const;
  void increasePhase(Node quantifier);

  QuantifierInfo& getQuantifierInfo(Node quantifier);

  bool hasQuantifier(Node quantifier) const
  {
    return ContainsKey(d_infos, quantifier);
  }

  virtual ~QuantifierLogger() { clear(); }

 protected:
  std::map<Node, QuantifierInfo> d_infos;
  std::vector<InstantiationInfo> d_instantiationBodies;
  std::map<Node, InstantiationExplanation> d_reasons;

  QuantifierLogger() {}
  void registerTryCandidate(Node quantifier, size_t varIx, Node candidate);

  void clear()
  {
    // std::cout << "clearing logger\n";
    d_infos.clear();
    d_instantiationBodies.clear();
    d_reasons.clear();
  }
  void transitiveExplanation();
  void findExplanation(
      InstantiationExplanation reason,
      Node body,
      std::unordered_set<TNode, TNodeHashFunction>& needsExplaining,
      std::map<Node, InstantiationExplanation>& reasons);

  std::ostream& printExtensive(std::ostream& out);
  std::ostream& printTermSamples(std::ostream& out);
  std::ostream& printTupleSamples(std::ostream& out);
  std::ostream& printTupleSamplesQuantifier(std::ostream& out,
                                            Node quantifier,
                                            const QuantifierInfo& info);
  std::ostream& printTupleSample(
      std::ostream& out,
      Node quantifier,
      const QuantifierLogger::NodeVector& instantiation,
      FeatureVector& featureVector,
      const Featurize& quantifierFeatures,
      bool isUseful,
      std::vector<std::map<Node, TermCandidateInfo>> termsInfo);
};

}  // namespace quantifiers
}  // namespace theory
}  // namespace cvc5

#endif /* QUANTIFIER_LOGGER_H_15196 */
