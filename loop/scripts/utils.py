import os
import re
import pickle
import gzip
import uuid
import signal
from time import strftime
from glob import glob
from math import log
from sys import getsizeof
from shutil import copyfile, rmtree
from contextlib import contextmanager
from itertools import product


def read(filename):
    with open(filename, encoding='utf-8') as f:
        return f.read()

def read_lines(filename):
    with open(filename, encoding='utf-8') as f:
        return f.read().splitlines()


def write_lines(list_of_lines, filename, backup=False):
    if backup:
        copyfile(filename, filename + '.bcp')
    with open(filename, encoding='utf-8', mode='wt') as f:
        f.write('\n'.join(list_of_lines) + '\n')
    return filename


def write_line(line, filename):
    with open(filename, encoding='utf-8', mode='wt') as f:
        f.write(line + '\n')
    return filename

def empty_file(filename):
    open(filename, 'w').close()
    return filename

def copy_file(source, target):
    if source:
        copyfile(source, target)
    else:
        empty_file(target)
    return target

# TODO isn't the above the same?
def write_empty(filename):
    with open(filename, encoding='utf-8', mode='wt') as f:
        f.write('')
    return filename


def append_lines(list_of_lines, filename):
    with open(filename, encoding='utf-8', mode='a') as f:
        f.write('\n'.join(list_of_lines) + '\n')
    return filename


def append_line(line, filename):
    with open(filename, encoding='utf-8', mode='a') as f:
        f.write(line + '\n')
    return filename


def save_obj(obj, filename, compress=False):
    if not compress:
        with open(filename, 'wb') as f:
            pickle.dump(obj, f, pickle.HIGHEST_PROTOCOL)
    else:
        with gzip.open(filename, 'wb') as f:
            pickle.dump(obj, f, pickle.HIGHEST_PROTOCOL)


def load_obj(filename, compress=False):
    if not compress:
        with open(filename, 'rb') as f:
            return pickle.load(f)
    else:
        with gzip.open(filename, 'rb') as f:
            return pickle.load(f)


def humanbytes(B):
    'Return the given bytes as a human friendly KB, MB, GB, or TB string'
    B = float(B)
    KB = float(1024)
    MB = float(KB ** 2)  # 1,048,576
    GB = float(KB ** 3)  # 1,073,741,824
    TB = float(KB ** 4)  # 1,099,511,627,776
    if B < KB:
        return '{0} {1}'.format(B, 'Bytes' if 0 == B > 1 else 'Byte')
    elif KB <= B < MB:
        return '{0:.2f} KB'.format(B / KB)
    elif MB <= B < GB:
        return '{0:.2f} MB'.format(B / MB)
    elif GB <= B < TB:
        return '{0:.2f} GB'.format(B / GB)
    elif TB <= B:
        return '{0:.2f} TB'.format(B / TB)


def size(obj):
    return humanbytes(getsizeof(obj))


def partition(lst, n):
    '''
    Splits a list into n rougly equal partitions.
    '''
    if n == 0:
        return [lst]
    if n > len(lst):
        n = len(lst)
    division = len(lst) / n
    return [lst[round(division * i):round(division * (i + 1))]
            for i in range(n)]


def mkdir_if_not_exists(dirpath):
    if not os.path.exists(dirpath):
        os.makedirs(dirpath)
    return dirpath


def rmdir_mkdir(dirpath):
    rmtree(dirpath, ignore_errors=True)
    os.makedirs(dirpath)
    return dirpath


def date_time():
    return strftime('%Y%m%d%H%M%S')


def read_deps(path, unions=False):
    deps = {}
    deps_lines = read_lines(path)
    for l in deps_lines:
        thm, ds = l.split(':')
        ds = set(ds.split(' '))
        ds = ds - {''}
        assert thm not in ds, (thm, ds)
        if unions:
            if thm in deps:
                deps[thm].update(ds)
            else:
                deps[thm] = ds
        else:
            if thm in deps:
                deps[thm].append(ds)
            else:
                deps[thm] = [ds]
    return deps


def save_deps(deps, filename):
    write_empty(filename)
    for thm in deps:
        if isinstance(deps[thm], list):
            for ds in deps[thm]:
                append_line(f"{thm}:{' '.join(ds)}", filename)
        elif isinstance(deps[thm], set):
            append_line(f"{thm}:{' '.join(deps[thm])}", filename)
        else:
            print('Error: wrong format of dependencies.')


def read_features(path):
    features_lines = read_lines(path)
    if ':"' in features_lines[0]:
        return read_features_binary(features_lines)
    else:
        return read_features_enigma(features_lines)


def read_features_enigma(features_lines):
    '''
    Assumed format:

    abstractness_v1_cfuncdom:32850:1 32927:1 34169:9
    abstractness_v1_cat_1:32927:1 33139:1 34169:8 36357:2
    ...

    '''
    features = {}
    for l in features_lines:
        t, f = l.split(':', 1)
        f = f.split(' ')
        f = dict([(i.split(':')[0], float(i.split(':')[1])) for i in f])
        features[t] = f
    return features


def read_features_binary(features_lines):
    '''
    Assumed format:

    abstractness_v1_cfuncdom:"fea_1", "fea_2", "fea_3", ...
    abstractness_v1_cat_1:"fea_8", "fea_5", "fea_1", ...
    ...

    '''
    features = {}
    for l in features_lines:
        t, ff = l.split(':')
        ff = ff.split(', ')
        t = t.strip(' "')
        ff = [f.strip(' "') for f in ff]
        ff = set(ff)
        features[t] = ff
    return features


def read_stms(file, short=False, tokens=False):
    '''
    file should contain lines of the form:
        fof(name,type,formula).
    '''
    stms = {}
    for line in read_lines(file):
        line = line.replace(' ', '').replace(',conjecture,', ',axiom,')
        name = line.split('(')[1].split(',')[0]
        assert name not in stms
        if short or tokens:
            line = tokenize(line)
            if short:
                line_list = line.split(' ')
                # formula from fof(name,type,formula).
                line_list = line_list[6:-2]
                # remove redundant brackets
                if line_list[0] == '(':
                    line_list = line_list[1:-1]
                line = ' '.join(line_list)
        stms[name] = line
    return stms


def tokenize(line):
    tptp_conns_orig = ['<=>', '<~>', '=>', '<=', '~|', '~&', '!=', '$t', '$f']
    tptp_conns = [' '.join(conn) for conn in tptp_conns_orig]
    tptp_conns_dict = dict(zip(tptp_conns, tptp_conns_orig))
    line_list = []
    line_token = ''
    for ch in line:
        if ch == ' ':
            if line_token:
                line_list.append(line_token)
                line_token = ''
        elif str.isalnum(ch) or (ch == '_'):
            line_token += ch
        else:
            if line_token:
                line_list.append(line_token)
                line_token = ''
            line_list.append(ch)
    out_str = ' '.join(line_list)
    for conn in tptp_conns:
        if conn in out_str:
            out_str = out_str.replace(conn, tptp_conns_dict[conn])
    return out_str


#def read_stms(path):
#    stms_lines = read_lines(path)
#    names = [l.split(',')[0].split('(')[1].replace(' ', '')
#             for l in stms_lines]
#    stms = [l.replace(' ', '').replace(',axiom,', ',conjecture,')
#            for l in stms_lines]
#    return dict(zip(names, stms))


def read_rankings(path):
    rankings = {}
    rankings_lines = read_lines(path)
    for l in rankings_lines:
        thm, rk = l.split(':')
        rk = rk.split(' ')
        assert thm not in rk
        assert thm not in rankings
        rankings[thm] = rk
    return rankings




def dict_features_flas(features):
    "Formulas associated with a given feature."
    dict_features_flas = {}
    for fla in features:
        for f in features[fla]:
            try:
                dict_features_flas[f].add(fla)
            except BaseException:
                dict_features_flas[f] = {fla}
    return dict_features_flas


def dict_features_numbers(features):
    "How many theorems with a given feature we have."
    dft = dict_features_flas(features)
    return {f: len(dft[f]) for f in dft}


#def similarity(thm1, thm2, dict_features_numbers, n_of_theorems, power=2):
#    ftrs1 = set(thm1[1])
#    ftrs2 = set(thm2[1])
#    ftrsI = ftrs1 & ftrs2
#    # we need to add unseen features to our dict with numbers
#    for f in (ftrs1 | ftrs2):
#        if f not in dict_features_numbers:
#            dict_features_numbers[f] = 1
#    def trans(l, n): return log(l / n) ** power
#    s1 = sum([trans(n_of_theorems, dict_features_numbers[f]) for f in ftrs1])
#    s2 = sum([trans(n_of_theorems, dict_features_numbers[f]) for f in ftrs2])
#    sI = sum([trans(n_of_theorems, dict_features_numbers[f]) for f in ftrsI])
#    return (sI / (s1 + s2 - sI)) ** (1 / power)  # Jaccard index

# TODO UNCHANGE
def similarity(thm1, thm2):
    ftrs1 = set(thm1[1])
    ftrs2 = set(thm2[1])
    return len(ftrs1 & ftrs2) / len(ftrs1 | ftrs2)


def merge_predictions(predictions_paths):
    if isinstance(predictions_paths, str):
        predictions_paths = [predictions_paths]
    assert isinstance(predictions_paths, list)
    predictions_lines = []
    for p in predictions_paths:
        predictions_lines.extend(read_lines(p))
    predictions = []
    for l in predictions_lines:
        conj, deps = l.split(':')
        deps = set(deps.split(' '))
        conj_deps = conj, deps
        if conj_deps not in predictions:
            predictions.append(conj_deps)
    return predictions


def unify_predictions(predictions):
    predictions_unified = {}
    for conj, deps in predictions:
        if conj not in predictions_unified:
            predictions_unified[conj] = set(deps)
        else:
            predictions_unified[conj].update(deps)
    return predictions_unified

def random_name():
    return uuid.uuid4().hex


def remove_supersets(list_of_sets):
    '''Removes supersets from the list of sets'''
    list_of_sets_clean = []
    list_of_sets = [set(s) for s in list_of_sets]  # in case we have lists
    l = len(list_of_sets)
    for i1 in range(l):
        for i2 in range(l):
            if list_of_sets[i1] > list_of_sets[i2]:
                break
        else:
            if list_of_sets[i1] not in list_of_sets_clean:
                list_of_sets_clean.append(list_of_sets[i1])
    return list_of_sets_clean


def grep_first(file, pattern):
    with open(file) as f:
        for line in f:
            if re.search(pattern, line):
                return line.strip()


class AvailablePremises:
    def __init__(self, chronology=None, available_premises=None,
                 knn_hard_negs=False, knn_prefiltering=None,

                 **_):
        self.available_premises_file = available_premises
        self.chronology_list = read_lines(chronology) if chronology else None
        # TODO if the chronology list large -- read the file after call

    def __call__(self, conj):
        if self.chronology_list:
            return set(self.chronology_list[:self.chronology_list.index(conj)])
        if self.available_premises_file:
            line = grep_first(self.available_premises_file, '^' + conj + ':')
            assert line, conj
            return set(line.split(':')[1].split(' '))


def parse_tptp_proof(proof_file):
    with open(proof_file) as f:
        proof_lines = f.read().splitlines()
    #proof_lines = [l for l in proof_lines if l and not l[0] == '#']
    proof_lines = [l for l in proof_lines if l and ('fof(' in l or 'cnf(' in l)]
    proof_lines_words = [re.findall(r"[\w']+", l) for l in proof_lines]
    names = [ws[1] for ws in proof_lines_words]
    assert '($false)' in ''.join(proof_lines), proof_file
    names[-1] = 'FALSE'
    axioms = [names[i]
              for i in range(len(names)) if 'axiom' in proof_lines_words[i]]
    # ^ sometimes this criterion is not enough... (e.g there is some bug in E)
    conjectures = [
        names[i] for i in range(
            len(names)) if 'conjecture' in proof_lines_words[i]]
    used = []
    for ws in proof_lines_words:
        ws = ws[2:]
        us = [w for w in ws if w in names]
        used.append(us)
    deps = dict(zip(names, used))
    no_axioms = []
    # see the comment above that word 'axiom' is not enough
    for t in deps:
        if set(deps[t]) - {t}:
            no_axioms.append(t)
    axioms = set(axioms) - set(no_axioms)
    for t in deps:
        if t in axioms or t in conjectures:
            deps[t] = []
    return deps, axioms, conjectures


# (sub)tree is a dictionary with one element: {root: [child1, child2, ...]}
def build_tree(start, deps):
    return {start: [build_tree(d, deps) for d in deps[start]]}


def build_compact_tree(start, deps):
    def bct(s):
        if len(deps[s]) == 1:
            return bct(deps[s][0])
        else:
            return {s: [bct(d) for d in deps[s]]}
    return bct(start)


def statements_dict(proof_file):
    with open(proof_file) as f:
        proof_lines = f.read().splitlines()
    #proof_lines = [l for l in proof_lines if l and not l[0] == '#']
    proof_lines = [l for l in proof_lines if l and ('fof(' in l or 'cnf(' in l)]
    proof_lines = [l.replace(' ','') for l in proof_lines]
    proof_lines_cut = [','.join(l.split(',')[2:]) for l in proof_lines]
    # TODO shorten below
    proof_lines_words = [re.findall(r"[\w']+", l) for l in proof_lines]
    names = [ws[1] for ws in proof_lines_words]
    assert '($false)' in ''.join(proof_lines), proof_file
    names[-1] = 'FALSE'
    ###
    statements = []
    for l in proof_lines_cut:
        if ',file(' in l:
            statements.append(l.split(',file(')[0])
        else:
            statements.append(l.split(',inference(')[0])
    for i in range(len(statements)):
        assert statements[i], proof_file
        if statements[i][0] == '(':
            statements[i] = statements[i][1:-1]
    assert len(statements) == len(proof_lines) == len(names)
    statements_dict = {names[i]: statements[i] for i in range(len(names))}
    return statements_dict


def root_name(tree):
    return list(tree)[0]

def subtrees_of_root(tree):
    return tree[list(tree)[0]]

def height(tree):
    subtrees = subtrees_of_root(tree)
    if len(subtrees) == 0:
        return 0
    else:
        return max([1 + height(subtree) for subtree in subtrees])

def leaves(tree):
    leaves = set()
    def add_leaves_to_list(tree0):
        if not subtrees_of_root(tree0):
            leaves.add(root_name(tree0))
        else:
            for subtree in subtrees_of_root(tree0):
                add_leaves_to_list(subtree)
    add_leaves_to_list(tree)
    return list(leaves)

def root_leaves_heigh_of_all_subtrees(tree):
    root_leaves_heigh = []
    def rlh(tree0):
        h = height(tree0)
        if h > 0:
            root_leaves_heigh.append((root_name(tree0), leaves(tree0), h))
            for subtree in subtrees_of_root(tree0):
                rlh(subtree)
    rlh(tree)
    return root_leaves_heigh


class TimeoutException(Exception): pass

@contextmanager
def time_limit(seconds):
    def signal_handler(signum, frame):
        raise TimeoutException("Timed out!")
    signal.signal(signal.SIGALRM, signal_handler)
    signal.alarm(seconds)
    try:
        yield
    finally:
        signal.alarm(0)


def preds_quality(preds, deps):
    deps = read_deps(deps, unions=True)
    preds = read_deps(preds, unions=True)
    thms = set(deps) & set(preds)
    goodness = []
    for t in thms:
        p = set(preds[t])
        d = set(deps[t])
        overlap  = len(p & d) / len(p | d)
        coverage = len(p & d) / len(d)
        assert overlap <= coverage
        good = overlap * 0.3 + coverage * 0.7
        goodness.append(good)
    return sum(goodness) / len(goodness)


def scored_preds_quality(preds, deps):
    deps = read_deps(deps)
    preds = read_deps(preds)
    thms = set(deps) & set(preds)
    goodness = []
    for t in thms:
        for d in deps[t]:
            for p in preds[t]:
                p = set(p)
                d = set(d)
                overlap  = len(p & d) / len(p | d)
                coverage = len(p & d) / len(d)
                good = overlap * 0.3 + coverage * 0.7
                goodness.append(good)
    return sum(goodness) / len(goodness)

def grid_from_params(params_string):
    params_list = params_string.split(';')
    params_list = [p.split(':') for p in params_list]
    params = dict([(p[0], p[1].split(',')) for p in params_list])
    names = list(params)
    values = [params[n] for n in names]
    prod = product(*values)
    def numer(x):
        try:
            return int(x)
        except:
            return float(x)
    return [dict(zip(names, [numer(x) for x in p])) for p in prod]
