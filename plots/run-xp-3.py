#!/usr/bin/env python3
from graphTools import *
from expTools import *

# Command line options
options = {}
options["--mode"] = ["local"]
options["--source" ] = ["../benchmarks/linux"]
options["--granularity"] = ["line"]
options["--variant"] = ["omp_for", "omp_task"]

options["--json-path" ] = ["../results"]
options["--threads"] = [1, 5, 10, 15, 20]
options["--perfs-path"] = ["../results/test3.csv"]

# Launch experiments
execute("../build/repo_analyzer", options, 1, verbose=True, easyPath=".")

options["--variant"] = ["seq"]
options["--threads"] = [1]

execute("../build/repo_analyzer", options, 1, verbose=True, easyPath=".")

execute_simple("../plots/plot.py -if ../results/test3.csv")

