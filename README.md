# Repository Analyzer

Repository Analyzer is a research project developed during my internship at the University of Parma.

The goal of this project is to build a scalable repository analysis pipeline capable of extracting structured information from software repositories. The extracted evidence will later be used for vulnerability detection and LLM-based analysis.

## Getting started

Clone the repository:

```bash
git clone https://github.com/paul2633/repo-analyzer.git
```

Download the benchmark repositories:

```bash
./scripts/download_benchmarks.sh
```

Build the project:

```bash
mkdir build
cd build
cmake ..
make
```

Run the analyzer:

```bash
./repo_analyzer ../benchmarks/curl
```