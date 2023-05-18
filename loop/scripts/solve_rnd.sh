#! /bin/env bash

err=${0%.*}.err
{
ulimit -t $3
if [ $# -eq 4 ]
then
    lgb_options="--lightGBModel=$4"
fi
solver="../CVC5/build/bin/cvc5"
problem=$1
log=$2
default_options=" --full-saturate-quant --fs-sum --no-e-matching --no-cegqi --no-quant-cf "
# --e-matching --fs-interleave
logging_options=" --qlogging --dump-instantiations --print-inst-full --produce-proofs "
1>&2 echo LGB  --fs-rnd-probability=0.1 --fs-rnd-seed=$RANDOM
$solver --stats-expert \
    $default_options \
    $logging_options \
     --fs-rnd-probability=0.1 --fs-rnd-seed=$RANDOM \
    $problem \
    &> $log
} 2>> $err
