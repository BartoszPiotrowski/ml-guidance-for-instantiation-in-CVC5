#! /bin/env python3

import os, sys, argparse, subprocess, uuid, re
import lightgbm as lgb
from utils import mkdir_if_not_exists, read_lines, write_lines
from joblib import Parallel, delayed
from tqdm import tqdm
from sklearn.datasets import load_svmlight_file
from sklearn.metrics import roc_auc_score
from itertools import product, chain, combinations
from random import shuffle

def grid_search(training_examples, args):
    training_examples = list(training_examples)
    n = int(0.25 * len(training_examples))
    grid_train_examples = training_examples[n:]
    grid_test_examples = training_examples[:n]
    grid_train_examples_path = args.data_dir + '/grid_train'
    grid_test_examples_path = args.data_dir + '/grid_test'
    write_lines(grid_test_examples, grid_test_examples_path)
    write_lines(grid_train_examples, grid_train_examples_path)
    train_X, train_y = load_svmlight_file(grid_train_examples_path)
    test_X, test_y = load_svmlight_file(grid_test_examples_path)
    params_basic = {
        'objective': 'binary',
        'boosting': 'gbdt',
        'verbose': -1,
        'n_jobs': args.n_jobs,
    }
    params_grid = {
        #'max_depth': [5, 10, 15],
        'eta':        [0.01, 0.05, 0.1],
        'num_trees':  [10, 50, 100],
        'num_leaves': [8, 32, 256],
        'max_bin':    [8, 32, 256],
    }
    params_grid_names = list(params_grid)
    params_grid_vals = [params_grid[n] for n in params_grid_names]
    params_grid_vals_combs = list(product(*params_grid_vals))
    params_grid_combs = [dict(zip(params_grid_names, c)) for c in \
                               params_grid_vals_combs]
    params_combs = [dict(**params_basic, **g) for g in params_grid_combs]
    best_params = None
    best_score = 0
    for params in params_combs:
        train_Xy = lgb.Dataset(train_X, label=train_y)
        model = lgb.train(params, train_Xy)
                          #valid_sets = [train_Xy], verbose_eval=10)
        test_preds = model.predict(test_X)
        test_score = roc_auc_score(test_y, test_preds)
        print(' '.join([f'{n}: {params[n]}' for n in params]))
        print(test_score)
        if test_score > best_score:
            best_score = test_score
            best_params = params
    return best_params

def prove(problems_file, args, lgb_model='', lgb_model_tuples=''):
    problems = read_lines(problems_file)
    logs_dir = os.path.join(args.data_dir, 'proof_logs')
    mkdir_if_not_exists(logs_dir)
    if lgb_model and lgb_model_tuples:
        print(f'Solving: Using lgb models: {lgb_model} {lgb_model_tuples}')
    print(    f'Solving: Logs will be saved to {logs_dir}')
    with Parallel(n_jobs=args.n_jobs) as parallel:
        prove_one_d = delayed(prove_one)
        logs = parallel(
            prove_one_d(problem, logs_dir, args.proving_script,
                        lgb_model, lgb_model_tuples)
                      for problem in tqdm(problems))
    solved = [p for p in logs if p]
    problems_solved, logs_solved = zip(*solved)
    return problems_solved, logs_solved

def prove_one(input_filename, dir_path, proving_script,
              lgb_model, lgb_model_tuples):
    uuid4 = uuid.uuid4().hex
    output_filename = os.path.join(dir_path, uuid4 + '.log')
    run_prover(input_filename, output_filename, proving_script,
               lgb_model, lgb_model_tuples)
    if "unsat" in read_lines(output_filename):
        return input_filename, output_filename
    else:
        return None

def run_prover(input_filename, output_filename, proving_script,
               lgb_model, lgb_model_tuples):
    os.popen(
        f'{proving_script} {input_filename} {output_filename} '
        f'{args.solving_time_limit} {lgb_model} {lgb_model_tuples}').read()

def train(training_examples, args, params=None, suffix=''):
    if params:
        print('Using: ' + ' '.join([f'{n}: {params[n]}' for n in params]))
        training_config = args.data_dir + '/training.config'
        config_lines = [p + '=' + str(params[p]) for p in params]
        write_lines(config_lines, training_config)
    else:
        training_config = args.training_config
        print('Using: ', training_config)
    training_examples_path = args.data_dir + '/training_examples'
    model = args.data_dir + '/model' + suffix
    write_lines(training_examples, training_examples_path)
    print(os.popen(f'{args.training_script} {training_examples_path} '
                   f'{model} {training_config}').read())
    return model

def name(log_file):
    with open(log_file) as f:
        log_lines = f.read().splitlines()
    for l in log_lines:
        if 'filename' in l:
            n = l.split(' = ')[1]
            return n
    return None

def n_inst(log_file):
    with open(log_file) as f:
        log_lines = f.read().splitlines()
    for l in log_lines:
        if 'Instantiations_Total' in l:
            n = int(l.split(' = ')[1])
            return n
    return None

def plot_inst(log_files_1, log_files_2, args, plot_name):
    if not log_files_1 or not log_files_2:
        return None
    insts_1, insts_2 = {}, {}
    for f in log_files_1:
        n, i = name(f), n_inst(f)
        if n and i:
            insts_1[n] = i
    for f in log_files_2:
        n, i = name(f), n_inst(f)
        if n and i:
            insts_2[n] = i
    insts_1_2 = [','.join([str(insts_1[n]), str(insts_2[n])]) \
                 for n in set(insts_1) & set(insts_2)]
    path = args.data_dir + '/' + plot_name + '.csv'
    write_lines(insts_1_2, path)

def extract_training_examples(log_files, neg_pos_ratio, tuples):
    examples = set()
    for f in log_files:
        examples.update(extract_training_examples_1(f, tuples))
    examples_pos = [e for e in examples if e[0] == '1']
    examples_neg = [e for e in examples if e[0] == '0']
    n = neg_pos_ratio * len(examples_pos)
    shuffle(examples_neg)
    examples_neg = examples_neg[:n]
    examples = set(examples_neg + examples_pos)
    return examples

def extract_training_examples_1(log_file, tuples):
    lines = read_lines(log_file)
    if lines and ('; TUPLE SAMPLES' in lines):
        if tuples :
            n = lines.index('; TUPLE SAMPLES')
            lines = lines[n:]
        else:
            n = lines.index('; TUPLE SAMPLES')
            lines = lines[:n]
    lines = [l for l in lines if re.search('^[01] [0-9]+:.+', l)]
    return set(lines)


def loop_tt(args):
    print('Solving/training loop on training/testing split.')
    mkdir_if_not_exists(args.data_dir)
    print(f'Directory for data produced during the run: {args.data_dir}')
    proof_logs_train_all = set()
    proof_logs_test_all = set()
    proof_logs_train_init = set()
    proof_logs_test_init = set()
    solved_training_problems_all = set()
    solved_testing_problems_all = set()
    training_examples = set()
    training_examples_tuples = set()
    model=''
    model_tuples=''
    for i in range(args.iterations):
        print(f'### Loop iteration no. {i + 1} ###')
        solved_training_problems, proof_logs_train = \
                prove(args.training_problems, args, model, model_tuples)
        solved_testing_problems, proof_logs_test = \
                prove(args.testing_problems, args, model, model_tuples)
        training_examples.update(
            extract_training_examples(
                proof_logs_train, args.neg_pos_ratio, False))
        training_examples_tuples.update(
            extract_training_examples(
                proof_logs_train, args.neg_pos_ratio, True))
        if not i:
            proof_logs_test_init = proof_logs_test
            proof_logs_train_init = proof_logs_train
        plot_inst(proof_logs_train_init, proof_logs_train, args,
                  'insts_train_' + str(i+1))
        plot_inst(proof_logs_test_init, proof_logs_test, args,
                  'insts_test_' + str(i+1))
        proof_logs_train_all.update(proof_logs_train)
        proof_logs_test_all.update(proof_logs_test)
        solved_training_problems_all.update(solved_training_problems)
        solved_testing_problems_all.update(solved_testing_problems)
        ns_inst_train = [n_inst(l) for l in proof_logs_train if n_inst(l) != None]
        avg_n_inst_train = sum(ns_inst_train) / len(ns_inst_train)
        ns_inst_test = [n_inst(l) for l in proof_logs_test if n_inst(l) != None]
        avg_n_inst_test = sum(ns_inst_test) / len(ns_inst_test)
        print(f'Number of problems solved now in train: {len(proof_logs_train)}')
        print(f'Number of all problems solved in train: '
              f'{len(solved_training_problems_all)}')
        print(f'Average number of instantiations in train: {avg_n_inst_train}')
        print(f'Number of problems solved now in test: {len(proof_logs_test)}')
        print(f'Number of all problems solved in test: '
              f'{len(solved_testing_problems_all)}')
        print(f'Average number of instantiations in test: {avg_n_inst_test}')
        print(f'Number of training examples: {len(training_examples)}')
        params = grid_search(training_examples, args)
        params_tuples = grid_search(training_examples_tuples, args)
        model = train(
            training_examples, args, params=params,
            suffix= '_' + str(i + 1))
        model_tuples = train(
            training_examples_tuples, args, params=params_tuples,
            suffix= '_tuples_' + str(i + 1))

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--training_problems',
        type=str)
    parser.add_argument(
        '--testing_problems',
        type=str,
        default=None)
    parser.add_argument(
        '--proving_script',
        type=str)
    parser.add_argument(
        '--training_script',
        type=str)
    parser.add_argument(
        '--training_config',
        type=str)
    parser.add_argument(
        '--data_dir',
        type=str)
    parser.add_argument(
        '--n_jobs',
        default=10,
        type=int)
    parser.add_argument(
        '--iterations',
        default=16,
        type=int)
    parser.add_argument(
        '--solving_time_limit',
        default=10,
        type=int)
    parser.add_argument(
        '--neg_pos_ratio',
        default=10,
        type=int)
    parser.add_argument(
        '--plot_insts_script',
        type=str,
        default='scripts/loop/plot_insts.R')
    args = parser.parse_args()
    if args.testing_problems:
        loop_tt(args)
    else:
        loop(args)
