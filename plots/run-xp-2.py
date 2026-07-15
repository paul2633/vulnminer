#!/usr/bin/env python3
from graphTools import *
from expTools import *

# Command line options
options = {}
options["--mode"] = ["local"]
options["--source" ] = ["../benchmarks/git"]
options["--granularity"] = ["line"]
options["--variant"] = ["seq", "omp_for", "omp_task"]

options["--json-path" ] = ["../results"]
options["--threads"] = [1, 4, 8, 12, 16, 20]
options["--perfs-path"] = ["../results/test2.csv"]

# Launch experiments
execute("../build/repo_analyzer", options, 2, verbose=True, easyPath=".")

execute_simple("../plots/plot.py -if ../results/test2.csv")

