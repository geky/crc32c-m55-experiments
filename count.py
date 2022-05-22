#!/usr/bin/env python3

import re

TYPES = {
    'mul': '$^',
    'ld/st': '(push|pop|ldr.*)',
    'branch': '(b.*)',
    'other': '(and.*|eor.*|add.*|sub.*|mov.*|mvn.*|lsl.*|uxtb|it|cmp|)',
}

def main(paths):
    print('%-24s %7s %s' % (
            '',
            'count',
            ' '.join('%7s' % t for t in TYPES.keys())))

    for path in paths:
        types = {t: 0 for t in TYPES.keys()}
        count = 0
        with open(path) as f:
            for line in f:
                m = re.match('=>\s+[^:]*:\s+([^\s]+)\s', line)
                if m:
                    ins = m.group(1)
                    count += 1

                    for t, pattern in TYPES.items():
                        if re.match(pattern, ins):
                            types[t] += 1
                            break
                    else:
                        print('warning: unknown ins "%s"' % ins)

        print('%-24s %7d %s' % (
                re.sub('\.trace$', '', path),
                count,
                ' '.join('%7s' % v for v in types.values())))


if __name__ == "__main__":
    import sys
    main(sys.argv[1:])
