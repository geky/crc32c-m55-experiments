
A comparison of some [crc32c][crc] implementations that may target
microcontrollers. The techniques here are mostly explained in [gf256][gf256].

The original motivation for this was the introduction of polynomial
multiplication instructions in the [MVE extensions][MVE] introduced in
Cortex-M55. MVE introduces `vmull.p8` and `vmull.p16`, 8x8-bit and 4x16-bit
widening multiplication instructions.

Polynomial multiplication as a form of CRC acceleration has become popular
recently, and on Cortex-M it has the extra benefit of replacing lookup tables
and reducing code size, which is often more a priority than performance on
these devices.

## Comparison

Each implementation can be found in its own .c file. I'm comparing code size
and executed instructions to get a rough comparison. This was compiled with
GCC 11 -Os -mcpu=cortex-m55.

To get a rough estimate of instructions executed I used a hacking GDB script
to step through each implementation with pseudo-random data of size 512 bytes.
I realize this is NOT cycle-accurate and not the best indicator of performance,
but it's was certainly the easiest measurement to perform, and possible with
open-source tooling. I may or may not (probably not) investigate further.

These implementations probably aren't super-optimal, but certainly usable.

## Results

|                                      |      code   |       ins   |      mul   |     ld/st   |    branch   |     other   |
|:-------------------------------------|------------:|------------:|-----------:|------------:|------------:|------------:|
| crc32c_naive                         |      **48** |     27656   |      **0** |       516   |      4609   |     22531   |
| crc32c_naive_words                   |        88   |     26376   |      **0** |     **132** |      4481   |     21763   |
| crc32c_mul                           |      **48** |     19976   |     4096   |       516   |      4609   |     10755   |
| crc32c_mul_words                     |        92   |     18312   |     4096   |     **132** |      4481   |      9603   |
| crc32c_small_table                   |       316   |      6152   |      **0** |      1540   |       513   |      4099   |
| crc32c_table                         |      1064   |      4104   |      **0** |      1028   |       513   |    **2563** |
| crc32c_barret_naive                  |       132   |    303114   |      **0** |       516   |     33281   |    269317   |
| crc32c_barret_naive_words            |       156   |     77963   |      **0** |       644   |      8577   |     68742   |
| crc32c_barret_mul                    |       128   |    205834   |    32768   |       516   |     33281   |    139269   |
| crc32c_barret_mul_words              |       152   |     53643   |     8192   |       644   |      8577   |     36230   |
| crc32c_barret_sparse                 |       136   |    209930   |    16384   |      2564   |     20993   |    169989   |
| crc32c_barret_sparse_words           |       184   |     53131   |     4096   |       644   |      5505   |     42886   |
| crc32c_barret_sparse_unrolled        |       228   |     53258   |    16384   |      2564   |       513   |     33797   |
| crc32c_barret_sparse_unrolled_words  |       276   |     13963   |     4096   |       644   |     **385** |      8838   |
| crc32c_barret_vmullp                 |       108   |     18442   |     1024   |       516   |       513   |     16389   |
| crc32c_barret_vmullp_words           |       156   |      5259   |      256   |     **132** |     **385** |      4486   |
| crc32c_barret_vmullp_flattened       |       120   |      7187   |     1024   |       516   |       513   |      5134   |
| crc32c_barret_vmullp_flattened_words |       200   |    **3987** |      256   |     **132** |     **385** |      3214   |

## Takeaways

Polynomial multiplication, _despite_ the limitations in its MVE implementation,
is a net benefit compared to existing methods of CRC calculation. Not only is
it as fast, if not faster, than table-based implementations, but is requires 80%
less flash.

While it may seem a bit silly to worry about 1KiB of code, the savings present here
likely also apply to other places where polynomial multiplication is used, mainly
error correction and cryptography.

## Which crc32c should I use?

Here are the top contenders:

|                                      |      code   |       ins   |      mul   |     ld/st   |    branch   |     other   |
|:-------------------------------------|------------:|------------:|-----------:|------------:|------------:|------------:|
| crc32c_naive                         |      **48** |     27656   |      **0** |       516   |      4609   |     22531   |
| crc32c_small_table                   |       316   |      6152   |      **0** |      1540   |       513   |      4099   |
| crc32c_table                         |      1064   |      4104   |      **0** |      1028   |       513   |    **2563** |
| crc32c_barret_vmullp_flattened_words |       200   |    **3987** |      256   |     **132** |     **385** |      3214   |

- **crc32c_naive** - At only 48 bytes of code, the naive implementation of
  crc32c is the best option if code size is the priority and performance is
  completely irrelevant. Otherwise its performance is terrible.

- **crc32c_small_table** - A small-table is my favorite default, providing
  decent performance without a noticable code cost.

- **crc32c_table** - This is the most common implementation, and provides the
  best performance while also being completely portable.

- **crc32c_barret_vmullp_flattened_words** - If you have MVE available, this
  is the best option in terms of both size and performance (probably, I'm only
  using a heuristic to compare). If you don't have MVE available then this one
  won't work.


[crc]: https://en.wikipedia.org/wiki/Cyclic_redundancy_check
[gf256]: https://docs.rs/gf256/latest/gf256/crc/index.html
[MVE]: https://www.arm.com/technologies/helium
