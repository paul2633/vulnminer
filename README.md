# Repository Analyzer

Repository Analyzer is a research project developed during my internship at the University of Parma.

The goal of this project is to build a scalable repository analysis pipeline capable of extracting structured information from software repositories. The extracted evidence will later be used for vulnerability detection and LLM-based analysis.

## Getting started

Clone the repository:

```bash
git clone https://github.com/paul2633/repo-analyzer.git
```

Build the project:

```bash
mkdir build
mkdir results    # optional
cd build
cmake ..
make
```

If the `results` directory does not exist, JSON outputs are written to the `build` directory instead.

# Usage

```bash
repo_analyzer [OPTION] <repository>
```

## Options

- `-l <path>`
  Analyze a repository already available on the local filesystem.

- `-d <url>`
  Clone the repository into a temporary directory, analyze it, then remove it.

- `-r <url>`
  Analyze a remote repository without cloning it locally. *(Work in progress.)*

## Examples

Analyze a local repository:

```bash
./repo_analyzer -l ../benchmarks/scrcpy
```

Clone, analyze, then remove the repository:

```bash
./repo_analyzer -d https://github.com/Genymobile/scrcpy
```

Analyze a remote repository (coming soon):

```bash
./repo_analyzer -r https://github.com/Genymobile/scrcpy
```
