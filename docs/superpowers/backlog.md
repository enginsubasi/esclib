# esclib backlog

Created 2026-08-05, last updated 2026-09-15.

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

## 7. Widths, and the Q16 variants — DONE 06/08/2026

Not proposed here — this entry records it because nothing else did, and eight commits of it went in unremarked after the list above was closed.

The plain integer half was finished first: `mafIterationi32`, `slewIterationu32`, `deadbandIterationu32`, both integer widths of `hysteresis`, and `statCovarianceu32`, which was the last hole in `statistic`. `deadbandInitu32` was also fixed to accept a zero threshold, which the other two widths already did.

Then three modules whose arithmetic is not integer arithmetic got a **Q16 fixed point variant**: `pid`, `ramp` and `alphabeta`. The scale is 65536 for 1.0 and it sits on the tuning parameters rather than on the signal, so a caller wires an ADC reading in unscaled. None of the three carries a `ts`: limits and gains are per sample, and the caller folds the period in once instead of paying a float multiply per call, which is the arithmetic the variant exists to avoid.

`ramp` was the one with a real question in it, and the answer was better than expected. `sqrtf ( 2 * a * remaining )` is the whole module, and an integer square root over a Q32 product returns a Q16 value — the scale falls out of the root for free, so the fixed point brake point is the float one rather than an approximation of it. The product is the variant's range limit and `ramp.c` records where it overflows.

Implemented in `23f4f8e`, `f00190d`, `b2b46f3`, `4cde7b0`, `c769d66`, `81ae139`, `ded0885` and `bdb2ced`. The exported symbol count went from 220 to 251, and every one of the new ones is covered by the existing tests.

## 8. The tree said things about itself that were no longer true — DONE 15/09/2026

Five weeks after the work above, none of it was written down. CLAUDE.md still claimed 220 exported symbols, the backlog still ended with "nothing is outstanding", and the word Q16 appeared nowhere outside the three file banners. A general review found that and four smaller things, and all of them are fixed:

- **CLAUDE.md** now carries the widths and the Q16 scale as a section of the module narrative, and its counts match the tree.
- **`rules.md`** was an empty placeholder and is now the architectural half of the rules — freestanding constraints, module independence, the driver-struct pattern, time, widths, preconditions, testing. `codingReference.md` keeps the code-level half and neither repeats the other.
- **`README.md`** was nine lines with no module list, for a library whose whole consumption model is picking one module out of thirty-four. It is now a module map with a width column.
- **`scripts/check.sh`** replaces the three command blocks CLAUDE.md asked a reader to paste, and adds a prefix check the tree had never had. **`scripts/doc.sh`** generates the Doxygen reference into the already ignored `doc/`; `WARN_IF_UNDOCUMENTED` went off in the Doxyfile because headers stay pure declarations by convention, which was 295 warnings of noise hiding a clean tree.
- **`.github/workflows/ci.yml`** runs both scripts, the tests and the cross link on every push. The five week gap between the work and its record is exactly what CI is for.
- **`src/communication/comsec.c`** was a zero byte file. CLAUDE.md described it as holding a banner, which is what it holds now.
- **`.gitattributes`** now pins `*.sh` to LF. Without it a fresh clone on Windows checks the scripts out with CRLF, which `dash` will not run — the one thing that would have made the runner useless on the machine it was written on.

## 9. `biquad` was the last float only filter — DONE 15/09/2026

Every other filter in the tree carried an integer width. `biquad` did not, which meant a part with no FPU could have the whole filter set except the one that shapes a response in hertz — and the notch has no substitute anywhere else in the library.

`biquadIniti32`, `biquadIterationi32`, `biquadGetOutputi32` and `biquadReseti32`, Q16, state in `int64_t`. Two decisions in it were settled by measuring against an independent model rather than by argument:

- **No `i32` designer.** The four designers turn hertz into coefficients with a `cosf` and a `sinf`. Running that at boot on an FPU free part links the software float library, which costs more than the filter saves, and the design is a compile time constant in every real use. The `i32` width takes the five coefficients; the file gives the one line that converts them.
- **The shift rounds, where `emafi32` and `alphabetai32` truncate.** A truncating shift loses half an LSB toward minus infinity every sample. Measured: a symmetric sine through a truncating Q16 low pass leaves a standing offset of half a count, and the rounded form leaves none. The test pins it as a sum over forty thousand samples and the mutation was confirmed — putting the plain shift back fails that check and only that check.

The same measurement found Q16's floor, which is documented rather than guarded: a low pass narrower than about one part in five hundred of the sample rate quantizes `b0` to a handful of LSBs and its dc gain goes visibly wrong. A notch is unaffected at any q.

`FilterSet_Test` grew forty checks, including one that designs the filter with the float `biquadInitLowPass` and asserts the Q16 literals the rest of the case uses are what it converts to — so the two widths cannot drift apart silently.

## 10. `mathLerp` was the last scalar with only a float width — DONE 15/09/2026

`mathClamp` had all three widths and `mathMap` had `i32`; `mathLerp` had neither, which left the smallest of the three scalars as the one an integer project still had to write by hand.

`mathLerpi32` takes `t` in Q16 and leaves `from` and `to` in plain units, the same split the fixed point filters use. Its product is `int64_t` — a full scale `t` against the span of `int32_t` needs more than thirty two bits — and its division rounds to nearest by `mathMapi32`'s own form, because a truncating lerp walked across a range in steps drifts steadily behind: the error carries the same sign at every step.

No `u32` variant, and that is the answer rather than an omission. `to - from` is signed whichever way the two are ordered, so an unsigned one would form its difference in a wider signed type anyway and would differ from this only in the type of its arguments.

`Math_Test` asserts eleven checks on it, every expected value worked out by hand before the function was run, including the two that tell rounding from truncation and the two that overflow a thirty two bit intermediate.

## 11. Three holes in what was already here — DONE 15/09/2026

A review asked what the library should grow, and the first answer was that three things it already claimed were incomplete.

**`dcMotor` could set a direction but not a speed.** `dcmotor_t` required a `pwm` callback at `Init`, called it once with zero and never touched it again — an injected callback the module never used, which has no other example in the tree. `dcMotorSetSpeed` and `dcMotorGetSpeed` close it.

Two decisions came with it. The duty is **clamped rather than rejected**, because a status every speed update would have to check is a status nobody checks, and `dcMotorGetSpeed` reports what was actually installed. The clamp is written so that anything not above zero lands on zero, which puts a `nan` at a standstill instead of through to the hardware — the same defect `pidInit` guards by rejecting a zero `ts`, found here by asking what the obvious two-sided clamp would do.

And **a reversal between two driven directions now zeroes the duty before any pin moves.** A motor turning at speed is a generator, and throwing the bridge across it puts the supply and the back emf in series through the winding. Once the driver owns the duty it owns that hazard. The duty is *not* restored afterwards: putting the previous torque back one PWM period later is the same hazard with a delay on it. Nothing else is a reversal — releasing, locking, and taking a direction up from a released bridge all leave the duty alone, which is what keeps the existing assertion that `BRIDGE_FORWARD` does not touch the PWM true. Both claims were confirmed by mutation.

**`crc8` was missing.** `crc16` and `crc32` were there. Two functions rather than one, because `crc8` and `crc8Dallas` are two polynomials — SMBus PEC and Dallas 1-Wire — and neither substitutes for the other. Both bitwise, no table: 256 bytes of flash to speed up a three-byte transaction is the wrong trade. Checked against the published check values, 0xF4 and 0xA1 over "123456789", and against a real DS18B20 ROM whose eighth byte the first seven must produce and whose whole eight must check to zero.

**Nothing turned a byte buffer into a number.** `comstxetx` delivers a payload and `comat` a string; the caller wrote the rest by hand, every project, and got it wrong. `pack` is sixteen stateless functions in a new group. No signed writers, because signed to unsigned of the same width is defined to wrap and a cast at the call site is already right; signed *readers*, because the reverse is implementation-defined before C23, so they sign-extend by subtraction. 24-bit readers with no writers, because twenty-four bits is a converter width — and `packGetI24be` is what the module is really for, pinned in the test with a real HX711 pattern for minus ten counts, the reading a forgetful implementation reports as 16777206.

`DcMotor_Test` gained twenty-one checks, `CRC_Test` twenty, and `Pack_Test` is new with forty-four. Every expected value was written from the byte pattern or the polynomial, not from a run.

## Everything on this list is built

`softtimer`, the `comstxetx` transparency and integrity work, `checksum`, `interp`, the `basicmath` scalars, `ramp` and `encoder`, and after them the widths and the Q16 variants, `biquad`'s and `mathLerp`'s included, and then `dcMotor`'s speed control, `crc8` and `pack`. Nothing is outstanding.

The last open question — whether a test runner belongs in the tree — was **settled on 06/08/2026: it does.** The throwaway script that had run the suite for six modules became `run_tests.sh` at the repository root, and the "no runner" line in CLAUDE.md was rewritten rather than left to quietly contradict the tree.

It went in because it costs nothing to keep true. Each test's module dependencies come from its own `#include "..."` lines, so there is no list to fall out of sync and a new module needs no edit to it. It had also already earned its keep by catching two things nobody was looking for: an `output.txt` comparison that looked like staleness and was really a line-ending artefact, and `WriteToAFile_Test`'s `output.txt` being the file the program *writes* rather than its stdout, which made the comparison meaningless.

Two things were fixed before it was checked in, neither of which mattered while it lived in a scratch directory. It used a `<( ... )` process substitution, which is a bashism that works under Git Bash and fails under a real POSIX `sh`; it is now written to `sh` and verified under `dash`. And it now deletes the stray `output.txt` that `WriteToAFile_Test` drops into the working directory, which a runner living in the repository would otherwise cause to be committed by accident.

## What is not here

This list held only additions. Nothing on it was a defect, and the four stub files — `comsec`, `comsafe`, `comgenbuf`, `matrixlib` — are deliberately still stubs; CLAUDE.md's "Known gaps" section is where those live, not here.
