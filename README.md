# SVA VulnMiner

SVA VulnMiner is a tool for collecting vulnerability-related Git commits from the NVD and GitHub as a first step towards generating a vulnerability dataset.

The current implementation focuses on C and C++ source files.

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

Example configuration:

    cwe-ids=787,476,190,125,79
    cve-published-before=01/09/2026
    cve-published-after=01/05/2026
    nvd-api-key=YOUR_NVD_API_KEY
    github-api-key=YOUR_GITHUB_API_KEY
    include-c-files=yes
    include-cpp-files=yes

### CWE IDs

    cwe-ids=787,476,190,125,79

Comma-separated list of CWE IDs to retrieve from the NVD.

For example:

    cwe-ids=79,80

will retrieve CVEs associated with CWE-79 and CWE-80.

### CVE publication dates

    cve-published-before=01/09/2026
    cve-published-after=01/05/2026

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

The following options control which source files are considered when processing a commit:

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
2. Extract CVE information, including the CVE identifier and English description.
3. Extract references associated with the CVEs.
4. Keep GitHub commit references tagged as `Patch`.
5. Create dataset entries for the corresponding GitHub commits.
6. Add the commits to a job queue.
7. Process GitHub jobs using worker threads.
8. Query the GitHub API for each commit.
9. Retrieve the commit parent information, commit message, and modified files.
10. Filter files according to their status and configured file extensions.
11. Retrieve the complete file versions before and after the commit.
12. Add successfully retrieved entries to the parsing queue.
13. Export the resulting dataset entry as a JSON file.

The following step is currently under development:

14. Parse the files using Tree-sitter to identify modified functions/methods.

## Commit filtering

The current implementation applies several restrictions when constructing dataset entries.

### Unavailable commits

Some commits may no longer be available and return a `404` error. These commits are discarded.

### File extensions

Only file types enabled in the configuration are considered for further processing.

The currently supported source file extensions are:

- `.c`
- `.h`
- `.cpp`
- `.hpp`

Other file types are retained in the dataset as excluded files with an `unsupported_extension` reason.

### File status

GitHub can report files with different statuses, including:

- `modified`
- `added`
- `deleted`
- `renamed`

Only files with the `modified` status are currently considered for further processing.

Other statuses are retained as excluded files with a `not_modified` reason.

### File content

For modified and supported files, the complete file content is retrieved both before and after the commit.

If either version cannot be retrieved, the file is excluded with a `content_unavailable` reason.

### Multiple parent commits

Most commits have a single parent, but merge commits can have multiple parents.

Since the dataset requires an unambiguous before/after pair, commits with multiple parents are currently discarded.

The unified diff returned by GitHub cannot necessarily be used to reconstruct the complete previous file because it only contains the changed sections and some surrounding context.

## Dataset format

The goal is to generate one JSON file for each dataset entry.

Each entry contains the CVE information, repository and commit information, and the files processed from the commit.

Files are divided into two groups:

- `included_files`: files for which both the before and after versions were successfully retrieved and which can be processed by the parsing stage.
- `excluded_files`: files that were not retained for further processing, together with the reason for their exclusion.

The current JSON structure is:

    {
        "CWE": 787,
        "CVE": "CVE-2026-45328",
        "CVE_description": "...",
        "repository": "espressif/esp-idf",
        "commit_hash": "7867f4a57560bf9fc4a931e37ba02b7a3e9f406b",
        "commit_message": "...",
        "included_files": [
            {
                "path": "src/example.c"
            }
        ],
        "excluded_files": [
            {
                "path": "README.md",
                "reason": "unsupported_extension"
            },
            {
                "path": "other.c",
                "reason": "content_unavailable"
            }
        ]
    }

The `included_files` entries will be extended by the Tree-sitter parsing stage to contain the modified functions or methods and their complete before and after versions.

## Output files

Generated dataset entries are written to the configured export directory.

Output filenames follow the format:

    CWE-<CWE>_<CVE>_<repository>_<commit>.json

where `<commit>` is the first 8 characters of the commit hash.

For example:

    CWE-787_CVE-2026-45328_esp-idf_7867f4a5.json

Existing files are not overwritten.

## Development

The following tools can be used to check the code during development:

- Valgrind
- Cppcheck
- Include-What-You-Use
- clang-format

For example:

    valgrind --leak-check=full ./vulnminer

These tools are not required to run SVA VulnMiner.