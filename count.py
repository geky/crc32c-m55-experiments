#!/usr/bin/env python3

import re

TYPES = {
    'vmul': '(vmul.*)',
    'vector': '(vmov.*|vdup.*|vshl.*|veor|vorr.*|vand.*)',
    'mul': '(mul.*|umull)',
    'ld/st': '(push|pop|ldr.*|str.*|ldmia.*|stmdb.*|vpush|vpop|vldr.*'
        '|vrev.*|vstr.*|vsli.*|vshr.*|vddup.*)',
    'branch': '(bne.*|bcc.*|bhi.*|beq.*|bcs.*|le|cbz|cbnz)',
    'other': '(and.*|orr.*|eor.*|add.*|sub.*|mov.*|mvn.*|lsl.*|lsr.*'
        '|uxtb|uxth|it|cmp|bic.*|rbit|b\.n|b\.w|bl|bx|dls|tst|vmsr|vpst'
        '|rsb|ubfx)',
}

def main(paths):
    print('%-42s %7s %s' % (
            '',
            'ins',
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

        print('%-42s %7d %s' % (
                re.sub('\.trace$', '', path),
                count,
                ' '.join('%7s' % v for v in types.values())))


if __name__ == "__main__":
    import sys
    main(sys.argv[1:])
