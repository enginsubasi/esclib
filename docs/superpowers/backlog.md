# esclib backlog

Date: 2026-08-05

Candidate work, ordered by value. Each entry says what is missing, why it matters, and what the first concrete step is. Nothing here is committed work — this is the list to pick from.

## 1. Run the test suite once on a host compiler

**State.** Nothing in this repository has ever been executed. Twenty-one test programs, thirteen of them assert-style, verified only by compiling and linking with `arm-none-eabi-gcc`, which cross-compiles for ARM and cannot run its output.

**Why it leads.** The assert-style tests were written to fail loudly, and their expected values were derived from independent models — an IEEE binary32 transliteration for the float filters, the CRC polynomials for `CRC_Test`, a state-machine replay for `Protocol_Test`, hand simulation for `SoftTimer_Test`. That is the best that can be done without running them, and it is not the same thing. A wrong expectation looks exactly like a passing test to every reader.

**First step.** On a machine with `gcc` (MinGW or WSL both work), build and run each test: the test's own `main`, its module sources, and the module include directories. Each test's dependencies are derivable from its own `#include "..."` lines, so no list needs maintaining.

**Follow-on.** Five checked-in `output.txt` files are stale — `MAF_Test`, `EMAF_Test`, `PID_Test`, `Hysteresis_Test`, `WriteToAFile_Test`. They predate the July 2026 bug fixes and the August 2026 switch from `sqrt`/`atan2`/`cos`/`sin` to the `f` variants. Regenerate them in the same pass. Until then a diff against them is not evidence of a regression.

**Note.** CLAUDE.md records that this repository has no build system and no test runner, deliberately. A permanent runner script would change that property, so this is a one-off run unless the runner is adopted as a decision in its own right.

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
