#ifndef TERM_TUPLE_ENUMERATOR_ML_H_23055
#define TERM_TUPLE_ENUMERATOR_ML_H_23055
#include "theory/quantifiers/featurize.h"
#include "theory/quantifiers/term_tuple_enumerator.h"
namespace cvc5 {
namespace theory {
namespace quantifiers {
/** \brief Term producer that orders terms according to an ML predictor.
 *  The producer acts as a decorator on top of a given term producer.
 *
 * This is implemented by calculating a permutation of the original
 * term producer.  This permutation is calculated only at the individualization
 * and then used to access the original terms coming from the decorated
 * producer.
 *
 * The ML predictor is assumed to give scores over the individual terms.
 * This score is then used to order the terms, i.e., the scores define the
 * permutation.
 * */
class MLProducer : public ITermProducer
{
 public:
  MLProducer(TermTupleEnumeratorGlobal* global,
             const TermTupleEnumeratorEnv* env,
             ITermProducer* producer,
             Node quantifier);
  virtual ~MLProducer() = default;

  /**  implementation of ITermProducer*/
  virtual size_t prepareTerms(size_t variableIx) override;
  /**  implementation of ITermProducer*/
  virtual Node getTerm(size_t variableIx,
                       size_t term_index) override CVC5_WARN_UNUSED_RESULT
  {
    Assert(d_initialized);
    return d_producer->getTerm(variableIx,
                               d_permutations[variableIx][term_index]);
  }
  /**  implementation of ITermProducer*/
  virtual void initialize() override
  {
    runPrediction();
    d_initialized = true;
  }
  /**  implementation of ITermProducer*/
  virtual Node getTermOriginal(size_t variableIx, size_t term_index) override
  {
    return d_producer->getTermOriginal(variableIx, term_index);
  }

  /** Obtain a calculated prediction score for a given term. */
  double predict(size_t variableIx, size_t termIx) const
  {
    Assert(d_initialized);
    Assert(variableIx < d_permutations.size());
    Assert(d_permutations[variableIx].size() > termIx);
    return d_predictions[variableIx][d_permutations[variableIx][termIx]];
  }

 protected:
  TermTupleEnumeratorGlobal* const d_global;
  const TermTupleEnumeratorEnv* d_env;
  ITermProducer* const d_producer;
  const Node d_quantifier;
  bool d_initialized = false;
  std::vector<std::vector<size_t>> d_permutations;
  std::vector<std::vector<double>> d_predictions;
  void runPrediction(const Featurize& quantifierFeatures,
                     FeatureVector& features,
                     size_t variableIx);
  void runPrediction();
};
/**
 * Create a term producer based on ML prediction.
 */
MLProducer* mkTermProducerML(TermTupleEnumeratorGlobal* global,
                             const TermTupleEnumeratorEnv* env,
                             ITermProducer* producer,
                             Node quantifier);
/**
 * Create a term to the numerator based on a*.
 */
TermTupleEnumeratorInterface* mkAStarTermTupleEnumerator(
    Node q,
    TermTupleEnumeratorGlobal* global,
    const TermTupleEnumeratorEnv* env,
    MLProducer* termProducer);
}  // namespace quantifiers
}  // namespace theory
}  // namespace cvc5
#endif /* TERM_TUPLE_ENUMERATOR_ML_H_23055 */
