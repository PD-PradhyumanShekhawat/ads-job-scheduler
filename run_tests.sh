#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
make
for n in 1 2 3; do
  ./jobScheduler "testcases/test${n}.txt"
  expected="testcases/test${n}_expected.txt"
  actual="testcases/test${n}_output_file.txt"
  # The project specification permits whitespace differences between status-table columns.
  # Compare every non-table line exactly and compare table rows after collapsing whitespace.
  python3 - "$expected" "$actual" <<'PY'
import re, sys
from pathlib import Path
expected = Path(sys.argv[1]).read_text().splitlines()
actual = Path(sys.argv[2]).read_text().splitlines()
def normalize(lines):
    result=[]; in_table=False
    for line in lines:
        if line == 'JOB STATUS': in_table=True
        if in_table:
            result.append(re.sub(r'\s+', ' ', line.strip()))
        else:
            result.append(line)
        if line == 'END JOB STATUS': in_table=False
    return result
if normalize(expected) != normalize(actual):
    import difflib
    print('FAIL:', sys.argv[1])
    print(''.join(difflib.unified_diff(normalize(expected), normalize(actual), fromfile='expected', tofile='actual')))
    sys.exit(1)
print('PASS:', Path(sys.argv[1]).name)
PY
done
