In order to reproduce experiments presented in the paper, please, follow
the instructions from this file.

# Requirements

- linux
- python 3.9

# Compile CVC5

The included CVC5 code is modified by us.

```
main_dir=`pwd`
cd CVC5
./configure.sh --auto-download
cd build
make -j20
cd $main_dir
```

# Install LightGBM

```
git clone --recursive https://github.com/microsoft/LightGBM
cd LightGBM
mkdir build
cd build
cmake ..
make -j20
cd $main_dir
```

# Install Python packages

```
cd loop
pip install -r requirements.txt
```

(If you see some conflicts, create a virtual environment, activate it and
install the Python dependencies there. For example like this:
```
virtualenv venv
source venv/bin/activate
pip install -r requirements.txt
```
)

# Download data

Download two families of SMT-LIB benchmarks: UFNIA and UFLIA,
and save it to `loop/data`

```
cd data
git clone git@clc-gitlab.cs.uiowa.edu:SMT-LIB-benchmarks/UFLIA.git
git clone git@clc-gitlab.cs.uiowa.edu:SMT-LIB-benchmarks/UFNIA.git
cd ..
```

If the above commands do not work, please, follow the instructions from
http://smtlib.cs.uiowa.edu/benchmarks.shtml
and download the UFLIA and UFLIA benchmarks from StarExec. In folder
`loop/data` there should be two folders, `UFLIA` and `UFNIA`, so that
paths listed in files `loop/data/*filtered-problems*` refer to existing
files.

# Running experiments

The looping training and evaluation presented in the paper can be
reproduced by running the following 6 scripts. First 3 run the ML-guided
solver, last 3 run the randomized solver.

Files
`loop/data/*filtered-problems-test` and
`loop/data/*filtered-problems-train`
used in the scripts refer to testing and training problems, respectively.

The scripts print logs with relevant data presented in the paper.  All the
data produced during run of a script `$SCRIPT.sh` is included in a new
directory `$SCRIPT.data`.

## ML-guided solver

```
./experiments/UFNIA-sledgehammer-ML.sh
./experiments/UFLIA-sledgehammer-ML.sh
./experiments/UFLIA-boogie-ML.sh
```

## Randomized solver

```
./experiments/UFNIA-sledgehammer-RANDOM.sh
./experiments/UFLIA-sledgehammer-RANDOM.sh
./experiments/UFLIA-boogie-RANDOM.sh
```

## Extracting relevant data from logs

Each of the above scripts prints logs. Assuming the log was saved to file
`$LOG`, the relevant information can be extracted from it by grepping for:

```
grep $LOG \
-e 'Loop iteration' \
-e 'Number of problems solved now in train' \
-e 'Number of all problems solved in train' \
-e 'Average number of instantiations in train' \
-e 'Number of problems solved now in test' \
-e 'Number of all problems solved in test' \
-e 'Average number of instantiations in test' \
-e 'Number of training examples'
```

# Computing infrastructure used for running experiments

All the experiments presented in the paper were run on 3 servers
with the following specification:

OS: Ubuntu 20.04 focal
Kernel: x86_64 Linux 5.4.0-40-generic
Shell: bash 5.0.16
RAM: 772687MiB
CPU: Intel Xeon Gold 6140 @ 3.7GHz
Number of CPUs: 72

(Total computing time used to run the experiments was about 3 CPU-years.)
