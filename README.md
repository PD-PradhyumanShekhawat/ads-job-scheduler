# COP 5536 Job Scheduler (C++)

## Build

```sh
make
```

## Run

```sh
./jobScheduler path/to/input.txt
```

The program writes output to `path/to/input_output_file.txt` (the input filename's extension is replaced by `_output_file.txt`).

## Implementation outline

- `Job.h`: shared job record and status/mode enums.
- `AVLTree.h`: custom AVL tree keyed by job ID; stores references to `Job` objects and tracks required statistics.
- `MaxHeap.h`: custom binary max heap over waiting jobs; supports insertion, extraction, arbitrary removal, priority updates, and a one-way switch to `SHORT_JOBS` ordering.
- `jobScheduler.cpp`: command parsing/dispatch, simulation time, running-job min-priority queue, cancelled list, status output, and final reports.
- The running-job priority queue uses the C++ standard library as permitted by the specification. No third-party dependencies are used.

This is a working implementation under verification against the supplied test cases; do not treat it as submission-ready until the comparisons and edge-case tests pass.
