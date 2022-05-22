#!/usr/bin/env python3

import re
import collections as co

def main(paths):
    hist = co.defaultdict(lambda: 0)

    for path in paths:
        with open(path) as f:
            for line in f:
                m = re.match('=>\s+[^:]*:\s+([^\s]+)\s', line)
                if m:
                    ins = m.group(1)
                    hist[ins] += 1

    for k, v in sorted(hist.items(),
            key=lambda p: (p[1], p),
            reverse=True):
        print('%7d %s' % (v, k))


if __name__ == "__main__":
    import sys
    main(sys.argv[1:])
