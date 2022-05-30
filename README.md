
A comparison of some [crc32c][crc] implementations that may target
microcontrollers. This is a branch of the exploration in [gf256][gf256],
with most of the techniques here explained in detail in Intel's [Fast CRC
Computation for Generic Polynomials Using PCLMULQDQ Instruction][intel-whitepaper].

The original motivation for this was the introduction of polynomial
multiplication instructions in the [MVE extensions][MVE] introduced in
ARM-v8.1 and Cortex-M55. MVE introduces `vmull.p8` and `vmull.p16`, 8x8-bit
and 4x16-bit widening multiplication instructions.

Polynomial multiplication as a form of CRC acceleration has become popular
recently, and on Cortex-M it has the extra benefit of replacing lookup tables
and reducing code size, which is often more a priority than performance on
these devices.

## Comparison

Each implementation can be found in its own .c file. I'm comparing code size
and executed instructions to get a rough comparison. This was compiled with
GCC 11 -Os -mcpu=cortex-m55.

To get a rough estimate of instructions executed I used a hacky GDB script
to step through each implementation with pseudo-random data of size 4096 bytes.
I realize this is NOT cycle-accurate and not the best indicator of performance,
but it's was certainly the easiest measurement to perform, and possible with
open-source tooling. I may or may not (probably not) investigate further.

``` bash
$ make size
$ make stack
$ make count -j
```

These implementations probably aren't super-optimal, but certainly usable.

## Results

|                                            |     code  |    stack  |      ins  |     vmul  |   vector  |      mul  |    ld/st  |   branch  |    other  |
|:-------------------------------------------|----------:|----------:|----------:|----------:|----------:|----------:|----------:|----------:|----------:|
| crc32c_naive                               |     **48**|       12  |   221192  |      **0**|      **0**|      **0**|     4099  |    36865  |   180228  |
| crc32c_naive_32wide                        |       84  |       16  |   209928  |      **0**|      **0**|      **0**|     1027  |    35841  |   173060  |
| crc32c_naive_mul                           |     **48**|      **8**|   159752  |      **0**|      **0**|    32768  |     4099  |    36865  |    86020  |
| crc32c_naive_mul_32wide                    |       88  |       16  |   145416  |      **0**|      **0**|    32768  |     1027  |    35841  |    75780  |
| crc32c_small_table                         |      316  |       12  |    49160  |      **0**|      **0**|      **0**|    12291  |     4097  |    32772  |
| crc32c_table                               |     1064  |      **8**|    32776  |      **0**|      **0**|      **0**|     8195  |     4097  |    20484  |
| crc32c_barret_naive                        |      132  |       28  |  2424842  |      **0**|      **0**|      **0**|     4099  |   266241  |  2154502  |
| crc32c_barret_naive_32wide                 |      152  |       52  |   622603  |      **0**|      **0**|      **0**|     5123  |    68609  |   548871  |
| crc32c_folding_naive_2x32wide              |      252  |       68  |   383740  |      **0**|      **0**|      **0**|     4104  |    67207  |   312429  |
| crc32c_barret_naive_mul                    |      128  |       24  |  1646602  |      **0**|      **0**|   262144  |     4099  |   266241  |  1114118  |
| crc32c_barret_naive_mul_32wide             |      148  |       44  |   428043  |      **0**|      **0**|    65536  |     5123  |    68609  |   288775  |
| crc32c_folding_naive_mul_2x32wide          |      236  |       68  |   252412  |      **0**|      **0**|    32832  |     4104  |    34375  |   181101  |
| crc32c_barret_sparse                       |      136  |       44  |  1679370  |      **0**|      **0**|   131072  |    20483  |   167937  |  1359878  |
| crc32c_barret_sparse_32wide                |      180  |       52  |   423947  |      **0**|      **0**|    32768  |     5123  |    44033  |   342023  |
| crc32c_folding_sparse_2x32wide             |      284  |       80  |   283192  |      **0**|      **0**|    16416  |     4104  |    22063  |   240609  |
| crc32c_barret_sparse_semirolled            |      180  |       52  |   933898  |      **0**|      **0**|   131072  |    20483  |    36865  |   745478  |
| crc32c_barret_sparse_semirolled_32wide     |      224  |       60  |   237579  |      **0**|      **0**|    32768  |     5123  |    11265  |   188423  |
| crc32c_folding_sparse_semirolled_2x32wide  |      356  |      104  |   177514  |      **0**|      **0**|    16416  |    22572  |     5647  |   132879  |
| crc32c_barret_sparse_unrolled              |      228  |       48  |   425994  |      **0**|      **0**|   131072  |    20483  |     4097  |   270342  |
| crc32c_barret_sparse_unrolled_32wide       |      272  |       56  |   110603  |      **0**|      **0**|    32768  |     5123  |     3073  |    69639  |
| crc32c_folding_sparse_unrolled_2x32wide    |      412  |       84  |    77992  |      **0**|      **0**|    16416  |     4104  |     1543  |    55929  |
| crc32c_barret_vmullp16                     |      108  |       24  |   147466  |     8192  |    57344  |      **0**|     4099  |     4097  |    73734  |
| crc32c_barret_vmullp16_32wide              |      152  |       32  |    40971  |     2048  |    14336  |      **0**|     1027  |     3073  |    20487  |
| crc32c_folding_vmullp16_2x32wide           |      264  |       60  |    34900  |     1026  |    10260  |      **0**|     3595  |     1543  |    18476  |
| crc32c_folding_vmullp16_4x32wide           |      376  |      120  |     8152  |     1028  |     1885  |      **0**|     1034  |    **783**|     3422  |
| crc32c_folding_vmullp16_8x16wide           |      316  |       72  |   **6364**|     1028  |     1886  |      **0**|    **266**|    **783**|   **2401**|
| crc32c_bitsliced_32x2x32wide               |      720  |      592  |   349726  |       66  |      660  |      **0**|    83330  |    51724  |   213946  |
| crc32c_bitsliced_64x2x32wide               |      780  |     1120  |   471760  |      130  |     1300  |      **0**|   131942  |    46076  |   292312  |
| crc32c_bitsliced_128x2x32wide              |      816  |     2132  |   283967  |      258  |    34581  |      **0**|    42662  |    22372  |   184094  |

## Takeaways

Hardware polynomial multiplication, even with MVE's limitations, offers a
significant improvement over existing methods of CRC calculation. Not only
is it ~80% faster than traditional table-based implementations, it requires
~70% less code-size.

While it may seem a bit silly to worry about 1KiB of code, the savings present here
likely also apply to other places where polynomial multiplication is used, mainly
error correction and cryptography.

## Which crc32c should I use?

Here are the top contenders:

|                                            |     code  |    stack  |      ins  |     vmul  |   vector  |      mul  |    ld/st  |   branch  |    other  |
|:-------------------------------------------|----------:|----------:|----------:|----------:|----------:|----------:|----------:|----------:|----------:|
| crc32c_naive                               |     **48**|       12  |   221192  |      **0**|      **0**|      **0**|     4099  |    36865  |   180228  |
| crc32c_naive_mul                           |     **48**|      **8**|   159752  |      **0**|      **0**|    32768  |     4099  |    36865  |    86020  |
| crc32c_small_table                         |      316  |       12  |    49160  |      **0**|      **0**|      **0**|    12291  |     4097  |    32772  |
| crc32c_table                               |     1064  |      **8**|    32776  |      **0**|      **0**|      **0**|     8195  |     4097  |    20484  |
| crc32c_folding_vmullp16_8x16wide           |      316  |       72  |   **6364**|     1028  |     1886  |      **0**|    **266**|    **783**|   **2401**|

- **crc32c_naive** - At only 48 bytes of code, the naive implementation of
  crc32c is the best option if code size is the priority and performance is
  completely irrelevant. Otherwise its performance is terrible.

- **crc32c_naive_mul** - A minor improvement if you have a single-cycle
  multiplier at no code-cost.

- **crc32c_small_table** - A small-table is my favorite default, providing
  decent performance without a noticable code-cost.

- **crc32c_table** - This is the most common implementation, and provides the
  best performance while also being completely portable.

- **crc32c_folding_vmullp16_8x16wide** - If you have MVE available, this is
  by far the best option in terms of both size and performance. If you don't
  have MVE available then this one won't work.

## -O3 Results

Usually Cortex-M devices stick to -Os, as the performance benefits of -O3 are
rarely worth the code-size tradeoff. As a curiosity, here are the results
with -O3. It's of course possible to exclusively compile only the crc32c code
with -O3 if it is a bottleneck for a given application.

Note that some of the non-vector implementations have been vectorized! These
results will likely be very different on non-MVE chips.

|                                            |     code  |    stack  |      ins  |     vmul  |   vector  |      mul  |    ld/st  |   branch  |    other  |
|:-------------------------------------------|----------:|----------:|----------:|----------:|----------:|----------:|----------:|----------:|----------:|
| crc32c_naive                               |    **112**|        8  |   147465  |      **0**|      **0**|      **0**|     4099  |     4097  |   139269  |
| crc32c_naive_32wide                        |      184  |       16  |   274440  |      **0**|      **0**|      **0**|     1027  |    35841  |   237572  |
| crc32c_naive_mul                           |      132  |        8  |   110600  |      **0**|      **0**|    32768  |     4099  |     4096  |    69637  |
| crc32c_naive_mul_32wide                    |      180  |       12  |   144392  |      **0**|      **0**|    32768  |     1027  |    35841  |    74756  |
| crc32c_small_table                         |      328  |      **4**|    40968  |      **0**|      **0**|      **0**|    12291  |     4096  |    24581  |
| crc32c_table                               |     1076  |      **4**|    24584  |      **0**|      **0**|      **0**|     8195  |     4096  |    12293  |
| crc32c_barret_naive                        |      140  |       24  |  2158604  |      **0**|      **0**|      **0**|     4100  |   266241  |  1888263  |
| crc32c_barret_naive_32wide                 |      244  |       28  |   542731  |      **0**|      **0**|      **0**|     1027  |    68609  |   473095  |
| crc32c_folding_naive_2x32wide              |      440  |       36  |   324079  |      **0**|      **0**|      **0**|    33858  |    66567  |   223654  |
| crc32c_barret_naive_mul                    |      576  |       72  |   380944  |    65536  |   163840  |      **0**|   102410  |     4097  |    45061  |
| crc32c_barret_naive_mul_32wide             |      968  |       76  |   103438  |    16384  |    40960  |      **0**|    27656  |     1025  |    17413  |
| crc32c_folding_naive_mul_2x32wide          |      400  |       48  |   309546  |      **0**|      **0**|    32832  |    35272  |    33863  |   207579  |
| crc32c_barret_sparse                       |      432  |       36  |   405516  |      **0**|      **0**|    65536  |     8198  |     4097  |   327685  |
| crc32c_barret_sparse_32wide                |      936  |       48  |   145425  |      **0**|      **0**|    20480  |     4104  |     1025  |   119816  |
| crc32c_folding_sparse_2x32wide             |     1620  |       56  |   105068  |      **0**|      **0**|    16416  |    13336  |     1029  |    74287  |
| crc32c_barret_sparse_semirolled            |      368  |       56  |   368654  |      **0**|      **0**|    65536  |    49158  |     4097  |   249863  |
| crc32c_barret_sparse_semirolled_32wide     |      780  |       64  |   123920  |      **0**|      **0**|    20480  |    15366  |     2049  |    86025  |
| crc32c_folding_sparse_semirolled_2x32wide  |     1384  |       88  |    96835  |      **0**|      **0**|    16416  |    28160  |     1543  |    50716  |
| crc32c_barret_sparse_unrolled              |      368  |       56  |   368654  |      **0**|      **0**|    65536  |    49158  |     4097  |   249863  |
| crc32c_barret_sparse_unrolled_32wide       |      792  |       64  |   126992  |      **0**|      **0**|    20480  |    17414  |     2049  |    87049  |
| crc32c_folding_sparse_unrolled_2x32wide    |     1372  |       88  |    98876  |      **0**|      **0**|    16416  |    28667  |     1543  |    52250  |
| crc32c_barret_vmullp16                     |      152  |        8  |    94226  |     8192  |    40965  |      **0**|     4100  |     4097  |    36872  |
| crc32c_barret_vmullp16_32wide              |      240  |       12  |    29714  |     2048  |    10244  |      **0**|     1027  |     2049  |    14346  |
| crc32c_folding_vmullp16_2x32wide           |      596  |       72  |    29250  |     1026  |     7196  |      **0**|     6652  |     1031  |    13345  |
| crc32c_folding_vmullp16_4x32wide           |      484  |      104  |     7597  |     1028  |     1865  |      **0**|     1034  |    **783**|     2887  |
| crc32c_folding_vmullp16_8x16wide           |      404  |       76  |   **5807**|     1028  |     1866  |      **0**|    **266**|    **783**|   **1864**|
| crc32c_bitsliced_32x2x32wide               |     4244  |      816  |   289306  |       66  |     5748  |      **0**|    54337  |    36028  |   193127  |
| crc32c_bitsliced_64x2x32wide               |     1400  |     1120  |   427416  |      130  |     1110  |      **0**|    88331  |    45976  |   291869  |
| crc32c_bitsliced_128x2x32wide              |     2592  |     2216  |   279884  |      258  |    34250  |      **0**|    44744  |    20666  |   179966  |



[crc]: https://en.wikipedia.org/wiki/Cyclic_redundancy_check
[gf256]: https://docs.rs/gf256/latest/gf256/crc/index.html
[intel-whitepaper]: https://www.intel.com/content/dam/www/public/us/en/documents/white-papers/fast-crc-computation-generic-polynomials-pclmulqdq-paper.pdf
[MVE]: https://www.arm.com/technologies/helium
