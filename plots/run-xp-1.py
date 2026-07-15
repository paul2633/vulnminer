#!/usr/bin/env python3
from graphTools import *
from expTools import *

# Command line options
options = {}
options["--mode"] = ["local"]
options["--source" ] = ["../benchmarks/git"]
options["--granularity"] = ["file", "function", "line"]
options["--variant"] = ["omp_task"]

options["--json-path" ] = ["../results"]
options["--threads"] = [1, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40]
options["--perfs-path"] = ["../results/test.csv"]

# Launch experiments
execute("../build/repo_analyzer", options, 2, verbose=True, easyPath=".")

execute_simple("../plots/plot.py -if ../results/test.csv")

