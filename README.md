# esclib

[![ci](https://github.com/enginsubasi/esclib/actions/workflows/ci.yml/badge.svg)](https://github.com/enginsubasi/esclib/actions/workflows/ci.yml)

A freestanding general-purpose C library for embedded targets.

github.com/enginsubasi/esclib/

No heap, no operating system, no build system. `<stdint.h>` types throughout,
C89-compatible style, and every module is independent — no module includes
another module's header — so a module is consumed by copying its `.h`/`.c` pair
into a target project.

```c
#include "maf.h"

static float   window [ 8 ];
static maf_t   filter;

if ( mafInit ( &filter, window, 8, 0.0f ) == TRUE )
{
    mafIteration ( &filter, sample );
    value = mafGetOutput ( &filter );
}
```

Every stateful module has that shape: the caller owns the storage, `Init`
validates and returns `TRUE` or `FALSE`, and hardware is injected as function
pointers rather than called directly. See `rules.md`.

## Layout

```
inc/<group>/<name>.h   <-->  src/<group>/<name>.c   strict 1:1 pair
drv/<name>.h, <name>.c                              hardware drivers
template/                                           copy these to start a module
test/<Name>_Test/                                   one standalone main() per module
sample/                                             standalone examples, not library code
```

## Modules

The width column lists the variants each module carries beside its `float` one.
An `i32`/`u32` suffix on a function name selects the variant.

### Filters — `inc/filter`

| module | what it is for | widths |
|---|---|---|
| `maf` | Moving average. Smooths, at the cost of a window of delay. | `i32` `u32` |
| `emaf` | Exponential moving average. Smooths with one state word. | `i32` `u32` |
| `median` | Rejects impulses outright rather than averaging them in. | `i32` `u32` |
| `biquad` | Shapes a response in hertz. The only way to notch mains hum. | `i32` (Q16) |
| `slew` | Bounds the rate of change. No target, chases the last sample. | `i32` `u32` |
| `deadband` | Holds the output still until the input really moves. | `i32` `u32` |
| `alphabeta` | Estimates position *and* velocity from position alone. | `i32` (Q16) |

Pick by what is wrong with the signal: noise that averages out wants `maf` or
`emaf`, noise that does not wants `median`, a specific frequency wants `biquad`,
a jumpy actuator wants `slew`, a twitching display wants `deadband`.

`biquad`'s four designers — low pass, high pass, band pass, notch — take a
corner in hertz and are float only. The `i32` variant takes the five
coefficients they produce, converted to Q16 once on a host, because computing a
cosine at boot would pull the whole software float library onto the part the
variant exists to serve.

### Control — `inc/control`

| module | what it is for | widths |
|---|---|---|
| `pid` | PID with separate output and integral limiters. | `i32` (Q16) |
| `hysteresis` | Two-threshold on/off control. | `i32` `u32` |
| `ramp` | Walks a setpoint to a target under velocity *and* acceleration limits, and stops exactly on it. | `i32` (Q16) |

`ramp` is what `slew` cannot be: `slew` bounds one derivative and has no target,
`ramp` bounds two and comes to rest on one.

### Math — `inc/math`, `inc/array`, `inc/matrix`, `inc/complex`

| module | what it is for | widths |
|---|---|---|
| `basicmath` | Array min, max, sum, mean, median, range; and the scalars `mathClamp`, `mathMap`, `mathLerp`. | `i32` `u32` |
| `statistic` | Variance, standard deviation, covariance. | `i32` `u32` |
| `interp` | The value *between* two entries of an ascending table. Calibration and linearisation curves. | `i32` |
| `basicarray` | 1D array limiting. | `i32` `u32` |
| `basicmatrix` | 1D and 2D thresholding and limiting. | `i32` `u32` `u8` |
| `complex` | Complex arithmetic and the polar conversions. | — |

`mathMap` and `mathLerp` deliberately do not clamp — a value outside the input
range extrapolates, which is why `mathClamp` is separate rather than folded in.
`mathClamp` carries all three widths; `mathMapi32` and `mathLerpi32` carry the
integer half of the other two, and take their fraction in Q16.

### Sort and search — `inc/sort`, `inc/search`

| module | what it is for | widths |
|---|---|---|
| `sort` | `sortInsertion`, `sortHeap`, `sortSelection`, `sortBubble`, plus `sortReverse` and `sortIsSorted`. | `i32` `u32` |
| `search` | `searchLinear`, `searchBinary`, `searchLowerBound`, `searchUpperBound`, `searchClosest`. | `i32` `u32` |

`sortInsertion` wins on short or nearly-sorted arrays and is stable; `sortHeap`
is the only one with an O(N log N) *guarantee* and does not recurse. Every sort
produces ascending order, and `sortReverse` turns that into descending in one
pass.

`searchBinary` answers whether a value is present, the bound pair answers where
it belongs, and `searchClosest` answers which entry to read. All of the binary
ones require ascending order and give a confident wrong answer without it, which
is what `sortIsSorted` is for.

### Integrity — `inc/crc`

| module | what it is for |
|---|---|
| `crc8` | SMBus packet error code (`crc8`) and Dallas 1-Wire (`crc8Dallas`). Two polynomials, not two spellings of one. |
| `crc16` | MODBUS CRC-16, table driven (`crc16`) and bit by bit (`crc16Alt`). |
| `crc32` | CRC-32. |
| `checksum` | `checksumXor`, `checksumSum8`, `checksumSum16`, `checksumFletcher16`, `checksumAdler32`, each returning its own natural width. |

`xor` and the two sums are blind to a reordering; `Fletcher16` sees it for
nearly the cost of a plain sum; `Adler32` is stronger on long payloads and
notably weak on short ones.

### Communication — `inc/communication`

| module | what it is for |
|---|---|
| `comstxetx` | Binary framing with DLE escaping, so any byte value crosses the link, and a two-byte check installed at `Init` — `crc16` or `checksumFletcher16` go in directly. A bad frame is dropped and counted. |
| `comat` | AT command protocol, ASCII, no check by design. |

Both are byte-driven state machines: `xxxReceive` per byte from the ISR,
`xxxEvaluate` from the main loop, `xxxTimeoutCounter` from a periodic tick.

### Buffers, timing and I/O

| module | group | what it is for |
|---|---|---|
| `circBuf` | `inc/buffer` | Circular buffer in `u8` and `u32`, overwrite or stop on full. |
| `softtimer` | `inc/timer` | One-shot and periodic timers counting calls to `softtimerTick`. The library's only time abstraction. |
| `bininp` | `inc/bininp` | Debounced binary input with a rising-edge flag. |
| `encoder` | `inc/encoder` | Quadrature decoding at four counts per cycle. A missed step is counted, never guessed. |
| `logic` | `inc/logic` | D and RS flip-flops. |

### Drivers — `drv/`

| driver | what it is for |
|---|---|
| `hc595` | Serial-in parallel-out shift register. |
| `hc597` | Parallel-in serial-out shift register. |
| `dcMotor` | H-bridge direction and duty cycle. A reversal zeroes the duty before the pins move. |

Both shift-register drivers offer two mutually exclusive transfer modes on one
struct: `xxxOneShot` blocks and paces itself with injected delay callbacks,
`xxxStart` plus `xxxInterrupt` from a fixed-rate ISR does the same transfer
without blocking. Only one may hold the driver at a time, and the driver
enforces that rather than trusting the caller.

## Building

There is nothing to build. Copy the pair you need and add the `.c` to your own
project. The two scripts in the tree build only the tests and ship nothing.

```bash
sh run_tests.sh              # build and run every test with a host compiler
sh scripts/check.sh          # warnings, header coexistence, symbol coverage
```

`run_tests.sh` derives each test's module dependencies from its own `#include`
lines, so neither script needs an edit when a module is added.

```bash
# Link check with a cross compiler, without running anything
CC=arm-none-eabi-gcc CFLAGS=--specs=nosys.specs LINKONLY=1 sh run_tests.sh
```

## Documentation

| file | what is in it |
|---|---|
| `rules.md` | Library rules: freestanding constraints, module independence, the driver-struct pattern, width variants, testing. |
| `codingReference.md` | Coding style, the naming and `Init` contracts, the Doxygen convention. |
| `Doxyfile` | `sh scripts/doc.sh` generates the API reference into `doc/`, which is not checked in. |

## Status

Every module with functions has a test, and every exported symbol is referenced
by at least one of them. `comsec`, `comsafe`, `comgenbuf` and `matrixlib` are
reserved names with no implementation — each of their headers opens with a
Doxygen `@warning` saying so.

## License

See `LICENSE`.
