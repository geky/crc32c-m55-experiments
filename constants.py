#!/usr/bin/env python3


# useful polynomial operations
def w(a, w=32):
    return ((1<<w)-1) & a

def brev(a, w=32):
    return int('{:0{w}b}'.format(((1<<w)-1) & a, w=w)[::-1], 2)

def pmul(a, b):
    x = 0
    for i, a_ in enumerate(reversed('{:b}'.format(a))):
        if a_ == '1':
            x ^= b << i
    return x

def pdiv(a, b):
    assert b != 0
    b_bits = len('{:b}'.format(b))
    x = 0
    while True:
        a_bits = len('{:b}'.format(a))
        if a_bits < b_bits:
            return x
        x ^= 1 << (a_bits-b_bits)
        a ^= b << (a_bits-b_bits)

def prem(a, b):
    assert b != 0
    b_bits = len('{:b}'.format(b))
    x = 0
    while True:
        a_bits = len('{:b}'.format(a))
        if a_bits < b_bits:
            return a
        a ^= b << (a_bits-b_bits)

# entry point
def main():
    polynomial = 0x11edc6f41
    polynomial_r = brev(polynomial >> 1)
    print('%-12s = %11s [0x%08x | 0x%08x]' % (
        'polynomial',
        '0x%x' % polynomial,
        w(polynomial),
        brev(polynomial)))
    print('%-12s = %11s [0x%08x | 0x%08x]' % (
        'polynomial_r',
        '0x%x' % polynomial_r,
        w(polynomial_r),
        brev(polynomial_r)))

    barret = pdiv(1 << 64, polynomial)
    barret_r = brev(barret >> 1)
    print('%-12s = %11s [0x%08x | 0x%08x]' % (
        'barret',
        '0x%x' % barret,
        w(barret),
        brev(barret)))
    print('%-12s = %11s [0x%08x | 0x%08x]' % (
        'barret_r',
        '0x%x' % barret_r,
        w(barret_r),
        brev(barret_r)))

    # [ 32  | 32  | 32  | 32  |          128          ]
    #    |     |     |     |              +
    #    |     '-----|-----+->[    64     |    64     ]
    #    |           |                    +
    #    '-----------+------->[    64     |    64     ]

    k64 = prem(1 << 64, polynomial)
    k64_r = brev(prem(1 << (64-1), polynomial))
    print('%-12s = %11s [0x%08x | 0x%08x]' % (
        'k64',
        '0x%x' % k64,
        w(k64),
        brev(k64)))
    print('%-12s = %11s [0x%08x | 0x%08x]' % (
        'k64_r',
        '0x%x' % k64_r,
        w(k64_r),
        brev(k64_r)))

    k96 = prem(1 << 96, polynomial)
    k96_r = brev(prem(1 << (96-1), polynomial))
    print('%-12s = %11s [0x%08x | 0x%08x]' % (
        'k96',
        '0x%x' % k96,
        w(k96),
        brev(k96)))
    print('%-12s = %11s [0x%08x | 0x%08x]' % (
        'k96_r',
        '0x%x' % k96_r,
        w(k96_r),
        brev(k96_r)))

    # [16|16|16|16|16|16|16|16|          128          ]
    #   |  |  |  |  |  |  |  |            +
    #   |  +--|--+--|--+--|--+-->[ 32  | 32  | 32  | 32  ]
    #   |  |  |  |  |  |  |  |            +
    #   |  '--|--+--|--+--|-->[ 32  | 32  | 32  | 32  ]
    #   |     |     |     |               +
    #   +-----+-----+-----+----->[ 32  | 32  | 32  | 32  ]
    #   |     |     |     |               +
    #   '-----+-----+-----+-->[ 32  | 32  | 32  | 32  ]

    k144_r = brev(prem(1 << (144-1), polynomial))
    print('%-12s = %11s [0x%08x | 0x%08x]' % (
        'k144_r',
        '0x%x' % k144_r,
        w(k144_r),
        brev(k144_r)))
    k160_r = brev(prem(1 << (160-1), polynomial))
    print('%-12s = %11s [0x%08x | 0x%08x]' % (
        'k160_r',
        '0x%x' % k160_r,
        w(k160_r),
        brev(k160_r)))

if __name__ == "__main__":
    main()
