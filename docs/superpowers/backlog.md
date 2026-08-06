# esclib backlog

Created 2026-08-05, last updated 2026-08-06.

Every item on this list has been built, and the one decision it carried is settled. Each entry records what was actually done and, where the outcome differed from the proposal, why.

## 1. Run the test suite once on a host compiler — DONE 05/08/2026

A host `gcc` (MinGW-w64, WinLibs) was installed and every test was built and run for the first time. All twenty-one build clean and pass, with no warnings from any test file, and every expected value that had been derived by reasoning rather than by running turned out to be correct.

The run found three things nothing else could have:

- `WriteToAFile.c` did not compile at all — it called `exit` without `<stdlib.h>`, which a modern compiler treats as an error rather than a warning, and declared `void main`.
- All six printing tests ended `main` with an unconditional `return ( 1 );`, so their exit status reported failure on every run.
- The assumed staleness of the checked-in `output.txt` files was mostly not real. `MAF_Test` and `Hysteresis_Test` matched to the digit; `EMAF_Test` differed in the last place of one value; only `PID_Test` had genuinely moved, from the July 2026 initial-state fix. What the old files actually differed by was whitespace — their tabs had been expanded to spaces, and the programs emit real tabs.

Fixed and regenerated in commits `315ebe6`, `ebdace6` and `bb5b864`.

**Settled 06/08/2026.** The throwaway script that run used became `run_tests.sh` at the repository root. See the last section.

## 2. Protocol modules do not verify what they receive — DONE 05/08/2026

`comstxetx` now frames binary and verifies what it receives. A payload byte equal to STX, ETX or the caller-chosen DLE travels preceded by DLE, so any byte value crosses the link; every frame carries a two-byte check computed over the unescaped payload by a function installed at `Init`, whose signature is `crc16`'s so `crc16` goes in with no wrapper; a frame that fails its check is dropped and counted in `comstxetxGetRejectCount`. `comstxetxBuildFrame` is the matching encoder, filling the transmit buffer the module already owned but never used, which is what keeps the two halves from drifting apart.

`comat` was deliberately left without one. It speaks AT, an ASCII command protocol whose real peers do not checksum, and adding one would invent a dialect nothing else speaks. A `@note` in `comat.c` records that so the next reader does not take it for an omission.

Implemented in commits `e6290d9`, `a559589`, `0e30dc6` and `f1b8fc2`. `Protocol_Test` grew from the old print-and-look case to 88 asserted checks, including a round trip that builds a frame carrying all three framing bytes and feeds the wire bytes back.

**The companion `checksum` module is done too, on 05/08/2026.** `checksumXor`, `checksumSum8`, `checksumSum16`, `checksumFletcher16` and `checksumAdler32`, stateless, in the `crc` group. Each returns its **own natural width** rather than a common one, so only the `uint16_t` pair fits the hook above — widening `checksumSum8` to sixteen bits would make it fit and would be a lie about how much protection it carries. Fletcher16 and Adler32 were checked against their published vectors rather than against this implementation, and `checksumFletcher16` was installed into `comstxetxInit` and round-tripped a payload carrying STX and DLE to prove the hook claim in situ. Implemented in `e4c9ce4`; `Checksum_Test` asserts 15 checks.

## 3. `interp` — table interpolation — DONE 05/08/2026

`interp` gives the value *between* two entries of an ascending table, which is the half of a calibration curve `searchClosest` does not answer.

It became a **driver rather than the stateless module this entry proposed**. The reason is the library's own rule: ascending order is a precondition here exactly as it is for the binary searches, and CLAUDE.md already records what a violated precondition costs. A stateless function has nowhere to check it and checking per call is O(N) on a per-sample path, so `interpInit` verifies strict ascent once, at boot, and `interpCalculate` divides without testing the divisor. Reverse lookup needed no function of its own — a second `interp_t` with the tables exchanged reads the curve backwards.

The bracketing search is duplicated from `searchUpperBound` on purpose, as this entry predicted, and the banner says so.

Design in `specs/2026-08-05-interp-design.md`, plan in `plans/2026-08-05-interp.md`, implemented in `ae1850a`, `08cd41a`, `484aba3` and `052cacf`. `Interp_Test` asserts 60 checks.

## 4. `ramp` — setpoint profile with an acceleration limit — DONE 05/08/2026

`ramp` walks a setpoint toward a target under both a velocity and an acceleration limit and comes to rest exactly on it. The braking point is not computed: `sqrtf ( 2 * a * remaining )` is the fastest the ramp could be going and still stop on the target, so deceleration starts by itself the moment the velocity meets that envelope.

The profile is trapezoidal rather than the pure acceleration limit this entry described. Without a velocity cap the peak speed of a long move grows without bound, and the cap cannot be added from outside — chaining `slew` after `ramp` would alter the velocity `ramp` computes its braking from.

`softtimer` is not underneath it after all. `ramp` takes `ts` at `Init` the way `pid` does and counts nothing itself, so it needs no timer.

Design in `specs/2026-08-05-ramp-design.md`, plan in `plans/2026-08-05-ramp.md`, implemented in `63479bd`, `aa9e82b` and `9e29302`. `Ramp_Test` asserts 55 checks.

## 5. `encoder` — quadrature decoder — DONE 06/08/2026

`encoder` turns two quadrature channels into a signed position at four counts per cycle, closing the loop `dcMotor` opens.

Its one real decision was what to do when both channels changed between two samples. A step was missed and its direction is unrecoverable, so the module refuses to guess: the position stands still and `encoderGetErrorCount` records it, the way `comstxetxGetRejectCount` counts a bad frame. A table answering ±2 there would look right on a clean signal and drift silently on a noisy one.

`encoderInit` takes the levels the pins are sitting at. That is not a convenience — without it the first `encoderUpdate` reads as a transition that never happened.

Implemented in `2e0fd20`. `Encoder_Test` asserts 44 checks, and all three of its claims were confirmed by mutation: guessing the missed step, ignoring the initial pin levels, and treating a masked register read as low.

## 6. Scalar primitives in `basicmath` — DONE 05/08/2026

`mathClamp` in all three widths, `mathMap` in float and `i32`, `mathLerp` in float. Added to `basicmath` rather than a new module, as this entry proposed.

`mathMap` and `mathLerp` deliberately do not clamp — a value outside the input range extrapolates, which is why `mathClamp` is separate rather than folded in. `mathMapi32` carries every intermediate in `int64_t` for `interp`'s reason, and differs from it in one place: its denominator may be negative, because unlike a table the input range is free to descend, so its round-to-nearest takes the magnitude of half the divisor first.

Implemented in `681e070` and `ae49b80`, tested inside the existing `Math_Test`, which now asserts 111 checks.

## Everything on this list is built

`softtimer`, the `comstxetx` transparency and integrity work, `checksum`, `interp`, the `basicmath` scalars, `ramp` and `encoder`. Nothing is outstanding.

The last open question — whether a test runner belongs in the tree — was **settled on 06/08/2026: it does.** The throwaway script that had run the suite for six modules became `run_tests.sh` at the repository root, and the "no runner" line in CLAUDE.md was rewritten rather than left to quietly contradict the tree.

It went in because it costs nothing to keep true. Each test's module dependencies come from its own `#include "..."` lines, so there is no list to fall out of sync and a new module needs no edit to it. It had also already earned its keep by catching two things nobody was looking for: an `output.txt` comparison that looked like staleness and was really a line-ending artefact, and `WriteToAFile_Test`'s `output.txt` being the file the program *writes* rather than its stdout, which made the comparison meaningless.

Two things were fixed before it was checked in, neither of which mattered while it lived in a scratch directory. It used a `<( ... )` process substitution, which is a bashism that works under Git Bash and fails under a real POSIX `sh`; it is now written to `sh` and verified under `dash`. And it now deletes the stray `output.txt` that `WriteToAFile_Test` drops into the working directory, which a runner living in the repository would otherwise cause to be committed by accident.

## What is not here

This list held only additions. Nothing on it was a defect, and the four stub files — `comsec`, `comsafe`, `comgenbuf`, `matrixlib` — are deliberately still stubs; CLAUDE.md's "Known gaps" section is where those live, not here.
