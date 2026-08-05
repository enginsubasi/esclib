# esclib backlog

Date: 2026-08-05

Candidate work, ordered by value. Each entry says what is missing, why it matters, and what the first concrete step is. Nothing here is committed work — this is the list to pick from.

## 1. Run the test suite once on a host compiler — DONE 05/08/2026

A host `gcc` (MinGW-w64, WinLibs) was installed and every test was built and run for the first time. All twenty-one build clean and pass, with no warnings from any test file, and every expected value that had been derived by reasoning rather than by running turned out to be correct.

The run found three things nothing else could have:

- `WriteToAFile.c` did not compile at all — it called `exit` without `<stdlib.h>`, which a modern compiler treats as an error rather than a warning, and declared `void main`.
- All six printing tests ended `main` with an unconditional `return ( 1 );`, so their exit status reported failure on every run.
- The assumed staleness of the checked-in `output.txt` files was mostly not real. `MAF_Test` and `Hysteresis_Test` matched to the digit; `EMAF_Test` differed in the last place of one value; only `PID_Test` had genuinely moved, from the July 2026 initial-state fix. What the old files actually differed by was whitespace — their tabs had been expanded to spaces, and the programs emit real tabs.

Fixed and regenerated in commits `315ebe6`, `ebdace6` and `bb5b864`.

**Still open from this item.** CLAUDE.md records that this repository deliberately has no build system and no test runner. The run above used a throwaway script that derives each test's module sources from its own `#include "..."` lines. Whether such a runner should live in the tree is an open decision, not something to settle by drift.

## 2. Protocol modules do not verify what they receive

**State.** `src/communication/comat.c` and `comstxetx.c` contain no CRC or checksum call at all. They frame bytes, time out, and hand the buffer to `packetProcess`. A corrupted frame that still has valid framing passes straight through.

**Why it matters.** This is the only entry on the list where a shipped module gives a wrong answer rather than merely lacking a convenience. `crc16` and `crc32` already exist in the tree.

**The design constraint.** Module independence forbids `comat.c` from including `crc16.h`. The integrity check therefore has to arrive the way every other dependency does in this library — as a function pointer installed at `Init`, alongside `packetProcess` and `txTransmissionTrigger`. That is a design decision, not a coding task: where in the frame the check sits, what width it is, what happens to a frame that fails it, and whether an existing caller that installs no checker keeps working unchanged.

**Companion.** A `checksum` module: LRC/XOR, sum8, sum16, Fletcher16, Adler32. Stateless, same signature shape as `crc16`, and the natural thing to install into the hook above when a full CRC is more than the link needs.

## 3. `interp` — table interpolation

**State.** `searchClosest` answers which table entry to read, and CLAUDE.md points at it as what a calibration or linearisation table needs. Nothing returns a value *between* two entries, so every caller writes that arithmetic itself.

**Shape.** A stateless value module like `complex`, not a driver: linear interpolation over an ascending x table, with the ends clamped.

**The trap.** It cannot include `search.h`. It has to carry its own bracketing search — a few lines, and a deliberate duplication rather than an oversight. Worth stating in the module's own banner so the next reader does not "fix" it.

## 4. `ramp` — setpoint profile with an acceleration limit

**State.** `slew` bounds the rate of change and nothing else — one derivative. `pidControl` takes an error that is already computed, so setpoint generation lives entirely outside the library.

**Shape.** A driver-struct module that walks a setpoint toward a target under an acceleration limit and reports arrival. Sits between `slew` and `pid`, and now has `softtimer` underneath it for anything time-based.

## 5. `encoder` — quadrature decoder

**State.** `bininp` gives a debounced level and a rising edge. Nothing turns two channels into a position count.

**Shape.** A driver-struct module fed the two debounced inputs, tracking direction and position. The natural feedback counterpart to `dcMotor`.

## 6. Scalar primitives in `basicmath`

**State.** `basicmath` operates on arrays only — min, max, sum, mean, median, range. There is no scalar clamp, no range remap, no interpolation between two values.

**Shape.** `mathClamp`, `mathMap`, `mathLerp` and their typed variants, added to the existing module rather than a new one. The cheapest item here, and the three lines most often rewritten by hand in an embedded project.
