python3 -u scripts/loop.py \
    --training_problems data/UFNIA-sledgehammer-filtered-problems-train \
    --testing_problems data/UFNIA-sledgehammer-filtered-problems-test \
    --proving_script scripts/solve.sh \
    --training_script scripts/train.sh \
    --n_jobs 20 \
    --data_dir ${0%.*}.data \
    --solving_time_limit 120 \
    | tee ${0%.*}.$RANDOM.log
