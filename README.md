# SVA VulnMiner

SVA VulnMiner is a C-based tool for building vulnerability datasets from publicly available vulnerability information and Git repositories.

The tool combines information from the **National Vulnerability Database (NVD)** and **GitHub** to identify vulnerability-related commits and extract structured information about the affected source code.

The current implementation focuses on **C and C++ repositories** and produces one JSON file per processed vulnerability-related commit.

## Table of Contents

- [Features](#features)
- [Requirements](#requirements)
- [Build](#build)
- [Usage](#usage)
- [Configuration](#configuration)
  - [CWE selection](#cwe-selection)
  - [CVE publication dates](#cve-publication-dates)
  - [NVD API key](#nvd-api-key)
  - [GitHub API key](#github-api-key)
  - [Source file types](#source-file-types)
  - [Context depth](#context-depth)
  - [File limits](#file-limits)
- [Processing pipeline](#processing-pipeline)
  - [NVD](#1-nvd)
  - [GitHub commit processing](#2-github-commit-processing)
  - [Changed files](#3-changed-files)
  - [Context extraction](#4-context-extraction)
  - [Source-code parsing](#5-source-code-parsing)
  - [Function matching](#6-function-matching)
  - [Dataset entry generation](#7-dataset-entry-generation)
  - [Output file generation](#8-output-file-generation)
- [Parallel processing](#parallel-processing)
- [Development](#development)
- [Known limitations](#known-limitations)
  - [Function matching](#function-matching)
  - [C++ operators](#c-operators)
  - [Merge commits](#merge-commits)
  - [GitHub availability](#github-availability)
  - [Large repositories](#large-repositories)

## Features

VulnMiner currently provides:

- NVD queries filtered by CWE and CVE publication date.
- Extraction of GitHub commit references from CVE information.
- GitHub commit retrieval through the GitHub API.
- Support for C and C++ source files.
- Handling of modified, added, deleted and renamed files.
- Retrieval of complete file contents before and after a commit.
- Context-file extraction around affected files.
- Context distance information.
- Tree-sitter based extraction of functions and methods.
- Extraction of function and method content.
- Matching of functions between the before and after versions.
- JSON dataset generation.

## Requirements

SVA VulnMiner currently supports **Linux systems only**.

The following dependencies are required:

- CMake >= 3.20
- GCC or Clang
- libcurl

Tree-sitter and yyjson are included in the `third_party/` directory and do not need to be installed separately.

## Build

From the project root:

    mkdir build
    cd build
    cmake ..
    make

The executable is generated as:

    build/vulnminer

## Usage

The program is configured through a `config.ini` file.

From the `build/` directory, run:

    ./vulnminer

By default, the program looks for the configuration file at:

    ../config.ini

A different configuration file can be specified with the `-i` option:

    ./vulnminer -i /path/to/config.ini

By default, the program looks for the generated dataset export folder at:

    ../results

A different export folder can be specified with the `-o` option:

    ./vulnminer -o /path/to/folder

## Configuration

A configuration file contains the parameters used to select CVEs and control the extraction process.

Example:

    cwe-ids=787,190,125
    cve-published-before=01/09/2026
    cve-published-after=01/06/2026
    nvd-api-key=YOUR_NVD_API_KEY
    github-api-key=YOUR_GITHUB_API_KEY
    include-c-files=yes
    include-cpp-files=yes
    context-depth=0
    max-files-commit=50
    max-files-context=50
    max-files-total=50

### CWE selection

Comma-separated list of CWE IDs to retrieve from the NVD.

For example:

    cwe-ids=787,190,125

will retrieve CVEs associated with CWE-787, CWE-190 and CWE-125.

### CVE publication dates

    cve-published-before=01/09/2026
    cve-published-after=01/06/2026

Defines the publication date range of the CVEs to retrieve.

Dates must use the following format:

    DD/MM/YYYY

If `cve-published-before` is not specified or invalid, the current date is used.

If `cve-published-after` is not specified or invalid, `01/01/1999` is used.

### NVD API key

    nvd-api-key=YOUR_NVD_API_KEY

API key used to query the NVD API.

Optional, but strongly recommended for large downloads.

To obtain a key, visit the [NVD API key request page](https://nvd.nist.gov/developers/request-an-api-key) and follow the instructions to request one.

### GitHub API key

    github-api-key=YOUR_GITHUB_API_KEY

GitHub personal access token used to query the GitHub API.

**Required.** VulnMiner requires authentication to avoid GitHub's very restrictive unauthenticated API rate limit.

To create a token, visit [GitHub Settings → Developer settings → Personal access tokens](https://github.com/settings/personal-access-tokens) and create a fine-grained personal access token.

### Source file types

The following options control which source files are considered when processing a commit. Setting an option to a value other than `yes` disables the corresponding file type.

For example:

    include-c-files=yes
    include-cpp-files=no

will include C files while ignoring C++ files.

#### C files

    include-c-files=yes

When enabled, C source and header files are included:

    .c
    .h

#### C++ files

    include-cpp-files=yes

When enabled, C++ source and header files are included:

    .cpp
    .hpp

### Context depth

    context-depth=2

The context depth controls how far from an affected file additional repository files can be selected.

The distance is defined relative to the affected file:

- `0`: same file
- `1`: same directory
- `2`: direct parent/child directory level
- `3`: direct parent/child directory of each directory included in context depth 2
- larger values: progressively farther directory levels

Context extraction is optional. A value of `0` disables context extraction.

### File limits

The following options limit dataset size:

    max-files-commit=50
    max-files-context=50
    max-files-total=50

- `max-files-commit`: maximum number of files affected by a commit. A commit exceeding this value is rejected.
- `max-files-context`: maximum number of context files found for a commit, depending on the selected depth. A commit exceeding this value is rejected.
- `max-files-total`: maximum total number of files in a dataset entry, including affected and context files. A commit exceeding this value is rejected.

These limits prevent unusually large commits or repositories from producing excessively large dataset entries.

## Processing pipeline

The processing pipeline is:

    NVD
     │
     ├── CWE + date range
     │
     ▼
    CVEs
     │
     ├── GitHub Patch references
     │
     ▼
    GitHub commits
     │
     ├── Commit metadata
     │
     ├── Changed files
     │     ├── Before version
     │     └── After version
     │
     ├── Context files
     │
     ▼
    Tree-sitter
     │
     ├── Functions content
     ├── Name, parameter count
     ├── Before/after matching
     │
     ▼
    JSON dataset

### 1. NVD

VulnMiner first queries the NVD using the configured CWE identifiers and CVE publication-date range.

For each CVE, the tool extracts:

- CVE identifier
- publication information
- description
- references

Only references identified as GitHub patch references are considered for further processing.

### 2. GitHub commit processing

For each candidate commit, VulnMiner retrieves the commit information from GitHub:

- repository
- commit hash
- parent commit
- commit message
- changed file paths

Commits containing files with unsupported extensions, or exceeding the configured file limit, are rejected before downloading the content of the files.

### 3. Changed files

GitHub can report files with different statuses, including:

- `modified`
- `added`
- `deleted`
- `renamed`

For each source file, the appropriate before and after versions are retrieved.

For example:

    modified:

        before = previous commit version
        after  = current version

    added:

        before = none
        after  = current version

    deleted:

        before = previous version
        after  = none

    renamed:

        before = previous path/version
        after  = new path/version

### 4. Context extraction

When context extraction is enabled, VulnMiner retrieves additional files from the repository tree.

If the context contains too many file paths, the dataset entry is discarded before downloading the actual content of the context files.

Each context file contains the distance to the affected file(s), allowing the resulting dataset to retain structural information about the repository around the vulnerability-related change.

Example:

    {
        "path": "README.md",
        "content": "...",
        "distances": [
            {
                "path": "file1.c",
                "distance": 0
            },
            {
                "path": "dir1/file2.c",
                "distance": 1
            },
            {
                "path": "dir1/dir2/file3.c",
                "distance": 2
            }
        ]
    }

The `distances` field lists the commit affected files associated with the context file and their corresponding directory distance.

### 5. Source-code parsing

Supported source files are parsed with Tree-sitter.

The parser extracts functions and methods and records information such as:

- function name
- function content
- number of parameters

The before and after versions of a file are then compared to determine which functions were added, removed or modified.

### 6. Function matching

Functions are matched between the before and after versions using their:

- name
- number of parameters

This allows the parser to identify functions whose implementation changed while keeping the same name and number of parameters.

### 7. Dataset entry generation

Each accepted commit produces one JSON file.

A dataset entry contains information about:

- the CWE
- the CVE
- the CVE description
- the repository
- the commit
- affected files
- extracted functions
- optional context files

A simplified example is:

    {
        "CWE": 476,
        "CVE": "CVE-2026-21498",
        "CVE_description": "...",
        "repository": "example/project",
        "commit_hash": "abcdef1234567890...",
        "commit_message": "...",
        "files": [
            {
                "status": "modified",
                "path": "src/example.cpp",
                "previous_path": "src/example.cpp",
                "before": "...",
                "after": "...",
                "affected_functions": [
                    {
                        "name": "example",
                        "parameters_number": 2,
                        "status": "modified",
                        "content": "...",
                        "previous_content": "...",
                    }
                ]
            }
        ],
        "context_files": [
            {
                "path": "README.md",
                "content": "...",
                "distances": [
                    {
                        "path": "src/example.cpp",
                        "distance": 1
                    }
                ]
            }
        ]
    }

The exact content depends on the commit type and configuration.

For example, a renamed file may contain a `previous_path`, while an added file has no previous version.

### 8. Output file generation

Dataset files are written to the configured output directory.

The filename follows the format:

    CWE-<CWE>_<CVE>_<repository>_<commit>.json

where `<commit>` is represented by the first eight characters of the commit hash.

For example:

    CWE-476_CVE-2026-21498_iccDEV_bdfa3194.json

Already existing output files names are not overwritten.

## Parallel processing

VulnMiner uses worker threads and job queues to separate the different processing stages. The current architecture contains a GitHub processing queue and a parsing/export queue:

    NVD
     │
     ├── main thread
     │
     ▼
    GitHub commits downloading job queue
     │
     ├── worker thread
     │
     ▼
    Tree-sitter parsing and export job queue
     │
     ├── worker thread
     │
     ▼
    JSON files

This separation allows the NVD acquisition stage, the GitHub acquisition stage and the source-code parsing stage to be processed independently:

1. The master thread retrieves and prepares a CVE reference. Once a reference is accepted, it is submitted to the GitHub queue.
2. A GitHub worker retrieves and prepares a commit. Once the commit is accepted, it is submitted to the parsing queue.
3. A parsing worker then performs Tree-sitter analysis and generates the JSON output.

The architecture is intentionally suitable for adding different sources of vulnerabilities. For example, adding another vulnerability source would only require a module responsible for retrieving that source, after which it could submit the resulting commits to the existing GitHub processing queue.

For this version, there are only three threads used in total, including the main thread: one for each of the three pipeline stages described above. Using several threads for the same download stage (NVD or GitHub) is not particularly useful, because the network remains by far the main bottleneck. Similarly, using several threads for the Tree-sitter parsing stage does not necessarily improve the overall execution time, since a single parsing thread is generally able to parse and export a commit before the next commit is downloaded.

## Development

The project uses CMake for its build system.

Several tools are used during development and validation:

- `clang-format`
- `Cppcheck`
- `Include-What-You-Use`
- `Valgrind`

For example:

    valgrind --leak-check=full ./vulnminer

Formatting can be performed with the project's formatting target:

    make format

Includes can be verified with:

    make inc

Code checking can be run with:

    make check

The available CMake targets should be preferred over manually reproducing the project's checks.

## Known limitations

The current implementation has several known limitations.

### Function matching

Function matching currently relies on the function name and parameter count.

As a result, different C functions or C++ methods with identical names and parameter numbers may be matched incorrectly.

### Multiple parent commits

Most commits have a single parent, but merge commits can have multiple parents.

Since the dataset requires an unambiguous before/after pair, commits with multiple parents are currently discarded.

The unified diff returned by GitHub cannot necessarily be used to reconstruct the complete previous file because it only contains the changed sections and some surrounding context.

### GitHub availability

The dataset depends on information that remains accessible through the GitHub API. Deleted repositories, unavailable commits, inaccessible files and API errors can therefore prevent an entry from being generated.

### Large repositories

Context extraction can become expensive for large repositories. The context-depth and file-count limits are provided to control this cost.