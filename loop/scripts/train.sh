#! /bin/env bash

dir=`dirname $0`
training_examples=$1
model=$2
config=$3
lightgbm=../LightGBM/lightgbm
./$lightgbm \
    config=$config \
    train_data=$training_examples \
    output_model=$model

