python3 -u scripts/loop.py \
    --training_problems data/UFLIA-sledgehammer-filtered-problems-train \
    --testing_problems data/UFLIA-sledgehammer-filtered-problems-test \
    --proving_script scripts/solve_rnd.sh \
    --n_jobs 20 \
    --data_dir ${0%.*}.data \
    --solving_time_limit 120 \
    | tee ${0%.*}.$RANDOM.log
