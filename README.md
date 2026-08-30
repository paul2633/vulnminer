# SVA VulnMiner

SVA VulnMiner is a tool for collecting vulnerability-related Git commits from the NVD and GitHub as a first step towards generating a vulnerability dataset.

The current implementation focuses on C and C++ source files.

## Requirements

SVA VulnMiner currently supports **Linux systems only**.

The following dependencies are required:

- A C compiler
- libcurl
- pthreads

Tree-sitter and yyjson are included in the `third-party/` directory and do not need to be installed separately.

## Build

From the project root:

    mkdir build
    cd build
    cmake ..
    make

## Usage

The program is configured through a `config.ini` file.

From the `build/` directory, run:

    ./repo_analyzer

By default, the program looks for the configuration file at:

    ../config.ini

A different configuration file can be specified with the `-o` option:

    ./repo_analyzer -o /path/to/config.ini

## Configuration

Example configuration:

    cwe-ids=79,80

    cve-published-before=30/08/2026
    cve-published-after=01/01/2023

    nvd-api-key=YOUR_NVD_API_KEY
    github-api-key=YOUR_GITHUB_API_KEY

    include-c-files=yes
    include-cpp-files=yes

### CWE IDs

    cwe-ids=79,80

Comma-separated list of CWE IDs to retrieve from the NVD.

For example:

    cwe-ids=79,80

will retrieve CVEs associated with CWE-79 and CWE-80.

### CVE publication dates

    cve-published-before=30/08/2026
    cve-published-after=01/01/2023

Defines the publication date range of the CVEs to retrieve.

Dates must use the following format:

    DD/MM/YYYY

If `cve-published-before` is not specified or invalid, the current date is used.

If `cve-published-after` is not specified or invalid, `01/01/1999` is used.

### NVD API key

    nvd-api-key=YOUR_NVD_API_KEY

API key used to query the NVD API.

**The key is optional, but using one is strongly recommended.**

### GitHub API key

    github-api-key=YOUR_GITHUB_API_KEY

GitHub personal access token used to query the GitHub API.

**The key is optional, but using one is strongly recommended.**

### Source file types

The following options control which source files are kept when processing a commit:

    include-c-files=yes
    include-cpp-files=yes

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

Set an option to a value other than `yes` to ignore the corresponding file types.

For example:

    include-c-files=yes
    include-cpp-files=no

will include C files while ignoring C++ files.

## Current pipeline

The current processing pipeline is:

1. Query the NVD for the selected CWE IDs and publication date range.
2. Extract references associated with the CVEs.
3. Keep GitHub commit references tagged as `Patch`.
4. Add the corresponding commits to a job queue.
5. Process the jobs in parallel using worker threads.
6. Query the GitHub API for each commit.
7. Retrieve the commit metadata, parent commit information, and modified files.

The following steps are currently under development:

8. Filter files according to their status and configured file extensions.
9. Retrieve the complete file versions before and after the commit.
10. Parse the files using Tree-sitter to identify modified functions/methods.
11. Generate the final JSON dataset entries.

## Commit filtering

The current implementation applies several restrictions when constructing dataset entries.

### Unavailable commits

Some commits may no longer be available and return a `404` error. These commits are discarded.

### File extensions

Only file types enabled in the configuration are considered.

Other file types are currently ignored, even though they may provide useful context for understanding the vulnerability fix.

### File status

Only files with the `modified` status are currently kept.

GitHub can also report files as:

- `added`
- `deleted`
- `renamed`

These cases may be useful, but the current version focuses on files for which both a before and an after version can be obtained.

### Multiple parent commits

Most commits have a single parent, but merge commits can have multiple parents.

Since the dataset requires an unambiguous before/after pair, commits with multiple parents are currently discarded.

The unified diff returned by GitHub cannot necessarily be used to reconstruct the complete previous file because it only contains the changed sections and some surrounding context.

## Dataset format

The goal is to generate one JSON file for each dataset entry.

Each entry will contain the files retained from the commit and, for each file, the functions or methods affected by the change together with their complete before and after versions.

The intended structure is approximately:

    {
        "files": [
            {
                "path": "src/example.c",
                "functions": [
                    {
                        "name": "example_function",
                        "before": "...",
                        "after": "..."
                    }
                ]
            }
        ]
    }

The exact dataset format is still under development.

## Development

The following tools can be used to check the code during development:

- Valgrind
- Cppcheck
- Include-What-You-Use

For example:

    valgrind --leak-check=full ./repo_analyzer

These tools are not required to run SVA VulnMiner.