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
| `fir` | Shapes a response from caller-supplied taps, with linear phase no IIR can give. | `i32` (Q16) |
| `goertzel` | Does not filter — measures how much of one frequency is present. | — |

Pick by what is wrong with the signal: noise that averages out wants `maf` or
`emaf`, noise that does not wants `median`, a specific frequency wants `biquad`,
a jumpy actuator wants `slew`, a twitching display wants `deadband`.

`maf` and `fir` are the same filter: `maf` is a rectangular window kept as a
running sum, O(1) per sample whatever its length, and `fir` lets you choose the
taps at O(N). Choose `fir` over `biquad` when the *shape* of a waveform matters
rather than its level — a symmetric tap set has exactly linear phase, which no
IIR can give.

`goertzel` is the other half of `biquad`'s notch: the notch removes a tone, this
measures one. One bin for two multiplies and two adds a sample, no buffer, which
beats an FFT whenever the frequency of interest is known in advance.

`sched` is `softtimer` for N jobs instead of one. The split matters: `schedTick`
only counts and marks from the ISR, `schedRun` calls the tasks from the main
loop — a scheduler that dispatched from inside the tick would run a display
update in an interrupt. `fsm` is the general form of the machines `comat` and
`comstxetx` write by hand, with the transitions as a `const` table in flash.

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
| `basicmatrix` | 1D and 2D thresholding and limiting on a bare array. | `i32` `u32` `u8` |
| `matrixlib` | Linear algebra on a matrix that carries its shape: add, multiply, transpose, invert. | — |
| `complex` | Complex arithmetic and the polar conversions. | `i32` (Q16, arithmetic only) |
| `q16` | Q16 fixed-point arithmetic for the caller: multiply, divide, square root, conversions. | — |
| `cordic` | Sine, cosine, arc tangent, magnitude and vector rotation in fixed point, with no float anywhere. | — |

`mathMap` and `mathLerp` deliberately do not clamp — a value outside the input
range extrapolates, which is why `mathClamp` is separate rather than folded in.
`mathClamp` carries all three widths; `mathMapi32` and `mathLerpi32` carry the
integer half of the other two, and take their fraction in Q16.

`q16` is for the caller, not for the library. Five modules here carry a Q16
variant and by the independence rule none of them may include it — but a caller
who now holds gains and limits in Q16 had nothing to do arithmetic on them with.
Five functions, each because the hand-written version gets one specific thing
wrong: a multiply needs a 64-bit intermediate, a divide needs the scale applied
*before* the division, a conversion back has to round, and there is no square
root at all without a float. Everything saturates rather than wrapping, because
a wrapped Q16 value changes sign. No float appears in it anywhere.

`cordic` is for the caller in the same way, and it exists because two absences
in this library were written down rather than filled: `complexi32` has no polar
pair and `biquad` has no `i32` designer, and both of those wanted a sine, a
cosine and an arc tangent that do not link the software float library. The
angle is a **binary angle** — the whole turn is 2^32 in a `uint32_t` — so it
wraps by itself, there is no pi to represent, and a frequency as a fraction of
the sample rate *is* that fraction of a turn: a corner at 50 Hz in a 1 kHz loop
is `( 50u * 4294967296u ) / 1000u` with no pi in the expression. Twenty
iterations, measured rather than assumed: the sine and cosine land within one
count of 65536, the angle within a ten-thousandth of a degree. It needs no
divide at all — `__aeabi_lmul` and a shift are the only helpers it pulls in,
where `q16` and `interp` each need `__aeabi_ldivmod`.

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
| `crc32` | CRC-32/MPEG-2, table driven (`crc32`) and bit by bit (`crc32Alt`). The table is a kilobyte; the loop is 56 bytes. |
| `checksum` | `checksumXor`, `checksumSum8`, `checksumSum16`, `checksumFletcher16`, `checksumAdler32`, each returning its own natural width. |

`xor` and the two sums are blind to a reordering; `Fletcher16` sees it for
nearly the cost of a plain sum; `Adler32` is stronger on long payloads and
notably weak on short ones.

### Communication — `inc/communication`

| module | what it is for |
|---|---|
| `comstxetx` | Binary framing with DLE escaping, so any byte value crosses the link, and a two-byte check installed at `Init` — `crc16` or `checksumFletcher16` go in directly. A bad frame is dropped and counted. |
| `comat` | AT command protocol, ASCII, no check by design. |
| `commodbus` | Modbus RTU framing: silence-delimited frames, address filtering, the check installed at `Init` — `crc16` is already the right polynomial. Framing only, no function codes. |
| `comgenbuf` | Queue of variable-length packets in one flat buffer. Keeps the packet boundaries `circBuf` loses. |
| `comsafe` | Black-channel safety framing over an untrusted transport: connection id, sequence number, independent check, watchdog. |
| `comsec` | Authenticated framing with replay protection. The MAC is injected; the module holds no cryptography and provides no confidentiality. |

The first three are byte-driven state machines: `xxxReceive` per byte from the
ISR, `xxxEvaluate` from the main loop, `xxxTimeoutCounter` from a periodic tick.

`commodbus` is the odd one of those three, because RTU has **no start byte and
no end byte** — a frame is delimited by silence, three and a half character
times of an idle line. So the tick is not a timeout here, it is the framing
itself, and the silence is expressed in ticks the way `softtimer`'s periods
are: the module is never told the baud rate, because converting character times
into ticks is one expression that belongs where the baud rate is configured.
It is framing and nothing above it — no function codes, no register map, no
exception responses, because a register map is the application's data and a
table of handlers is what `fsm` already is. Two counters, not one: a frame for
another device is **ignored**, a frame that failed its check is **rejected**,
and keeping them apart is what leaves the reject count meaning "the line is
bad" on a bus where most traffic is addressed to somebody else.

`comgenbuf` is the layer above them. `circBuf` is a queue of bytes and loses
where one packet ends and the next begins; this stores a two-byte length with
each payload, so a frame the ISR assembled comes back whole. A push is all or
nothing, and a full queue refuses the newest packet rather than dropping the
oldest — the opposite of `circBuf`'s overwrite mode, because dropping a packet
already accepted loses a message the caller was told it would get.

`comsafe` sits above all of them. It treats the transport as an opaque black
channel and carries end to end what has to be relied on: a connection id, a
sequence number, an integrity check installed at `Init` (so it can differ from
the transport's own), and a watchdog for the case where nothing arrives at all.
**It is framing, not cryptography** — it catches accidental corruption, loss,
repetition and reordering, and nothing about it resists an attacker, which
would need a MAC and a key it does not have.

`comsec` is where that MAC goes. It carries a session id, a 32-bit counter and
a tag computed by a function installed at `Init`, so the module itself contains
no cryptography at all — the same hook `comstxetx` uses for its check. It gives
**authenticity, integrity and replay protection, and no confidentiality**: the
payload travels in the clear, and a caller who needs secrecy encrypts before
handing it over. Its replay rule is *strictly greater*, not *next*, because a
link may lose frames legitimately; the counter does not wrap, because two frames
under one key and counter break every guarantee at once.

### Buffers, timing and I/O

| module | group | what it is for |
|---|---|---|
| `circBuf` | `inc/buffer` | Circular buffer in `u8`, `u32` and `i32`, overwrite or stop on full. |
| `softtimer` | `inc/timer` | One-shot and periodic timers counting calls to `softtimerTick`. The library's only time abstraction. |
| `sched` | `inc/timer` | Table of periodic tasks. The tick marks them due, the main loop runs them, and a task that could not keep up is counted. |
| `fsm` | `inc/fsm` | Table-driven state machine. An event no row accepts is counted, never guessed at. |
| `bininp` | `inc/bininp` | Debounced binary input with a rising-edge flag. |
| `encoder` | `inc/encoder` | Quadrature decoding at four counts per cycle. A missed step is counted, never guessed. |
| `logic` | `inc/logic` | D and RS flip-flops. |
| `pack` | `inc/pack` | Multi-byte values to and from a byte buffer, either endianness, with the signed and 24-bit readers that C has no type for. |

`pack` is the line the two protocol modules leave to their caller. `comstxetx`
hands over a payload of bytes; turning four of them into a reading is where the
hand-written version goes wrong, so it lives here instead. Nothing in it casts a
buffer pointer to a wider type — that assumes the machine's endianness, assumes
alignment a Cortex-M0 will fault on, and aliases.

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
sh run_tests.sh Ramp_Test    # or just one
sh scripts/check.sh          # warnings, headers, symbol coverage, static storage
STRICT=1 sh scripts/check.sh # the same, under -Wconversion and its neighbours
sh scripts/mutate.sh         # every known defect still fails the test that pins it
sh scripts/size.sh           # code size per module
sh scripts/runtime.sh        # which compiler runtime helpers each module needs
sh scripts/samples.sh        # build and run every example under sample/
```

## Examples

`sample/` holds three worked examples that use the library, each one a stack
rather than a single call:

| example | what it puts together |
|---|---|
| `FilteredScale` | `pack` → `median` → `biquad` → `interp`. A 24-bit converter read, one bad sample thrown away, mains hum notched out, counts turned into kilograms. |
| `MotionLoop` | `ramp` → `encoder` → `pid` → `dcMotor`. Four modules that disagree about what a number means, and the conversions between them. |
| `FramedLink` | `comstxetx` + `crc16` + `comgenbuf`. The ISR/main-loop split, with a payload that contains the framing bytes themselves. |

The hardware is faked in each, so they build and run anywhere; on a real board
that one function is all that changes. The other directories under `sample/`
teach C rather than this library and predate it.

The whole library is 17 kB of code on a Cortex-M0 at `-Os`, and **zero bytes of
RAM** — the caller owns every buffer, which `scripts/check.sh` enforces rather
than assumes.

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

Every module has a test, and every one of the 342 exported symbols is
referenced by at least one of them. There are no reserved names and no stubs
left: `comgenbuf`, `matrixlib`, `comsafe` and `comsec` were placeholders from
2022 and 2023 and were implemented on 15/09/2026.

Sixty-eight mutations reintroduce a known defect each and require the test that
pins it to fail; `sh scripts/mutate.sh` runs them and CI gates on the result.
The whole library is about 20 kB of code on a Cortex-M0 at `-Os` and **zero
bytes of RAM**.

## License

See `LICENSE`.
