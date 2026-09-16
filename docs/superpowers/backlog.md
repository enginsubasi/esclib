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

## 12. The two filters the group was missing — DONE 15/09/2026

**`fir`.** `maf` was already an FIR — a rectangular window with every tap equal — and there was no way to choose the taps. That left the group able to smooth, to reject impulses, to bound a rate and to shape a response in hertz, but not to place a stopband or to keep linear phase.

The framing that settled its shape: `maf` is not a special case waiting to be replaced. It keeps a running sum and costs one add and one subtract a sample whatever its length, where this multiplies every tap every sample, so `maf` stays the right filter whenever a rectangular window will do. What the taps buy is a stopband that can be placed — a rectangular window's first sidelobe is only 13 dB down and cannot be moved — and, for a symmetric set, exactly linear phase, which is the reason to pay O(N) over `biquad`'s handful of operations: when the measurement is the *shape* of a waveform rather than its level, a filter that delays every frequency differently has already destroyed the answer.

No designer, for `biquad`'s reason one step further out: a windowed sinc or a Parks-McClellan fit belongs on a host, which is also why the taps are a pointer to `const` and expected to live in flash. `firInit` fills the history with `inputInit` as `mafInit` does, and reports the settled output as `inputInit` times the sum of the taps — `inputInit` for a unity-gain design, and correctly zero for a differentiator, which the test asserts rather than assumes. The `i32` variant is Q16 on the taps only, `int64_t` accumulator, and its single shift rounds.

**`goertzel`.** The other half of `biquad`'s notch. Nothing in the tree could say how much of one frequency was present — a filter reports what is left after it, not what went in.

Not an FFT, and the reason is the library's usual one: an FFT gives every bin at the cost of a scratch buffer, a twiddle table and N log N operations, while this gives one bin for two multiplies and two adds a sample with no buffer at all. Block based, which is inherent rather than a simplification. The block completes and reloads *inside* the iteration, so a missed result costs that result and never the phase — `softtimer`'s rule. Normalized so a sine of amplitude one reads one whatever the block length, with `goertzelGetPower` beside `goertzelGetMagnitude` because a threshold comparison needs no square root.

Two decisions are recorded rather than hidden. The coefficient is taken at the frequency **asked for** rather than snapped to the nearest exact bin, so a tone between bins reads low instead of being quietly rounded onto one; the file tells the caller to choose a block length making `frequency * blockLength / sampleRate` a whole number, and the test pins a tone half a bin off centre reading 0.65 to prove no snapping happens. And there is **no integer variant yet**: the two state words grow with the block length, so the usable range depends on a parameter chosen at `Init` rather than on a fixed bound, and sizing that honestly — plus an integer square root for the magnitude — is a design rather than a transliteration. `goertzelGetPower` needs no root, so a power-only fixed point variant is the natural shape if one is wanted.

`goertzelIsReady` is the second accessor in the library that writes, after `bininpGetRisingValue`, for the same reason: an event flag that survives being read stays TRUE forever after the first event. `codingReference.md`'s direction table was updated rather than left to contradict the tree.

`FirGoertzel_Test` is new, 89 checks, every expected value from an independent model run before the C was. Four mutations were confirmed: walking the history forward, truncating the fixed point shift, dropping the cross term from the squared magnitude, and not reloading the state at the block end.

## 13. The tests were never tested — DONE 15/09/2026

Six modules into this round of work, the process had a hole worth more than another module.

The testing rule here is that a fixed bug earns a check aimed at it, so the regression fails rather than passing quietly, and `CLAUDE.md` keeps a table of which test pins which bug. **Nothing checked that those pins still bite.** Soften an assertion while tidying a test and the pin dies silently — which is exactly the failure the pin was written to prevent. Eight mutations were run by hand across this session's work and all eight were thrown away afterwards.

`scripts/mutate.sh` keeps them. One file per defect under `scripts/mutations/`: the literal block to replace, its replacement, and the test that must fail. The runner applies each, runs only that test through `run_tests.sh` (which grew a single-test argument for this), reverts, and reports anything that survived. A mutation whose block no longer matches reports STALE rather than passing quietly, and one that does not compile reports BROKEN, because a mutant that cannot build proves nothing.

Three implementation notes worth keeping. The replacement is a **literal substring match**, not a regex and not a patch: a regex would need every metacharacter in a block of C escaped, and a patch goes stale the moment anything above it moves. Carriage returns are stripped on both sides, because on Windows the `.c` files check out CRLF and the mutation blocks LF, and a block differing only in line endings would look stale; the original is restored from a byte-for-byte copy, so nothing is reformatted. And `awk` forbids a space between a user-defined function name and its paren — the one place in this tree that cannot follow the house style.

Ten mutations are recorded, all ten caught: `fir`'s history direction and shift, `goertzel`'s cross term and block reload, `biquad`'s shift, `dcMotor`'s reversal interlock and its nan clamp, `mathLerpi32`'s rounding, `pack`'s 24-bit sign extension, and `crc8Dallas` being handed the SMBus polynomial. The runner was itself checked with a deliberate no-op mutation, which it correctly reported as SURVIVED — otherwise "ten caught" would have been unearned.

**Two smaller things came with it.** `scripts/check.sh` gained a fourth check: every module object's `.data` and `.bss` must be empty. "The caller owns all storage and the module holds no static state of its own" was stated in `rules.md` and checked nowhere; it turns out to be true across all 38 modules and is now enforced. It was verified by smuggling a writable static into `slew.c`, which it caught.

And `scripts/size.sh` reports code size per module. The documentation here argues about cost constantly — that a 256-byte table is the wrong trade for an 8-bit CRC, that a cosine at boot links the whole software float library, that `maf` is O(1) where `fir` is O(N) — and none of it had ever been measured. It has now: the whole library is **17250 bytes of code on a Cortex-M0 at `-Os` and zero bytes of RAM**, `crc8` is 104 bytes against tabled `crc16`'s 616, and `biquad` is the largest module at 1628.

CI runs the mutations as a gate and prints the size table into the log. The CI itself was confirmed green for the first time in this pass — four runs, including the cross link, so the apt `gcc-arm-none-eabi` does carry `--specs=nosys.specs` after all.

## 14. The rest of the pin table, and the three holes it found — DONE 15/09/2026

The mutation harness landed with ten mutations, all from that day's own work. The rows of `CLAUDE.md`'s bug-pinning table that predate it — the oldest fixes, the least recently touched code, the pins most likely to have drifted — had none. Eighteen more were written from the table, generated by extracting the exact function body from the source so that a width variant could not be confused for its sibling.

**Three of the twenty-eight survived the first run.** All three were holes in the tests, not bad mutations.

Two were the same root cause. `sortSelection` computing `length - 1` on an empty array, and `searchBinary` letting its unsigned right bound wrap below zero, are both checked in `SortSearch_Test` — on the `u32` and `i32` widths only. Every width carries its own copy of the guard, so deleting the one in the `float` version passed the entire suite. That is the mirror image of the hole `CLAUDE.md` already records for `MAF_Test` and `EMAF_Test`, where the tests only ever touched the float halves and the defects were in the integer ones. The lesson generalizes and is now a rule: **a check on one width proves nothing about the others.** The float cases were added and both mutations are caught.

The third was subtler and is the more useful finding. `Control_Test` already had a check that depends on `pidInit` clearing `lastError` — "the first step is the error divided by the sampling time" — and it passed with the whole initial-state block deleted from `pidInit`. The driver is a stack local, and the memory happened to be zero, so the missing assignment read exactly like a successful one. The test was measuring the stack, not the code. `poison` now fills the driver with `0x5A` bytes before each `Init` in a dedicated case, which makes the assignment observable; `0x5A5A5A5A` is a valid float of about 1.5e16 rather than the more obvious `0xFF`, because all-ones is a nan and a nan compares false against everything, which could pass a check by accident — the very problem being guarded against. Also now a rule: **a test that claims `Init` writes a field has to poison the struct first.**

Twenty-eight mutations, twenty-eight caught, and two rules in `rules.md` that were learned rather than assumed.

## 15. `q16`, for the caller the Q16 variants left empty handed — DONE 15/09/2026

Five modules carry a Q16 fixed point variant and by the module independence rule none of them may include a shared helper. That rule is right and stays. What it does not cover is the **caller**, who by now has been handed four APIs taking gains, rate limits and coefficients in Q16 and had nothing whatever to do arithmetic on them with.

Five functions, and the set is small because each one earns its place by being a thing the hand-written version gets wrong:

- `q16Mul` needs an `int64_t` intermediate. Two Q16 values make a Q32 product, so a gain of 2.0 against 40000 is already past thirty-two bits and the naive version comes back with the wrong sign.
- `q16Div` needs the scale applied **before** the division. Doing it after answers zero for every quotient below one, which is most of them in a control loop.
- `q16ToInt` rounds rather than shifts. A shift truncates toward minus infinity, which on a value walked across a range in steps is a steady drift rather than noise.
- `q16Sqrt` exists because C has no integer square root, and this is the one operation with no workaround at all without a float.
- `q16FromInt` is the trivial one, and it is here because it has to saturate like everything else.

Three decisions worth recording. **Everything saturates rather than wrapping** — a wrapped Q16 value changes sign, and these numbers are headed for a control loop where that means full reverse torque from a small overshoot; `alphabetaGetVelocityi32` already saturates at its range ends for the same reason. **No float appears in the file**, so a program that links it cannot pull the software float library in by accident; converting a designed constant stays a compile-time expression at the call site, exactly as `biquad.c` and `fir.c` already instruct. And **there is no `q16Clamp` or `q16Abs`**: a Q16 value is an `int32_t`, so `mathClampi32` and `mathAbsolutei32` already do the right thing to one, and a renamed copy of either would be the symmetry-for-its-own-sake rule 7 forbids. No `q16Lerp` either — `mathLerpi32` already takes its fraction in Q16.

The integer square root is `rampSquareRoot` duplicated, which is what rule 2 requires and what `interp.c` does with `searchUpperBound`'s bracketing loop. `Q16_Test` ties the two together directly: it rebuilds `rampIterationi32`'s braking envelope, `sqrt ( 2 * a * remaining )`, out of `q16FromInt`, `q16Mul` and `q16Sqrt`, and asserts it lands on 14.142136 for an acceleration of 2 and a distance of 50 — the identity that the square root of a Q32 product is a Q16 value, which is the whole reason that envelope survived the move to fixed point unchanged.

`Q16_Test` asserts 57 checks, every expected value computed from the definition of the scale rather than from a run, including a round trip over all 65536 representable integers. Five mutations were added and all five are caught. The module is 460 bytes on a Cortex-M0.

## 16. `sched` and `fsm`, and three more holes — DONE 15/09/2026

Two modules that every project using this library writes by hand, and the last of the second-tier list.

**`sched`.** `softtimer` is one timer; this is the table of them. Its one real decision is the split between `schedTick`, which only counts and marks from the interrupt, and `schedRun`, which calls the due tasks from the main loop — a scheduler that dispatched inside the tick would run a display update in an interrupt, and `comat` already draws that line between `Receive` and `Evaluate`. A task still due when its period comes round again is an **overrun**, counted rather than swallowed and never caught up on: running it twice to make up the time is a guess. The reload subtracts the period, so a busy loop costs the run and never the phase. It does not include `softtimer.h`.

**`fsm`.** The general form of the machines `comat` and `comstxetx` write by hand, with the transitions as a `const` table in flash. The state changes **before** the action runs, so a self-dispatching action transitions out of the new state instead of looping back into the transition it is inside; the first matching row wins and the scan stops; an event no row accepts is counted rather than ignored. A `NULL` action is a legitimate row — a transition that only changes state — and is the one pointer the module checks per call.

**Three of the ten new mutations survived the first run, and all three were the same shape.** A second check inside the module was masking the defect the test aimed at.

- `schedTick` counting a disabled task survived, because `schedRun` tests the enabled flag too, so the task still never ran. The damage is real but shows elsewhere: overruns pile up for a task nobody is waiting on. The test now asserts that.
- Disabling a task while its due flag stands survived for exactly the same reason, and only shows on the way back — the stale flag fires the moment the task is switched on again. The test now completes that round trip.
- Removing `fsmDispatch`'s `break` survived the obvious duplicate-row table, and the reason is worth keeping: the state is written inside the loop, so by the time the scan reaches the second row its `from` no longer matches. The table that catches it is a **chain** whose second row matches the state the first row moves to, where a scan that ran on would fire both transitions for one event.

The lesson generalizes and is the same one that came out of the previous pass in a different dress: **a test has to reach the place the defect actually shows, which is not always the place it was introduced.**

`Sched_Test` asserts 61 checks and `Fsm_Test` 53, and all ten mutations are caught. `sched` is 256 bytes on a Cortex-M0 and `fsm` is 106.

## 17. The four stubs — DONE 15/09/2026

Names reserved in 2022 and 2023 that had never been written. `comgenbuf` had no `.c` at all and a struct with an index, a size and a buffer; `matrixlib` had a banner and a `mtrx_t` holding a bare `float*` with no dimensions, which no operation could have used; `comsafe` and `comsec` had a banner each and a header declaring nothing.

**`comgenbuf`** is a queue of variable-length packets where `circBuf` queues bytes. Each packet is a two-byte length and its payload in one caller-owned ring. A push is all or nothing — half a frame handed to a parser is worse than no frame — and a full queue refuses the *newest* packet rather than dropping the oldest, which is the opposite of `circBuf`'s overwrite mode and is the point: dropping a packet already accepted loses a message the caller was told it would get.

**`matrixlib`** is the linear algebra `basicmatrix` does not do. `mtrx_t` carries its shape; every operation returns a status because whether two shapes agree cannot be known when either was initialized. Aliasing is allowed for the element-wise operations and refused for multiply and transpose, where a result sharing storage with an operand would compute against values it had already overwritten. `matrixInverse` takes a scratch matrix rather than allocating one, pivots partially, and invents no epsilon for "nearly singular" — how close is too close depends on what the numbers mean.

**`comsafe`** is the black-channel pattern: connection id, sequence, an independent check installed at `Init`, and a watchdog for the case where nothing arrives at all. Any rejection drops the channel and it stays down until `comsafeReset`, because only the caller knows whether the process may resume. **The test found a real defect in the module rather than the other way round**: the banner said a failure stands until a reset, and the code put the channel back into service on the next good frame. The banner was right.

**`comsec`** is the one that had been unwritable, and the hook `comstxetx` already had is what unblocked it. It contains no cryptography: a session id, a 32-bit counter and a tag from an injected MAC. Its replay rule is *strictly greater* rather than *next*, which is exactly where it differs from `comsafe` — a link may lose frames legitimately, and the one thing that must never happen is a frame being accepted twice. The counter does not wrap; once spent, `comsecBuildFrame` refuses, because two frames under one key and counter break everything at once. A rejection does not stop the channel, unlike `comsafe`, because a link an attacker can reach would otherwise be denied service by anyone able to send a packet.

Two things were written down rather than papered over. `comsec` **provides no confidentiality** and says so twice; a caller who needs secrecy encrypts before handing the payload over. And its tag comparison is constant time, which **no mutation can pin**: the early-exiting version rejects exactly the same frames and differs only in timing, so the pin table records the gap instead of pretending to cover it.

Four tests, 77 + 80 + 88 + 91 checks, and twenty-five mutations all caught. With these in, **the Known gaps section of CLAUDE.md is empty for the first time** and the rule in `rules.md` — a header with no source is a defect — is unconditional.

## 18. What the fixed point variants actually cost — DONE 15/09/2026

Three measurements, and two of them corrected something the documentation was overstating.

**The strict warning profile is cheap.** The tree carried nine warnings under `-Wconversion -Wsign-conversion -Wshadow -Wdouble-promotion -Wcast-qual`, across forty-two modules. Eight were one pattern — an array length converted to `float` for a divide, in `maf`, `basicmath` and `statistic` — correct in every case, because a `uint32_t` past 2^24 is not a reachable array length, and implicit in every case. The ninth was a narrowing after an xor in `crc16`. All nine are written out now and `STRICT=1 sh scripts/check.sh` is a gate that CI runs, which is only defensible because the cost of adopting it was nine casts rather than a project.

**The fixed point variants are not free, and nothing said so.** Every `i32` and Q16 variant in this tree exists to avoid the software float routines, and every one of them pulls in 64-bit integer helpers in their place. Measured on a Cortex-M0: `q16`, `interp` and `biquad` each link `__aeabi_ldivmod` and `__aeabi_lmul`; `pid`, `ramp`, `alphabeta` and `fir` link `__aeabi_lmul`. That part has no 64-bit multiply and no divide instruction at all, so a Q16 divide is a runtime call of the same order as the float divide it replaced. The multiplies, the roots and the conversions still win comfortably; the divides win mostly on code size. This is not a defect — an `int64_t` intermediate is what keeps those variants correct and the reasoning for it is recorded all over the tree — but "for parts with no FPU" is half a claim without the other half, and a library that argues about cost as often as this one does should not have left it unmeasured.

The same run found the matching omission in `checksum`, whose own notes compare its five functions without mentioning that `Fletcher16` and `Adler32` reduce modulo 255 and 65521 — a `__aeabi_uidivmod` per byte on a part with no divider, where `xor` and the two sums need nothing. It still does not change which to reach for.

**And nothing pulls in double precision.** Several banners claim the library is single precision throughout and nothing had ever checked it. `scripts/runtime.sh` reports the helpers per module and fails on a `__aeabi_d*`, which is the one rule in that report rather than an observation.

## 19. Composition, and the examples nobody was building — DONE 15/09/2026

Thirty-four tests, every one of them exercising a single module. Modules may not include each other, so they only ever meet in caller code — and the repository contained none. Whether their units, buffer sizes and callback shapes line up was the first thing a consumer would find out and the last thing anything here checked.

`test/Integration_Test/` is that caller code. Three real stacks: `comstxetx` → `comgenbuf` → `comsafe` on the receive path, `ramp` → `encoder` → `pid` → `dcMotor` for motion, `pack` → `median` → `biquad` → `interp` for a measurement. **It found three things while it was being written, and not one of them was a defect in any module.**

- A frame the transport drops is, to the safety layer above it, a frame that was *lost*. `comsafe` treats a gap in its sequence as a fault by design, so the next good frame is refused too and the caller has to decide the channel is trustworthy again. Both modules behave exactly as documented; the consequence only exists where they meet.
- The first draft never called `biquadReset`, so a notch sat climbing toward a reading that was already there. `biquad`'s own banner says to reset it and says why, and the draft did it wrong anyway — which is a fair measure of how easy that is to miss.
- **`dcMotorBridgeState` has to be called before `dcMotorSetSpeed`.** The reversal interlock zeroes the duty before it moves the pins, so the other order has it wipe the value just written, and the motor stops every time the loop changes its mind. That one is asserted on its own, because it is the kind of thing that costs somebody a day and cannot show up in `DcMotor_Test`.

A fourth thing was an expectation of mine rather than a finding: the loop would not overshoot at a low proportional gain, so the reversal interlock was never exercised in the stack at all. The gain was raised until it did, which is both more realistic and the only way that check means anything.

**And `sample/` did not use the library.** Five directories of C tutorials — void pointers, function pointers, a DFT, byte and bit representation, file logging — and nothing showing how to consume a module, for a library whose entire model is copying one `.h`/`.c` pair into a project. Three worked examples were added: `FilteredScale`, `MotionLoop` and `FramedLink`, each a stack rather than a single call, with the hardware faked so they build and run anywhere. The old tutorials stay; they teach something else.

`scripts/samples.sh` builds and runs all of them, deriving library dependencies from each sample's own `#include` lines exactly as `run_tests.sh` does. **It found two defects on its first run, and both were the same two classes the test suite's first run found in 2026:** `sample/DFT` had not compiled for years — it called `time()` without `<time.h>`, which a modern compiler makes an error rather than a warning — and `sample/FunctionPointerBasic` ended `main` with `return ( 1 );`, so it reported failure on every successful run. The other three carried three warnings between them. All of it is clean now and CI gates on it.

## 20. The last three width gaps — DONE 15/09/2026

Three that had been visible for a while and each had a clear precedent already in the tree, which is what separated them from symmetry for its own sake.

**`circBufi32`.** The ring came in `u8` and `u32`, and a ring of *signed* samples is ordinary — a converter reading that swings about zero, a control error, an encoder delta. A caller holding one had to cast on the way in and on the way out, and either cast is silent when it is wrong: minus one comes back as four thousand million. The five functions are the `u32` ones generated rather than retyped, because nothing in their bodies depends on the element type — they index an array and compare indices — so only the signatures differ. Generating them is what makes it impossible for the two widths to drift apart in the body, which is the thing that would actually go wrong.

**`crc32Alt`.** `crc16` has offered a table and a bit-by-bit loop for one polynomial since it was written, for the caller who would rather spend eight shifts a byte than the flash. `crc32` never did, although its table is the **largest constant in the library**. Measured: the table is 1024 bytes and the loop is 56. A caller copying only `crc32Alt` out of the tree saves a kilobyte.

**`complexi32`.** Q16 arithmetic, for the one caller that genuinely wants complex numbers without an FPU: phasor work, where an energy meter multiplies a voltage phasor by a current and an impedance measurement divides one by the other. Four operations and an `Init`, and **the polar pair is deliberately absent** — a fixed-point magnitude needs a square root and a fixed-point angle needs an `atan2`, which together are a CORDIC with a table, an iteration count and a range reduction. That is a module of its own rather than a width of this one, and a version built on a float `atan2` would defeat the entire reason the width exists. Saying so is better than shipping half of it.

Two details in `complexi32` are worth keeping. Its `complexMuli32` computes **both** parts into locals before writing either, which is what makes an aliased result safe there where `matrixMul` refuses one: the imaginary part needs the operands' real parts, so writing `result->re` first would destroy one of them. And its `complexDivi32` carried the same sign defect the float width once had — the test pins it in both, which is the point of pinning it at all.

Eleven new symbols, six new mutations, all caught. Everything stayed clean under the strict profile without a single cast being added, which is the first change since that gate went in and a fair test of whether it was set at a sensible level.

## 21. Three claims the tree made and nothing checked — DONE 15/09/2026

Everything above was a thing to build. This one was a thing to *measure*, and it started from a list of claims the library makes that no gate tested. Three were picked because each is answerable with a compiler flag rather than an argument.

Two held. Every header and every module compiles as **C++**, which is what the `extern "C"` block in each header has always promised and nothing had ever checked. And the tree is clean under **`-std=c99 -pedantic-errors`**, with no GNU extension anywhere — C99 rather than C89 because `<stdint.h>` and `int64_t` are C99, so the style is C89 and the floor is not, and saying otherwise would be a claim that cannot be held. Both are now `scripts/portable.sh`, and both are gates.

The third found a real defect. Running the suite under **UBSan** made `Q16_Test` and `Ramp_Test` fail, and the cause was six sites scaling a value into Q16 with `( ( int64_t ) value ) << Q`. **Left-shifting a negative signed value is undefined in C** — right-shifting one is only implementation-defined, which this tree already documents and accepts, and the two are not the same category. `ramp`, `alphabeta`, `biquad` and `q16` all did it.

What makes this worth writing down is that **nothing was wrong with the output**. Every one of the six produced the correct answer on every compiler in reach, which is why thirty-five test programs and seventy-four mutations had never seen it and why no assertion could have. The fix is `value * ONE`, which is defined for both signs and compiles to the same instruction, so it costs nothing at all.

Two of the six sat on paths no test ever drove negative, so `FilterSet_Test` gained negative cases for `alphabetai32` and `biquadi32` — an `alphabeta` initialized below zero and tracking a negative measurement, a `biquad` reset to a load cell reading below tare. Those cases are documented for what they actually do rather than for what would sound better: they do **not** catch the shift, and that was verified by putting the shift back and watching the plain build still pass while the sanitized one traps. What they do is make the line reachable, because a sanitizer only reports what the tests execute. For the same reason there is deliberately no mutation for this defect — a mutation restoring the shift would survive by construction, which is exactly what `ComSec_Test` already says about its constant-time comparison.

The general rule is now in `rules.md` where the width-variant rules live, because it belongs next to "an integer division rounds to nearest" rather than in a changelog: scaling into a fixed-point format is a multiply, never a left shift.

## 22. cordic, the module two other files had already asked for — DONE 15/09/2026

This one was not chosen from a list of things that would be nice. Two files in the tree had written down an absence and given the reason for it, and both reasons were the same reason. `complex.c` says its `i32` width has no polar pair because a fixed point magnitude needs a square root and a fixed point angle needs an `atan2`, which together are a CORDIC and a module of their own. `biquad.c` says its `i32` width has no designer because turning a corner in hertz into coefficients takes a cosine, and calling `cosf` at boot on a part with no FPU links the whole software float library — which is the cost the fixed point variant exists to avoid. One module answers both.

**The angle is a binary angle and that is the design decision.** The whole turn is 2^32 counts in a `uint32_t`. An angle then wraps by itself, so there is no range reduction to get wrong; there is no pi to represent, which Q16 could not do honestly anyway; the type is unsigned, so the wrap is defined rather than the undefined behaviour a signed overflow would be — which is the lesson from the entry above, applied on purpose this time; and the conversion a caller actually needs becomes trivial, because a frequency as a fraction of the sample rate *is* that fraction of a turn. A corner at 50 Hz in a 1 kHz loop is `( 50u * 4294967296u ) / 1000u`, and there is no pi in it.

**Both parameters were measured rather than picked.** Twenty iterations and a Q24 intermediate: at Q16 internally the sine is out by eleven counts of 65536 *whatever* the iteration count, because the shifted terms fall below the last bit before the algorithm has finished with them, and at Q24 it is out by one. Past twenty iterations nothing improves, because the table entries are then smaller than the angle's own last bit. The final numbers, against a double precision model over two hundred thousand cases: sine and cosine within 1 count of 65536, angle within 1307 counts of a turn, length within four parts in a hundred million.

**Two defects were found by measuring rather than by testing**, and both are the kind that pass a plausible test. The normalization doubled one step too far, putting the larger component past the range the iteration was sized for; small vectors were perfect and large ones overflowed, and the tell was `cordicMagnitude` at the very end of `int32_t` coming back negative. And the gain compensation was a Q16 constant, whose own rounding is 1.8 parts in a million — every length came back biased high by exactly that, which only showed because a magnitude of a round number is a round number. Widening that one constant to Q24 improved every length in the module by a factor of forty four and cost nothing.

**It needs no divide at all.** `sh scripts/runtime.sh` reports `__aeabi_lmul` and a shift helper, where `q16`, `interp` and `biquad` each link `__aeabi_ldivmod`. 844 bytes of code, nothing in `.data` or `.bss`, and no float anywhere in the file — which is the whole point, since a part with an FPU would have called `sinf`.

Seven functions, eight mutations, all caught.

## 23. commodbus, the protocol an industrial project actually meets — DONE 15/09/2026

`comstxetx` frames binary the way this library would design it. `commodbus` frames it the way the field already decided, and the two differ in the one place that matters: **RTU has no start byte and no end byte**. A frame ends when the line has been idle for three and a half character times. So the periodic tick is not a timeout bolted onto the framing — it *is* the framing, and everything else follows from that. The silence is expressed in ticks, `softtimer`'s rule, and the module is never told the baud rate: turning character times into ticks is one expression that belongs where the baud rate is configured, and taking a baud rate in would hand this module a second thing it cannot check.

**The scope was the real decision, and it is framing only.** No function codes, no register map, no exception responses. A register map is the application's data and this library allocates nothing; a table of handlers is what `fsm` already is. What a project cannot write for itself in an afternoon is the part below that — where a frame begins and ends, whether it is addressed here, and whether it survived the line.

Three details are the module:

**The check travels low byte first.** Modbus is big endian in every field of every payload and little endian in its own check, which is the sort of thing that is obvious once and never again. A frame built the obvious way is the right length, right in every other byte, and refused by every peer on the bus — so the test pins the two halves separately, because the encoder and the decoder can each be wrong in the same direction and agree with each other perfectly.

**A frame for another device is ignored, not rejected**, and the two have separate counters. On a multidrop bus with ten devices, nine tenths of what each one hears is addressed to somebody else; counting that as a fault would leave the reject count meaning nothing, when the one thing it is for is telling somebody the line is bad.

**t1.5 is deliberately absent.** The intra-character gap would cost a second threshold and a second piece of state, to reject a frame with a hole in the middle of it — and a hole in the middle means bytes from two different frames concatenated, which the check refuses with overwhelming probability. The reason is written in the file rather than left as a gap for somebody to notice.

Nine mutations, and one of them survived its first run and was worth the trouble. The truncation mutation — keeping what fits instead of refusing the whole frame — passed, because the test fed twelve arbitrary bytes into an eight byte buffer and the truncated remains failed the check anyway. The fixture that tells them apart is a **valid eight byte frame with four more bytes behind it**: truncation there hands the parser a frame that passes its check and is half of what was sent. That is the failure the rule exists for, and until the fixture matched it the mutation was checking nothing.

368 bytes of code, nothing in `.data` or `.bss`.

## 24. cobs, the other answer to the question comstxetx answers — DONE 16/09/2026

`comstxetx` has framed binary in this tree for years by escaping: a payload byte equal to STX, ETX or the caller's DLE travels behind a DLE. That works, and its cost is unbounded in the only way that matters to an embedded caller — a payload made of those three bytes **doubles** on the wire, so the transmit buffer has to be twice the payload or some frame nobody predicted gets refused.

COBS removes a byte value from the payload instead of escaping it. The overhead is one byte per 254 regardless of content: under half a percent, and known before the payload is. That is the whole argument, and it is arithmetic rather than taste — both modules stay, because escaping is right when the delimiter has to be a printable character a human can see in a terminal, and this is right when the buffer has to be sized in advance.

Four decisions are worth keeping:

**The name does not start with `com`.** Everything else in that directory does, and this is one named algorithm rather than a protocol of this library's design. The name is what a reader searches for; `crc16` and `checksum` sit beside their users under their own names for the same reason.

**It carries no check, and the file says so.** COBS structure is thin — a flipped bit frequently produces a well formed frame of the wrong length rather than something a decoder can refuse. Integrity goes inside the payload, where `comsafe` and `comsec` put theirs, or beside it as a check the caller appends before encoding. Writing that down is the difference between a framing module and a framing module somebody trusts for the wrong thing.

**`cobsEncode` does not write the delimiter.** One zero between frames or two around each is the caller's protocol, and a module that picked one would be choosing for them.

**`cobsDecode` validates the whole frame before writing a byte.** It costs a second pass, and that pass reads only the code bytes and steps over the runs between them — a two hundred and fifty fifth of the buffer. What it buys is the all-or-nothing guarantee `comgenbufPop` makes and `matrixInverse` explicitly cannot, and both refusal checks in the test assert the destination was left untouched rather than merely that FALSE came back.

276 bytes of code. Nine mutations, all caught on the first run — the test was written around where the algorithm changes behaviour (a payload of exactly 254 bytes, either side of it, zeros at both ends, nothing but zeros) rather than around variety, which is why.

## 25. text, the stdio this library does not have — DONE 16/09/2026

Nothing in `src/` may call stdio, so a project on this library has never had `printf` or `strtol` to lean on and wrote bytes-to-text by hand. `pack` already answered the binary half of that; this is the ASCII half — `comat`'s link, a terminal, a log line. Hexadecimal, base64 and decimal, and each earns its place with one specific thing the hand-written version gets wrong rather than with convenience.

**The decimal half is where the mistakes live**, and the scope grew to include it for that reason. The parsers test `accumulated > ( limit - digit ) / 10` before each multiply, because after the multiply the value has wrapped and the evidence is gone — `atoi`'s trouble exactly. The signed writers never negate, since the most negative `int32_t` has no positive counterpart. `textFromQ16` carries — `0.999` at two places is `1.00`, not `0.100` — and prints no negative zero. `textToQ16` counts the digits after the point instead of valuing them, so `0.05` is five hundredths. Five places are the fewest that survive a round trip for every Q16 value; the test sweeps a hundred thousand values to show it, and shows four failing on a single LSB.

**Base64 is strict**, and that is the decision in that half: length a multiple of four, padding only at the end, and the bits padding leaves unused required to be zero. The last rule is the one lenient decoders skip, and skipping it gives one payload several encodings — `Zg==` and `Zh==` are both "f" — which fools anything that compares or signs the text rather than the bytes.

**The text is ASCII by definition**, so the file writes byte values rather than character literals. A character literal is in the compiler's execution character set, and C promises only that the digits are contiguous there. It is a small point and the right one for a library whose output is defined by the protocols it serves rather than by the compiler that built it.

Fourteen mutations, all caught on the first run. One defect is deliberately not pinned — negating `INT32_MIN` gives the right answer on every compiler in reach, so the cases that reach it exist for the sanitized run, which is where the Q16 shifts were found.

**A measurement finding came out of this and is recorded rather than fixed.** `scripts/runtime.sh` listed `__aeabi_idiv` and `__aeabi_ldivmod` for `text`, which divides nothing signed. Neither is called: the object carries them as undefined symbols with no relocation against them. They are not harmless for that — a plain link pulls both into the image, and only `--gc-sections` drops them. The same shows on two older modules, `checksum` (`__aeabi_idivmod`) and `maf` (`__aeabi_uldivmod`). So `runtime.sh` answers "what gets linked without garbage collection" correctly and "what gets called" not at all, and CLAUDE.md's helper claims were written as if it answered the second. Telling the two apart is a script change of a few lines, and it belongs to the measurement tranche rather than to this module.

## 26. rms, statistic in the shape an interrupt can call — DONE 16/09/2026

`statistic` already had a mean and a standard deviation, so this had to justify itself against it, and the justification is the shape: `statVariance` wants the whole block in memory, and a sampling interrupt has one sample. `rms` keeps a handful of words whatever the block length and completes a block inside the iteration, which is `goertzel`'s arrangement. It is to `statistic` what `maf` is to `basicmath`'s mean. The proposal was a broader streaming-statistics accumulator; the three results that came out — mean, RMS, AC RMS — are that accumulator, named for the question it is asked most.

**The float design was settled by measurement before a line was written.** The textbook form — mean square less the square of the mean — was modelled in single precision against an exact reference: 181 percent wrong on amplitude 10 over dc 2048 across 65536 samples, and wrong by a factor of 770 on one count over 20000. Subtracting each block's first sample before accumulating brings every one of those within eight parts in a million, at no cost per sample. Welford's method was the alternative and lost on its divide per sample.

**The integer variant takes `int16_t`**, the first width of that name here, and the type is the precondition rather than a restriction: inside it the square fits 32 bits, every sum is exact in 64 bits over the 65536 samples `rmsIniti16` allows, and the variance comes out exactly as N·Σx² − (Σx)² with nothing to cancel. The per-sample path is one `muls` and two 64-bit adds, and the disassembly was read to confirm the first runtime helper call sits after the block-end branch. Results are Q16 because noise is routinely below a count, and the roots are normalized before they are taken, which is `cordic`'s move.

Ten mutations, all caught. Two properties are recorded as unpinned rather than pretended: the `int32_t` cast on the square, which only a part with a 16-bit `int` can show and so belongs to the 16-bit `int` gate proposed for the measurement tranche, and the float variance clamp, which rounding has to be provoked into.

712 bytes of code, nothing in `.data` or `.bss`.

## 27. Two corrections to the measuring instruments — DONE 16/09/2026

Neither of these added a feature. Both made an existing claim true.

**The address sanitizer joined the undefined behaviour one in CI.** The sanitized run has asked since 15/09/2026 whether the answers were arrived at legally, and it was asking half the question. Undefined arithmetic is what UBSan sees; a read past the end of a caller's buffer is not undefined arithmetic at all, it is a read of whatever was next in memory, and this library has seven functions that take bytes from a link nobody controls — `comstxetxReceive`, `comatReceive`, `commodbusReceive`, `cobsDecode`, `comsafeCheckFrame`, `comsecCheckFrame` and `comgenbufPop`. Adding `address` to the flag list costs one word and covers all of them at once. It cannot be checked on this machine: a MinGW host has neither `libasan` nor `libubsan`, which is why the local run uses trap mode, and trap mode has no address half. So this one is verified in CI and nowhere else, and saying so is part of the change.

**`scripts/runtime.sh` was reporting two different things as one.** Writing `text` turned up `__aeabi_idiv` and `__aeabi_ldivmod` in its report for a module that divides nothing signed. Neither is called: the object names them in its symbol table with no relocation reaching them, because gcc declares a libcall while it is weighing an expansion and keeps the declaration after throwing the code away. The script read `nm -u`, so it could not tell that from a real call. It now reads the relocations for what a module **calls** and keeps `nm -u` for what it only **names**, and prints the second on its own line. Three modules carry one: `checksum` names a signed 32-bit divide, `maf` an unsigned 64-bit one, `text` both signed divides.

The distinction matters in both directions. The uncalled ones are not harmless — an undefined symbol pulls its archive member into a plain link and only `--gc-sections` drops it, so they cost flash on a build that does not use it. And the called ones are what every helper claim in CLAUDE.md was meant to be about; checked against the new column, all of them hold as written, which is the good outcome: the instrument was wrong and the readings taken from it were not.

The double precision rule still counts both kinds. A `double` that reached even a discarded expansion came from somewhere.

## Everything on this list is built

`softtimer`, the `comstxetx` transparency and integrity work, `checksum`, `interp`, the `basicmath` scalars, `ramp` and `encoder`, and after them the widths and the Q16 variants, `biquad`'s and `mathLerp`'s included, and then `dcMotor`'s speed control, `crc8`, `pack`, `fir` and `goertzel`, the mutation harness that keeps the tests honest, `q16`, `sched`, `fsm`, and finally the four stubs, a measurement pass over what the fixed point variants cost, the composition tests and worked examples that had never existed, the last three width gaps, a sanitizer run that found six pieces of undefined behaviour hiding behind correct answers, cordic, which two other files had been asking for in writing, commodbus, which is the framing an industrial bus actually uses, cobs, which is the other answer to the question comstxetx answers, text, the stdio this library does not have, and rms, statistic in the shape an interrupt can call. Nothing is outstanding, and for the first time nothing is reserved either.

The last open question — whether a test runner belongs in the tree — was **settled on 06/08/2026: it does.** The throwaway script that had run the suite for six modules became `run_tests.sh` at the repository root, and the "no runner" line in CLAUDE.md was rewritten rather than left to quietly contradict the tree.

It went in because it costs nothing to keep true. Each test's module dependencies come from its own `#include "..."` lines, so there is no list to fall out of sync and a new module needs no edit to it. It had also already earned its keep by catching two things nobody was looking for: an `output.txt` comparison that looked like staleness and was really a line-ending artefact, and `WriteToAFile_Test`'s `output.txt` being the file the program *writes* rather than its stdout, which made the comparison meaningless.

Two things were fixed before it was checked in, neither of which mattered while it lived in a scratch directory. It used a `<( ... )` process substitution, which is a bashism that works under Git Bash and fails under a real POSIX `sh`; it is now written to `sh` and verified under `dash`. And it now deletes the stray `output.txt` that `WriteToAFile_Test` drops into the working directory, which a runner living in the repository would otherwise cause to be committed by accident.

## What is not here

This list held only additions. Nothing on it was a defect, and the four stub files — `comsec`, `comsafe`, `comgenbuf`, `matrixlib` — are deliberately still stubs; CLAUDE.md's "Known gaps" section is where those live, not here.
