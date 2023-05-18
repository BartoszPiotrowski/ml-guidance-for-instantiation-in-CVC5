#include "theory/quantifiers/term_tuple_enumerator_ml.h"

#include <algorithm>
#include <unordered_set>

#include "base/map_util.h"
#include "base/output.h"
#include "options/quantifiers_options.h"
#include "smt/smt_statistics_registry.h"
#include "theory/quantifiers/featurize.h"
#include "theory/quantifiers/index_trie.h"
#include "theory/quantifiers/quant_module.h"
#include "theory/quantifiers/quantifier_logger.h"
#include "theory/quantifiers/relevant_domain.h"
#include "theory/quantifiers/term_registry.h"
#include "theory/quantifiers/term_tuple_enumerator_utils.h"
#include "theory/quantifiers/term_util.h"
#include "theory/quantifiers_engine.h"
#include "util/statistics_registry.h"
#include "util/statistics_stats.h"
namespace cvc5 {
namespace theory {
namespace quantifiers {
MLProducer::MLProducer(TermTupleEnumeratorGlobal* global,
                       const TermTupleEnumeratorEnv* env,
                       ITermProducer* producer,
                       Node quantifier)
    : d_global(global),
      d_env(env),
      d_producer(producer),
      d_quantifier(quantifier),
      d_permutations(d_quantifier[0].getNumChildren()),
      d_predictions(d_quantifier[0].getNumChildren())
{
}

size_t MLProducer::prepareTerms(size_t variableIndex)
{
  Assert(variableIndex < d_permutations.size());
  // prepare terms of the producer being decorated
  const auto termCount = d_producer->prepareTerms(variableIndex);
  // set of the permutation for this variable, initially as identity
  auto& permutation = d_permutations[variableIndex];
  permutation.resize(termCount, 0);
  std::iota(permutation.begin(), permutation.end(), 0);
  return termCount;
}

void MLProducer::runPrediction()
{
  TimerStat::CodeTimer codeTimer(d_global->d_learningTimer);
  // set of features for quantifier
  Featurize quantifierFeatures(true);
  {
    TimerStat::CodeTimer codeTimer1(d_global->d_featurizeTimer);
    quantifierFeatures.count(d_quantifier);
  }

  //  feature vector or that we are using for all terms
  FeatureVector features(&TermFeatureProperties::s_features);
  Trace("inst-alg-rd") << "setting up quantifier features" << std::endl;
  AlwaysAssert(d_global->d_ml->numberOfFeatures()
               == TermFeatureProperties::s_features.count())
      << "expecting " << TermFeatureProperties::s_features.count()
      << " features but the model has " << d_global->d_ml->numberOfFeatures()
      << std::endl;
  // featurize  current quantifier
  featurizeQuantifier(&features, quantifierFeatures);

  // run prediction for individual variables
  const auto variableCount(d_quantifier[0].getNumChildren());
  for (size_t variableIx = 0; variableIx < variableCount; variableIx++)
  {
    runPrediction(quantifierFeatures, features, variableIx);
  }
}

void MLProducer::runPrediction(const Featurize& quantifierFeatures,
                               FeatureVector& features,
                               size_t variableIx)
{
  auto& permutation = d_permutations[variableIx];
  const auto termCount = permutation.size();

  if (d_global->d_ml == nullptr || termCount == 0)
  {
    return;
  }
  Assert(QuantifierLogger::s_logger.hasQuantifier(d_quantifier))
      << "Missing info about  quantifier" << std::endl;
  const auto& qinfo =
      QuantifierLogger::s_logger.getQuantifierInfo(d_quantifier);
  ++d_global->d_learningCounter;
  Trace("inst-alg-rd") << "Predicting terms for var" << variableIx << std::endl;

  const auto& tsinfo = qinfo.d_infos[variableIx];
  auto& predictions = d_predictions[variableIx];
  predictions.resize(termCount);

  for (size_t termIx = 0; termIx < termCount; termIx++)
  {
    // add features for the term
    const auto term = d_producer->getTerm(variableIx, termIx);
    features.push();
    {
      const auto termInfo = tsinfo.at(term);
      TimerStat::CodeTimer codeTimer1(d_global->d_featurizeTimer);
      featurizeTerm(&features, term, variableIx, termInfo, quantifierFeatures);
    }
    Assert(features.isFull());

    {  // run prediction
      TimerStat::CodeTimer predictTimer(d_global->d_mlTimer);
      const auto prediction = d_global->d_ml->predict(features.rawValues());
      predictions[termIx] = options::mlThreshold.wasSetByUser()
                                ? (prediction > options::mlThreshold() ? 1 : 0)
                                : prediction;
    }

    Trace("inst-alg-rd") << term << " features : " << features << std::endl;
    Trace("inst-alg-rd") << "Prediction " << term << " : "
                         << predictions[termIx] << std::endl;
    features.pop();  // remove current term from the feature vector
  }

  // create a permutation that corresponds to sorting the terms by the predicted
  // score, assuming that the permutation was initially the identity
  Assert(permutation.size() == termCount);
  std::stable_sort(permutation.begin(),
                   permutation.end(),
                   [predictions](size_t a, size_t b) {
                     return predictions[a] > predictions[b];
                   });
  Trace("inst-alg-rd") << "Learned order : [";
  for (size_t i = 0; i < permutation.size(); i++)
  {
    Trace("inst-alg-rd") << (i ? ", " : "")
                         << d_producer->getTerm(variableIx, permutation[i])
                         << "@" << predictions[permutation[i]];
  }
  Trace("inst-alg-rd") << "]" << std::endl;
}

MLProducer* mkTermProducerML(TermTupleEnumeratorGlobal* global,
                             const TermTupleEnumeratorEnv* env,
                             ITermProducer* producer,
                             Node quantifier)
{
  return new MLProducer(global, env, producer, quantifier);
}

class AStarTupleEnumerator : public TermTupleEnumeratorBase
{
 public:
  AStarTupleEnumerator(Node quantifier,
                       TermTupleEnumeratorGlobal* global,
                       const TermTupleEnumeratorEnv* env,
                       MLProducer* termProducer)
      : TermTupleEnumeratorBase(quantifier, global, env),
        d_predictions(termProducer)
  {
    Assert(d_predictions == env->d_termProducer);
  }
  virtual ~AStarTupleEnumerator() = default;
  /*implementation of virtual methods from ancestor*/
  virtual void initializeAttempts() override;
  virtual bool nextCombinationAttempt() override;

  typedef ImmutableVector<size_t> Tuple;
  struct ScoredTuple
  {
    Tuple d_tuple;
    float d_score;
  };
  struct ScoredTupleCompare
  {
    bool operator()(const ScoredTuple& a, const ScoredTuple& b) const
    {
      return a.d_score < b.d_score;
    }
  };

 protected:
  static ScoredTupleCompare s_compare;
  typedef std::vector<ScoredTuple> Heap;
  MLProducer* d_predictions;
  std::unordered_set<Tuple,
                     ImmutableVector_hash<size_t>,
                     ImmutableVector_equal<size_t>>
      d_visited;
  Heap d_open;
  void push(const std::vector<size_t>& tuple);
  float calculateScore(const Tuple& tuple);
};
static Cvc5ostream& operator<<(Cvc5ostream& out,
                               const AStarTupleEnumerator::ScoredTuple& t)
{
  return out << "[" << t.d_tuple << ", " << t.d_score << "]";
}
void AStarTupleEnumerator::initializeAttempts() { push(d_termIndex); }
bool AStarTupleEnumerator::nextCombinationAttempt()
{
  if (d_open.empty())
  {
    return false;
  }
  // pop largest element from the heap (top)
  std::pop_heap(d_open.begin(), d_open.end(), s_compare);
  ScoredTuple top = d_open.back();
  d_open.pop_back();
  Trace("inst-alg-rd") << "[A*] Pop " << top << std::endl;
  // push top's neighbors
  d_termIndex.clear();
  d_termIndex.insert(d_termIndex.end(), top.d_tuple.begin(), top.d_tuple.end());
  std::vector<size_t> temporary;
  for (size_t varIx = d_termIndex.size(); varIx--;)
  {
    const auto newValue = d_termIndex[varIx] + 1;
    if (newValue >= d_termsSizes[varIx])
    {
      continue;  // digit cannot be increased
    }
    temporary = d_termIndex;
    temporary[varIx] = newValue;
    push(temporary);
  }
  Trace("inst-alg-rd") << "[A*] Heap size " << d_open.size() << std::endl;
  return true;
}
void AStarTupleEnumerator::push(const std::vector<size_t>& values)
{
  Assert(values.size() == d_variableCount);
  Tuple tuple(values);
  if (!d_visited.insert(tuple).second)
  {
    return;  // already seen
  }
  d_open.resize(d_open.size() + 1);
  auto& newElement = d_open.back();
  newElement.d_tuple = tuple;
  newElement.d_score = calculateScore(tuple);

  Trace("inst-alg-rd") << "[A*] Push " << newElement << std::endl;
  std::push_heap(d_open.begin(), d_open.end(), s_compare);
}
float AStarTupleEnumerator::calculateScore(const Tuple& tuple)
{
  float rv = 1;
  if (d_global->d_tuplePredictor)
  {
    TimerStat::CodeTimer codeTimer(d_global->d_learningTimer);
    // set of features for quantifier
    Featurize quantifierFeatures(true);
    FeatureVector featureVector(&TermTupleFeatureProperties::s_features);
    {
      TimerStat::CodeTimer codeTimer1(d_global->d_featurizeTimer);
      quantifierFeatures.count(d_quantifier);
    }
    Trace("inst-alg-rd") << "setting up quantifier features" << std::endl;
    // featurize current quantifier
    featurizeQuantifier(&featureVector, quantifierFeatures);
    bool oversized = false;
    const auto& qinfo =
        QuantifierLogger::s_logger.getQuantifierInfo(d_quantifier);
    for (size_t varIx = 0; varIx < tuple.size(); varIx++)
    {
      const auto termIx = tuple[varIx];
      oversized |= termIx >= d_termsSizes[varIx];
      Assert(!oversized || (termIx == 0 && d_termsSizes[varIx] == 0));
      if (oversized)  // TODO
      {
        break;
      }
      const Node& term = d_predictions->getTerm(varIx, termIx);
      const auto& candidateInfo = qinfo.d_infos[varIx].at(term);
      featurizeTerm(
          &featureVector, term, varIx, candidateInfo, quantifierFeatures);
    }
    rv = oversized
             ? 1
             : d_global->d_tuplePredictor->predict(featureVector.rawValues());
  }
  else
  {
    for (size_t varIx = 0; varIx < tuple.size(); varIx++)
    {
      const auto termIx = tuple[varIx];
      const bool oversized = termIx >= d_termsSizes[varIx];
      Assert(!oversized || (termIx == 0 && d_termsSizes[varIx] == 0));
      const auto score = oversized ? 1 : d_predictions->predict(varIx, termIx);
      rv *= score;
    }
  }
  return rv;
}
TermTupleEnumeratorInterface* mkAStarTermTupleEnumerator(
    Node q,
    TermTupleEnumeratorGlobal* global,
    const TermTupleEnumeratorEnv* env,
    MLProducer* termProducer)
{
  return new AStarTupleEnumerator(q, global, env, termProducer);
}

}  // namespace quantifiers
}  // namespace theory
}  // namespace cvc5
