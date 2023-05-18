#ifndef CVC5__THEORY__QUANTIFIERS__TERM_TUPLE_ENUMERATOR_H
#define CVC5__THEORY__QUANTIFIERS__TERM_TUPLE_ENUMERATOR_H

#include <random>
#include <vector>

#include "expr/node.h"
#include "smt/smt_statistics_registry.h"
#include "theory/quantifiers/featurize.h"
#include "theory/quantifiers/index_trie.h"
#include "theory/quantifiers/ml.h"
#include "theory/quantifiers/term_registry.h"
#include "util/statistics_stats.h"

namespace cvc5 {
namespace theory {
namespace quantifiers {

class TermPools;
class QuantifiersState;
class TermDb;
class RelevantDomain;

/** A general interface for producing a sequence of terms for each quantified
 * variable.*/
class ITermProducer
{
 public:
  virtual ~ITermProducer() = default;
  /** Set up terms for given variable.  */
  virtual size_t prepareTerms(size_t variableIx) = 0;
  /** Get a given term for a given variable.  */
  virtual Node getTerm(size_t variableIx,
                       size_t term_index) CVC5_WARN_UNUSED_RESULT = 0;
  virtual Node getTermOriginal(size_t variableIx, size_t term_index)
  {
    return getTerm(variableIx, term_index);
  }
  virtual void initialize() = 0;
};

/**  Interface for enumeration of tuples of terms.
 *
 * The interface should be used as follows. Firstly, init is called, then,
 * repeatedly, verify if there are any combinations left by calling hasNext
 * and obtaining the next combination by calling next.
 *
 *  Optionally, if the  most recent combination is determined to be undesirable
 * (for whatever reason), the method failureReason is used to indicate which
 *  positions of the tuple are responsible for the said failure.
 */
class TermTupleEnumeratorInterface
{
 public:
  /** Initialize the enumerator. */
  virtual void init() = 0;
  /** Test if there are any more combinations. */
  virtual bool hasNext() = 0;
  /** Obtain the next combination, meaningful only if hasNext Returns true. */
  virtual void next(/*out*/ std::vector<Node>& terms) = 0;
  /** Record which of the terms obtained by the last call of next should not be
   * explored again. */
  virtual void failureReason(const std::vector<bool>& mask) = 0;
  virtual ~TermTupleEnumeratorInterface() = default;
};

struct QuantifierInfo
{
  size_t d_currentPhase = 0;
  std::vector<std::map<Node, TermCandidateInfo> > d_candidateInfos;
};

struct TermTupleEnumeratorGlobal
{
  TermTupleEnumeratorGlobal();
  virtual ~TermTupleEnumeratorGlobal() {}
  TermRegistry* d_treg;
  PredictorInterface* d_ml;
  PredictorInterface* d_tuplePredictor;

  TimerStat d_learningTimer, d_mlTimer, d_featurizeTimer;
  IntStat d_learningCounter;
  std::mt19937 d_mt;
};

/** A struct bundling up parameters for term tuple enumerator.*/
struct TermTupleEnumeratorEnv
{
  /* QuantifiersEngine* d_quantEngine; */
  RelevantDomain* d_rd = nullptr;
  /**
   * Whether we should put full effort into finding an instantiation. If this
   * is false, then we allow for incompleteness, e.g. the tuple enumerator
   * may heuristically give up before it has generated all tuples.
   */
  bool d_fullEffort;
  /** Whether we increase tuples based on sum instead of max (see below) */
  bool d_increaseSum;
  /**term producer to be used to generate the individual terms*/
  ITermProducer* d_termProducer = nullptr;
};

/**  A function to construct a tuple enumerator.
 *
 * In the methods below, we support the enumerators based on the following idea.
 * The tuples are represented as tuples of
 * indices of  terms, where the tuple has as many elements as there are
 * quantified variables in the considered quantifier q.
 *
 * Like so, we see a tuple as a number, where the digits may have different
 * ranges. The most significant digits are stored first.
 *
 * Tuples are enumerated in a lexicographic order in stages. There are 2
 * possible strategies, either all tuples in a given stage have the same sum of
 * digits, or, the maximum over these digits is the same (controlled by
 * TermTupleEnumeratorEnv::d_increaseSum).
 *
 * In this method, the returned enumerator draws ground terms from the term
 * database (provided by td). The quantifiers state (qs) is used to eliminate
 * duplicates modulo equality.
 */
/** Make term pool enumerator */
TermTupleEnumeratorInterface* mkTermTupleEnumeratorPool(
    Node q,
    TermTupleEnumeratorGlobal* global,
    TermTupleEnumeratorEnv* env,
    TermPools* tp,
    Node p);
TermTupleEnumeratorInterface* mkStagedTermTupleEnumerator(
    Node q,
    TermTupleEnumeratorGlobal* global,
    const TermTupleEnumeratorEnv* env);

/** Make term pool enumerator */
ITermProducer* mkPoolTermProducer(Node quantifier, TermPools* tp, Node pool);

ITermProducer* mkTermProducer(Node quantifier,
                              QuantifiersState& qs,
                              TermDb* td);

ITermProducer* mkTermProducerRd(Node quantifier, RelevantDomain* rd);
ITermProducer* mkTermProducerRandomize(ITermProducer* producer,
                                       std::mt19937* mt);

class TermTupleEnumeratorBase : public TermTupleEnumeratorInterface
{
 public:
  /** Initialize the class with the quantifier to be instantiated. */
  TermTupleEnumeratorBase(Node quantifier,
                          TermTupleEnumeratorGlobal* global,
                          const TermTupleEnumeratorEnv* env)
      : d_quantifier(quantifier),
        d_variableCount(d_quantifier[0].getNumChildren()),
        d_global(global),
        d_env(env),
        d_stepCounter(0),
        d_disabledCombinations(
            true)  // do not record combinations with no blanks
  {
    Assert(d_env && d_env->d_termProducer);
    d_changePrefix = d_variableCount;
  }

  virtual ~TermTupleEnumeratorBase() = default;

  // implementation of the TermTupleEnumeratorInterface
  virtual void init() override;
  virtual bool hasNext() override;
  virtual void next(/*out*/ std::vector<Node>& terms) override;
  virtual void failureReason(const std::vector<bool>& mask) override;
  // end of implementation of the TermTupleEnumeratorInterface

 protected:
  /** the quantifier whose variables are being instantiated */
  const Node d_quantifier;
  /** number of variables in the quantifier */
  const size_t d_variableCount;
  /** env of structures with a longer lifespan */
  TermTupleEnumeratorGlobal* const d_global;
  /** env of structures with a longer lifespan */
  const TermTupleEnumeratorEnv* const d_env;
  /** number of candidate terms for each variable */
  std::vector<size_t> d_termsSizes;
  /** tuple of indices of the current terms */
  std::vector<size_t> d_termIndex;
  /** total number of steps of the enumerator */
  uint32_t d_stepCounter;

  /** a data structure storing disabled combinations of terms */
  IndexTrie d_disabledCombinations;

  /**becomes false once the enumerator runs out of options*/
  bool d_hasNext;
  /** the length of the prefix that has to be changed in the next
  combination, i.e.  the number of the most significant digits that need to be
  changed in order to escape a  useless instantiation */
  size_t d_changePrefix;
  virtual bool nextCombinationAttempt() = 0;
  virtual void initializeAttempts() = 0;
  /** Move on in the current stage */
  bool nextCombination();
};

}  // namespace quantifiers
}  // namespace theory
}  // namespace cvc5
#endif /* TERM_TUPLE_ENUMERATOR_H_7640 */
