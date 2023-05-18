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
1>&2 echo LGB $lgb_options
$solver --stats-expert \
    $default_options \
    $logging_options \
    $lgb_options \
    $problem \
    &> $log
} 2>> $err
