# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this repo is

`esclib` is a freestanding general-purpose C library for embedded targets: filters, PID/hysteresis control, circular buffer, CRC, sort/search, matrix/complex math, serial protocol handlers, soft timers, and shift-register drivers. No heap allocation, no OS dependency, C89-compatible style, `<stdint.h>` types throughout.

The `filter/` group is the largest and each module there answers a different problem, so pick by what is wrong with the signal rather than by habit: `maf`/`emaf` smooth, `median` rejects impulses outright, `biquad` shapes a response in hertz (and is the only way to notch mains hum), `slew` bounds the rate of change, `deadband` holds the output still until the input really moves, `alphabeta` estimates position *and* velocity, `fir` shapes a response the way `biquad` does but with linear phase, and `goertzel` does not filter at all — it measures how much of one frequency was there. `emaf` also carries an integer-only variant, `emafIniti32`/`emafIterationi32`, for parts with no FPU — it is a width variant of `emaf` rather than a module of its own, so the float code shares the file with it.

`sort` and `search` pair up and the choice within each is the same kind of decision. Among the sorts, `sortInsertion` wins on short or nearly-sorted arrays and is stable, `sortHeap` is the only one with an O(N log N) *guarantee* and stays in place without recursing, and `sortSelection`/`sortBubble` are there for teaching rather than for speed. Every sort produces ascending order; `sortReverse` turns that into descending in one pass, which is why there is no descending variant of each. Among the searches, `searchBinary` answers whether a value is present, `searchLowerBound`/`searchUpperBound` answer where it belongs — the insertion point, and their difference is the number of duplicates — and `searchClosest` answers which entry to read, which is the first half of what a calibration or linearisation table needs. All of the binary ones require ascending order and give a confident wrong answer without it, so `sortIsSorted` exists to check that precondition cheaply.

`interp` is the other half of that. `searchClosest` says which table entry to read, so the caller gets a value that is actually in the table; `interp` gives the value *between* two entries, which is what a calibration or linearisation curve is usually asked for. It is a driver rather than a stateless function for one reason: ascending order is a precondition here exactly as it is for the binary searches, and a driver has somewhere to check it. `interpInit` verifies strict ascent once, at boot, so `interpCalculate` can divide without testing the divisor — a repeated x would zero it. Past either end the value is held rather than extrapolated, and `interpInRange` answers separately whether that happened, for the caller who has to tell a saturated reading from a broken sensor. Reverse lookup needs no function of its own: a second `interp_t` initialized with the x and y tables exchanged reads the same curve backwards. The `i32` variant exists for the same reason `emaf`'s does — a part with no FPU — and carries every intermediate in `int64_t`, which is the only place in this tree that width appears.

`basicmath` is otherwise an array module — min, max, sum, mean, median, range — but it also carries three scalars that take values rather than arrays: `mathClamp`, `mathMap` and `mathLerp`. They are the lines an embedded project rewrites most often by hand. `mathMap` and `mathLerp` deliberately do **not** clamp: a value outside the input range extrapolates, which is the useful behaviour and is exactly why `mathClamp` is a separate function rather than folded into them. Only `mathClamp` has a `u32` variant — a `u32` map would need a descending range to earn its keep and cannot express one. `mathMapi32` carries every intermediate in `int64_t` for the reason `interp`'s integer path does: a twelve-bit reading scaled to millivolts already reaches seven digits. It differs from `interp` in one place, and that place is tested — its denominator may be negative, because unlike a table the input range is free to descend, so its round-to-nearest takes the magnitude of half the divisor first. A zero-width input range returns `outLow` rather than dividing.

`matrixlib` is the other half of `basicmatrix`. `basicmatrix` works on a bare array the caller describes at every call and does thresholding and limiting — element-by-element work where the shape barely matters. `matrixlib`'s `mtrx_t` carries its rows and columns, and does the operations where the shape *is* the point: multiply, transpose and invert, which are what a coordinate transform or a Kalman update is made of and the three an embedded project gets wrong inline against a flat array. Row major, caller-owned storage, nothing allocated — the struct describes somebody else's array rather than containing one. **Every operation returns a status**, which is unusual here and is not a break from the Init contract: these take *other matrices* as new arguments and whether their shapes agree cannot be known when any one of them was initialized, which is `pidChangeCoefficients`'s reason exactly. **Aliasing is allowed where it is safe and refused where it is not** — `matrixAdd`, `matrixSub` and `matrixScale` walk their operands in step so a result that is also an input is fine, while `matrixMul` and `matrixTranspose` compare the data pointers and refuse, because each element of a product is a sum over a whole row and column and there is no order that makes overwriting an operand safe. `matrixInverse` takes a **scratch matrix** rather than allocating one, runs Gauss-Jordan with partial pivoting, and refuses a pivot that is exactly zero once the best row has been swapped in; it invents no epsilon for "nearly singular", because how close is too close depends on what the numbers mean. It is also the one function in this library that may leave its output written on a `FALSE`, which is unavoidable — singularity is not visible until the elimination reaches the row that shows it, and the banner says so.

`ramp` is what `slew` cannot be. `slew` bounds one derivative and has no target it is trying to reach — it chases whatever sample it was last handed. `ramp` bounds two, and comes to rest exactly on a target. Knowing when to brake is the entire problem, and it is solved by a velocity envelope rather than a brake point: `sqrtf ( 2 * a * remaining )` is the fastest the ramp could be going and still stop on the target, so deceleration starts by itself the moment the velocity meets it. The target is a parameter of `rampIteration` rather than something installed at `Init`, which is `slewIteration`'s shape and is what makes a moving target fall out for free. The profile is trapezoidal, not triangular: without a velocity cap the peak speed of a long move grows without bound, and the cap cannot be bolted on from outside because chaining `slew` after `ramp` would alter the velocity that `ramp` computes its braking from. It takes `ts` at `Init` the way `pid` does, so the limits are written in units per second rather than per call. Nothing in it divides, so the zero limits `rampInit` rejects are rejected for `slew`'s reason — a ramp that never moves — rather than `pid`'s.

`encoder` closes the loop `dcMotor` opens: the driver could turn a shaft and nothing could read where it went. It decodes two quadrature channels into a signed position at **four counts per cycle**, because the state machine sees all four transitions anyway and discarding three would be a deliberate loss — a caller who wants cycles divides by four. Its one real decision is what to do when both channels changed between two samples: a step was missed, its direction is unrecoverable, and `encoder` refuses to guess. The position stands still and `encoderGetErrorCount` records the event, the same way `comstxetxGetRejectCount` counts a bad frame instead of swallowing it. A table answering ±2 there would look correct on a clean signal and drift silently on a noisy one, which is the worst failure this module could have; a non-zero count is the caller's signal to sample faster or filter the lines. `encoderInit` takes the levels the pins are sitting at, and that is not optional — starting from an assumed zero would make the first `encoderUpdate` read as a transition that never happened. It is the natural consumer of two `bininp` outputs and includes neither header, for the usual reason.

`softtimer` is the library's only time abstraction. It counts calls to `softtimerTick`, which the caller makes from a fixed-rate ISR, so the period is expressed in ticks and the interrupt rate is the unit — the same rule `hc595Interrupt` follows. One-shot and periodic modes share one struct; the periodic reload happens inside the tick rather than at the read, which is what lets an expiry the main loop fails to read cost the event but never the phase. The reload itself subtracts the period rather than clearing the counter, a form that stays correct if the counter ever overshoots today's invariant that it lands exactly on the period — the two forms are equivalent as things stand, and no test of this API distinguishes them. It is deliberately not consumed by `comat` or the shift-register drivers: they keep their own counters, because module independence forbids one module including another's header.

The two protocol modules answer different problems and only one of them checks what it receives. `comstxetx` frames binary: a payload byte equal to STX, ETX or the caller-chosen DLE travels preceded by DLE, and the byte after DLE is always data, so any byte value can cross the link. Every frame carries a two-byte check computed over the unescaped payload by a function the caller installs at `Init` — the signature is `crc16`'s, so `crc16` goes in directly, and the indirection is what lets the module keep its independence from `crc/`. `comstxetxBuildFrame` is the matching encoder, filling the transmit buffer the module owns, so the two halves cannot drift apart. A frame that fails its check is dropped and counted in `comstxetxGetRejectCount` rather than silently. `comat` has none of this on purpose: it speaks AT, an ASCII command protocol whose real peers do not checksum, and adding one would invent a dialect nothing else speaks.

`comsec` is the one module whose name was reserved for four years because it could not be written, and the thing that unblocked it is the hook `comstxetx` already had. **It contains no cryptography.** It is the framing around one: a session id, a monotonic 32-bit counter and a tag, with the tag computed by a function the caller installs at `Init` — which keeps the module independent of any algorithm and lets a part with a hardware accelerator use it instead of a software one. Two warnings in the banner are load-bearing and neither is boilerplate. **It provides authenticity, integrity and replay protection, and not confidentiality**: the payload travels in the clear, and a caller who needs secrecy encrypts it before handing it over, so that what gets authenticated is the ciphertext. And **the whole security rests on the injected function** — a tag from a CRC makes it worthless against anyone who can compute one, which is everyone, and nothing at this level could check that. Its counter rule is where it parts company with `comsafe`: `comsafe` expects the very next sequence number because a safety channel treats a lost frame as a fault, while `comsec` requires only that the counter be **strictly greater**, because a link may lose frames legitimately and the one thing that must never happen is a frame being accepted twice. The counter does **not** wrap — `comsecBuildFrame` refuses to send once it is spent, because two frames under one key and one counter break authenticity and replay protection together, and `comsecRekey` is how a caller who has established new key material carries on. A rejection does *not* stop the channel, which is the opposite of `comsafe` and is deliberate: a link an attacker can reach will be offered forged frames as a matter of course, and one that failed closed on the first would be denied service by anyone able to send a packet. The tag comparison is **constant time** — the obvious loop, which returns at the first differing byte, tells an attacker how many leading bytes of a guess were right and turns forging a tag into a byte-at-a-time search.

`comsafe` is the layer above all of them, and it is the **black channel** pattern: the transport underneath is treated as opaque and untrusted, and everything that has to be relied on travels end to end inside the payload it carries. That is why it is not another byte-level state machine — `comstxetx` already delivers whole frames, and repeating its framing would add a second thing to get wrong without adding a second *independent* check. The four failures it exists for are the ones a transport's own CRC cannot see: a frame lost, repeated, reordered, or arriving from the wrong sender. A one-byte sequence catches the first three, a connection id the fourth, and a watchdog catches the case no frame can report because there is no frame. The check is installed at `Init` like `comstxetx`'s, for that module's reason **and** one more: the safety layer should use a different algorithm from the transport, because two layers sharing one CRC share its blind spots. Two rules make it a safety layer rather than a retry: any rejection drops the channel to `CS_FAILED` and **it stays there** — while failed it refuses every frame without checking or counting it, because those are not new failures — and recovery is `comsafeReset` and nothing else, since only the caller knows whether the process it controls may resume. Both `@warning`s in the banner are load-bearing: **this is not cryptography** (an attacker can recompute the check, forge the id and advance the sequence, none of which involves a secret) and **it is not a certification** (the residual error rate, the reaction time budget and the diagnostic coverage are arguments about a whole channel that no library can make for its caller).

`comgenbuf` is the layer above both of them, and the reason it is not `circBuf` is one sentence: `circBuf` is a queue of *bytes* and loses where one packet ends and the next begins. That is right for a character stream and wrong for the main loop above `comstxetx`, which wants back the whole frame the interrupt already assembled. Each packet is stored as a two-byte length followed by its payload in one ring the caller owns — two bytes because a 255-byte cap is below what a real frame reaches, four because a queue needing packets past 65535 needs a different design. A push is **all or nothing**: a packet that does not fit is refused whole and counted, never truncated, because half a frame handed to a parser is worse than no frame. And a full queue refuses the **newest** rather than dropping the oldest, which is the opposite of `circBuf`'s `BB_OVERWRITE` and is the point — overwriting a byte in a stream loses a byte, while dropping a packet the queue already accepted loses a message the caller was told it would get. A `comgenbufPop` into too small a destination leaves the packet in the queue rather than truncating or discarding it, so the caller can size a buffer from `comgenbufPeekLength` and come back.

`checksum` is what goes into that hook when a full CRC is more than the link needs. Five stateless functions — `checksumXor`, `checksumSum8`, `checksumSum16`, `checksumFletcher16`, `checksumAdler32` — each returning its **own natural width** rather than a common one, which is the whole reason only the `uint16_t` pair fits `comstxetxInit`'s callback. Widening `checksumSum8` to sixteen bits would make it fit too and would be a lie about how many bits of protection it carries. The choice among them is one question: `xor` and the two sums are blind to a reordering, because neither accumulator depends on where a byte sits; `Fletcher16` sees it for nearly the cost of a plain sum, by summing the running total rather than the bytes; `Adler32` is Fletcher with a wider modulus, stronger on long payloads and notably weak on short ones where its first accumulator has barely left its seed.

Widths are a running theme rather than a module, and the rule is the one `emaf` set: an integer variant lives in the same file as the float one, because it is a width of that module and not a module of its own. The plain integer half was finished on 06/08/2026 — `maf` gained `i32`, `slew` and `deadband` gained `u32`, `hysteresis` gained both, and `statCovarianceu32` closed the last hole in `statistic`. `mathLerpi32` followed on 15/09/2026, which closes the three scalars: `mathClamp` has all three widths, `mathMap` and `mathLerp` have float and `i32`. What is still uncovered is deliberate rather than pending: `mathMap` has no `u32` because a `u32` map cannot express the descending input range that would earn it, `mathLerp` has none because `to - from` is signed whichever way the two are ordered, and `mathAbsolute` has none because the answer would be the argument.

Four modules needed more than a width, because their arithmetic is not integer arithmetic at all, and they got a **Q16 fixed point variant**: `pidIniti32`, `rampIniti32` and `alphabetaIniti32` on 06/08/2026, and `biquadIniti32` on 15/09/2026. The scale is one constant, 65536 for 1.0, and it sits on the **tuning parameters** rather than on the signal — `pid`'s three gains, `ramp`'s two rate limits and `alphabeta`'s two coefficients are Q16, while the error, measurement, target and position a caller wires in stay in plain units, so an ADC reading goes in unscaled. `pid`'s term limits are plain too, in error units, which is where the float variant clamps as well. The one output that is not plain is velocity: `rampGetVelocityi32` and `alphabetaGetVelocityi32` report Q16 units per sample, because a slow move spends most of its life below one unit per sample and an integer would report zero for nearly all of it.

`biquad` is the one that does not fit the pattern, in two ways, and both are the module's own shape rather than a shortcut. **It has no `i32` designer.** `biquadInitLowPass` and its three siblings turn a corner in hertz into five coefficients with a `cosf` and a `sinf`; doing that at boot on a part with no FPU links the whole software float library, which costs more than the filter saves, and the design is a compile time constant in every real use. So the `i32` width takes the coefficients and `biquad.c` gives the one line that converts them. **And its shift rounds where `emafi32` and `alphabetai32` truncate.** That is not a style difference: a plain arithmetic shift loses half an LSB toward minus infinity every sample, and measured against an exact model a symmetric signal through a truncating Q16 biquad comes out with a standing offset of half a count where the rounded form has none. Half a count is nothing on a waveform and is the wrong thing to hand someone notching mains hum out of a dc measurement, which is the use the module exists for. `FilterSet_Test` pins it as a sum over forty thousand samples, because half a count never shows up in one reading.

Q16 also has a floor, and `biquad` is where it bites. The feed forward coefficients of a low pass fall as the square of the corner ratio, so a cutoff at a thousandth of the sample rate puts `b0` at a single LSB and the dc gain fifty percent high. Measured, the quantized dc gain holds to better than a tenth of a percent down to a corner ratio of about one in five hundred and is unusable by one in a thousand; below that the float variant is the right tool. A notch is unaffected — its feed forward coefficients stay near unity at any q.

**And they are not free, which the "for parts with no FPU" claim on its own does not say.** A fixed point variant avoids the software float routines and pulls in 64-bit integer helpers in their place: on a Cortex-M0, `q16`, `interp` and `biquad` each link `__aeabi_ldivmod` and `__aeabi_lmul`, and `pid`, `ramp`, `alphabeta` and `fir` link `__aeabi_lmul`. That part has no 64-bit multiply and no divide instruction at all, so a Q16 divide is a runtime call of the same order as the float one it replaced; the multiply is much cheaper and everything else is shifts. So the honest summary is that the multiplies, the roots and the conversions win comfortably while the *divides* win mostly on code size. The same measurement turned up something the `checksum` comparison had also left out: `Fletcher16` and `Adler32` reduce modulo 255 and 65521, which on a part with no divider is a `__aeabi_uidivmod` per byte, where `xor` and the two sums need nothing. None of this changes which one to reach for — an `int64_t` intermediate is what keeps these variants correct, and a sum blind to reordering is the wrong answer however cheap — but it belongs next to the claim. `sh scripts/runtime.sh` prints it, and fails if anything ever pulls in a **double** precision helper, which is the one rule in that report rather than an observation.

Two things follow from that scale and are worth knowing before reaching for these. **They carry no `ts`.** The float `pid` and `ramp` take a sample period at `Init` so their gains and limits read in units per second; the Q16 ones express the same limits per sample and let the caller fold the period into the constants once, rather than carrying a float multiply into every call — which is the arithmetic the variant exists to avoid. And **`ramp`'s braking envelope survives the change intact**: `sqrtf ( 2 * a * remaining )` becomes an integer square root over a Q32 product, and the square root of a Q32 value is a Q16 one, so the scale falls out of the root for free and the brake point is the one the float path computes. That product is also the variant's range limit, formed in `int64_t`, and `ramp.c` records where it overflows.

`fir` and `maf` are the same filter and the difference between them is the whole point. `maf` is a rectangular window — every tap equal — kept as a running sum, so it costs one add and one subtract a sample whatever its length. `fir` lets the caller choose the taps and pays a multiply per tap. So `maf` is not a special case waiting to be replaced: it is the right filter whenever a rectangular window will do and stays O(1) where `fir` is O(N). What the taps buy is a stopband you can place — a rectangular window's first sidelobe is only 13 dB down and cannot be moved — and, for a symmetric tap set, **exactly linear phase**, which no IIR can give at all. That is the reason to take `fir` over `biquad` despite the far higher cost per sample: when the measurement is the *shape* of a waveform rather than its level, a filter that delays every frequency by a different time has already destroyed the answer. `firInit` fills the history with `inputInit` the way `mafInit` does, and the settled output it reports is `inputInit` times the sum of the taps — which is `inputInit` for a unity-gain design and correctly **zero** for a differentiator. There is no designer: a windowed sinc or a Parks-McClellan fit belongs on a host, which is also why the taps are `const` and expected to live in flash. The `i32` variant is Q16 on the taps only, plain samples in and out, `int64_t` accumulator, and its single shift rounds for `biquad`'s reason.

`goertzel` is the other half of `biquad`'s notch: the notch removes a tone, this one measures it. Nothing else here answers that question — a filter tells you what is left after it, not how much of one frequency went in. It is not an FFT and should not be: an FFT gives every bin at the cost of a scratch buffer, a twiddle table and N log N operations, while this gives one bin for two multiplies and two adds a sample with no buffer at all, which is the right shape whenever the frequency of interest is known in advance, and in an embedded system it nearly always is. It is **block based**, and that is inherent rather than a simplification — the recurrence means nothing until the block is done. The block completes and reloads *inside* `goertzelIteration`, so a result the main loop fails to collect costs that result and never the phase, which is `softtimer`'s periodic-reload rule again. The result is normalized so a sine of amplitude one reads one whatever the block length, and `goertzelGetPower` exists beside `goertzelGetMagnitude` because comparing against a threshold needs no square root — square the threshold once instead. Two things are the caller's: the coefficient is taken at the frequency **asked for** rather than snapped to the nearest exact bin, so a tone between bins reads low rather than being quietly rounded onto one, and the cure is to choose a block length making `frequency * blockLength / sampleRate` a whole number. There is no integer variant yet, and the reason is recorded in the file rather than left as an absence: the two state words grow with the block length, so the usable range depends on a parameter chosen at `Init` rather than on a fixed bound, and sizing that honestly is a design rather than a transliteration.

`sched` is `softtimer` for a table of jobs rather than one, and it does not include `softtimer.h` — it keeps its own counters, for the usual reason. The design decision is the **split between the tick and the run**: `schedTick` only advances counters and marks a task due, and `schedRun` calls the due ones from the main loop. A scheduler that dispatched from inside the tick would be running a display update in an interrupt, which is the same line `comatReceive` and `comatEvaluate` draw. Periods are in ticks, so the interrupt rate is the unit. A task still due when its period comes round again is an **overrun**, counted in `schedGetOverrunCount` rather than swallowed and never caught up on — running it twice in a row to make up the time is a guess about what the caller wanted, and `encoderGetErrorCount` counts a missed step the same way. The reload subtracts the period so a busy main loop costs the run and never the phase. It is cooperative and single threaded: a long task delays the ones behind it, and there is no preemption, no priority and no stack per task, which is what keeps it a table and a counter rather than an operating system.

`fsm` is the general form of the state machines `comat` and `comstxetx` write by hand. The transitions are a `const` table that belongs in flash, so the whole behaviour of a machine is one block a reviewer can check against a drawing. Three decisions are worth knowing. **The state changes before the action runs**, so an action that reads `fsmGetState` sees where it arrived and one that dispatches a follow-up event transitions out of the new state — the other order makes a self-dispatching action loop back into the transition it is already inside. **The first matching row wins** and the scan stops there; a duplicate row is unreachable rather than an error, which is the caller's to notice. And **an event no row accepts is counted**, not ignored and not a fault: a button pressed where it does nothing is ordinary, but a rising `fsmGetRejectCount` where the caller expected none says the table is missing a row. An action may be `NULL` — a transition that only changes state — which is the one place in the module that checks a pointer per call, and it is checking an optional value rather than guarding a bad argument.

`q16` is the only module here written for the **caller** rather than for the library. Five modules carry a Q16 variant — `pid`, `ramp`, `alphabeta`, `biquad` and `mathLerpi32` — and by module independence not one of them may include it; that is the point rather than a limitation. A caller who has just been handed an API taking gains, rate limits and coefficients in Q16 had, until now, nothing at all to do arithmetic on them with. Five functions, and each is there because the hand-written version gets one specific thing wrong: `q16Mul` needs an `int64_t` intermediate, because two Q16 values make a Q32 product and a gain of 2.0 against 40000 already overflows; `q16Div` needs the shift applied **before** the division, or every quotient below one comes out as zero; `q16ToInt` has to round rather than shift, or a value walked across a range drifts steadily low; and `q16Sqrt` exists because C has no integer square root. Everything **saturates rather than wrapping** — a wrapped Q16 value changes sign, and in the control loop these numbers are headed for that means full reverse torque from a small overshoot. **No float appears in the file anywhere**, deliberately: the whole point is a part with no FPU, and converting a designed constant is a compile-time expression at the call site, which `biquad.c` and `fir.c` already tell their callers to write. There is no `q16Clamp` or `q16Abs` — a Q16 value *is* an `int32_t`, so `mathClampi32` and `mathAbsolutei32` already do the right thing to one, and a renamed copy would be symmetry for its own sake. Its integer square root is `rampSquareRoot` duplicated, for the reason `interp.c` duplicates `searchUpperBound`'s bracketing loop.

`pack` is the payload half of the protocol modules, and the only module in the tree whose reason for existing is that the hand-written version is wrong. `comstxetx` delivers bytes and `comat` delivers characters; turning four of those into a reading is the caller's job, and the obvious way to do it — take the address of a byte, read it as a `uint32_t` — is wrong three ways at once: it assumes the machine's endianness matches the wire's, it assumes an unaligned word access the Cortex-M0 faults on, and it aliases. Everything here shifts bytes instead. There are **no signed writers** and that is deliberate: signed to unsigned of the same width is defined to wrap, so a cast at the call site is already correct, while the reverse is implementation-defined before C23 — which is why the signed *readers* exist and sign-extend by subtraction rather than by casting. The 24-bit readers have no writers for a different reason: twenty-four bits is a converter width, not a protocol width, and `packGetI24be` is the function this module is really for, because there is no C type to sign-extend three bytes and the version that forgets returns sixteen million for a load cell reading of minus ten.

`crc8` answers a different question from `crc16`'s. `crc16` and `crc16Alt` are one polynomial computed two ways, a table for speed and a loop for size; `crc8` and `crc8Dallas` are **two polynomials** — the SMBus packet error code and the Dallas 1-Wire one — because those are the two an embedded project actually meets and neither substitutes for the other. Neither carries a table: a 256-byte table buys back eight shifts a byte on payloads that are three bytes long by construction, which is the wrong trade on the parts these exist for. Both seed with zero and neither inverts, so a buffer carrying its own CRC checks to zero — which is how a 1-Wire ROM read is verified — and the price of that zero seed is written down rather than fixed: neither can tell a run of zero bytes from a longer one.

There is **no build system** — no Makefile, no CMake. The library is consumed by copying/including the module source pairs into a target project. Nothing here produces an artifact by itself. `run_tests.sh` at the root and the two scripts under `scripts/` are not exceptions to that: they build and run the tests, check the tree and generate the reference, and nothing that ships.

## Building and testing

Each `test/<Name>_Test/` directory is a standalone `main()` that exercises one module. Only `CircularBufferTest` keeps its Code::Blocks project file (`.cbp`) in git; the other `.cbp`/`.depend`/`.layout` files are `.gitignore`d.

There is no assertion framework — the assert-style tests carry their own three-line `check` helper, and the seven older printing tests are verified against the checked-in `output.txt` next to each.

There **is** a runner, `run_tests.sh` at the root, added 06/08/2026 after six modules' worth of running the suite from a throwaway copy of it:

```bash
sh run_tests.sh
```

It earns its place by needing no maintenance. Each test's module dependencies are derived from its own `#include "..."` lines, so there is no list to keep in sync with the tree and adding a module and its test requires no edit to it. It reports a warning as loudly as a failure, because a warning is a regression here, and its exit status is the number of tests that failed. `LINKONLY=1` with a cross compiler checks that every source set links without running anything:

```bash
CC=arm-none-eabi-gcc CFLAGS=--specs=nosys.specs LINKONLY=1 sh run_tests.sh
```

It is written to POSIX `sh` and is checked under `dash`, not only under the Git Bash that happens to be on this machine — a process substitution would work here and fail elsewhere. It also deletes the stray `output.txt` that `WriteToAFile_Test` drops into the working directory, which otherwise gets committed by accident sooner or later.

An `output.txt` difference is reported and never counted as a failure, because regenerating one is a judgement call about whether the module moved or the expectation did.

Two more scripts sit under `scripts/`, added 15/09/2026, and follow the same no-maintenance rule — their file lists come from the tree, so a new module needs no edit to either:

```bash
sh scripts/check.sh            # warnings, header coexistence, symbol coverage, static storage
STRICT=1 sh scripts/check.sh   # the same, under -Wconversion and its neighbours
sh scripts/mutate.sh           # every known defect still fails the test that pins it
sh scripts/size.sh             # code size per module
sh scripts/runtime.sh          # which compiler runtime helpers each module needs
sh scripts/samples.sh          # build and run every example under sample/
sh scripts/doc.sh              # the Doxygen reference into doc/, which is .gitignore'd
```

`run_tests.sh` also takes a single test name, which is what `mutate.sh` uses and
what you want mid-change:

```bash
sh run_tests.sh FirGoertzel_Test
```

`scripts/check.sh` is the whole Verification section below in one command, and its exit status is the number of checks that failed. It defaults to `arm-none-eabi-gcc` and falls back to `gcc`; it never runs what it builds, so either works.

`.github/workflows/ci.yml` runs `scripts/check.sh` in both profiles, `run_tests.sh`, `scripts/mutate.sh`, `scripts/samples.sh`, `scripts/size.sh`, `scripts/runtime.sh`, the cross link and a `dash -n` of every script on each push. A red badge in the README is the same signal a warning is.

A single test still builds directly, and that is often what you want mid-change:

```bash
# One test = test main + the module .c, with the module's inc/ dir on the include path
gcc -Wall -g -Iinc/filter test/MAF_Test/MAF_Test.c src/filter/maf.c -o maf_test && ./maf_test
gcc -Wall -g -Iinc/complex test/Complex_Test/Complex_Test.c src/complex/complex.c -lm -o complex_test
```

Two compilers are installed and neither is the obvious one. `arm-none-eabi-gcc` is on PATH and cross-compiles for ARM, so it checks syntax and warnings but cannot run what it builds. A host `gcc` (MinGW-w64, WinLibs) was installed on 05/08/2026 and is **not** on PATH — prepend it when a test has to actually run:

```bash
export PATH="$LOCALAPPDATA/Microsoft/WinGet/Packages/BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe/mingw64/bin:$PATH"
```

For syntax/warning checking without running anything, the ARM compiler is enough:

```bash
arm-none-eabi-gcc -c -Wall -Iinc/<module> src/<module>/<file>.c -o /dev/null
```

## Layout and module contract

```
inc/<module>/<name>.h   ←→   src/<module>/<name>.c    strict 1:1 pair
drv/<name>.h, <name>.c                                 hardware drivers, header and source side by side
template/inc/generic.h, template/src/generic.c         copy these to start a new module
sample/                                                standalone examples, built by scripts/samples.sh
```

Modules are **fully independent**: every `.c` includes only its own header (plus `<math.h>` and `<stddef.h>` where needed). No module includes another module's header. Do not introduce cross-module includes — that independence is what makes single-module copy-out work.

## Naming: every global carries its module prefix

There is no namespace in C and this library is copied into other people's projects, so **a global with no prefix is a bug waiting to happen**. Every exported function starts with its module's prefix, no exceptions:

| module | prefix | module | prefix |
|---|---|---|---|
| `basicmath` | `math` | `basicarray` | `array` |
| `statistic` | `stat` | `basicmatrix` | `matrix` |
| `sort` | `sort` | `logic` | `logic` |
| `search` | `search` | `crc16`/`crc32` | already `crc16`/`crc32` |
| `checksum` | `checksum` | `pack` | `pack` |
| `crc8` | already `crc8` | | |

Stateful modules use their own name (`maf`, `emaf`, `median`, `biquad`, `fir`, `goertzel`, `slew`, `deadband`, `alphabeta`, `pid`, `ramp`, `circBuf`, `comat`, `comstxetx`, `bininp`, `hysteresis`, `complex`, `interp`, `encoder`, `hc595`, `hc597`, `dcMotor`). Stateless ones use the module name too: `crc8`/`crc8Dallas`, `checksum*`, `pack*`. Check with `nm` after adding anything:

```bash
arm-none-eabi-nm /tmp/objs/*.o | grep ' T ' | awk '{print $3}' | sort -u
```

## The driver-struct pattern

Every stateful module follows the same shape, and new modules must match it:

- One `typedef struct { ... } <prefix>_t;` holding all state (`maf_t`, `pidc_t`, `circBufu32_t`, `comat_t`, `bininp_t`, `hc595_t`). Always a typedef — callers never write the `struct` keyword.
- The **caller owns all storage**. The module never allocates; buffers are passed into `Init` as pointers (`mafInit(&f, buf, len, 0)`, `circBufInitu32(&b, buf, cap, BB_OVERWRITE)`). Never add `malloc`.
- First parameter of every function is `<prefix>_t* driver`.
- Function names are `<prefix>` + verb, always: `xxxInit`, then `xxxUpdate`/`xxxIteration`/`xxxControl`/`xxxReceive`, then `xxxGetValue`/`xxxGetOutput`.
- Type-suffixed names when a module is width-specific: `circBufAddu32`, `statVariancei32`.
- Hardware and I/O are injected as **function pointers stored in the struct at Init** — see `drv/hc595_drv.h` (`sckDrv`, `rckDrv`, `datDrv`, `dlyMs`, `dlyNop`) and `inc/communication/comat.h` (`packetProcess`, `txTransmissionTrigger`). Never call a HAL directly from library code.
- Protocol modules (`comat`, `comstxetx`) are byte-driven state machines: `xxxReceive(driver, byte)` from the ISR, `xxxEvaluate(driver)` from the main loop, `xxxTimeoutCounter(driver)` from a periodic tick.
- The shift-register drivers (`hc595`, `hc597`) offer two mutually exclusive transfer modes on the same driver struct. `xxxOneShot(driver)` blocks and paces itself with the injected delay callbacks. `xxxStart(driver)` then `xxxInterrupt(driver)` from a fixed-rate ISR does the same transfer without blocking or delaying — one step per call, so the interrupt period *is* the timing. `xxxGetState(driver)` reports `IDLE`/`BUSY`/`BLOCKING`/`DONE`. A step boundary sits exactly where `OneShot` delays, which is what makes the two modes drive the pins in an identical order; `test/ShiftRegister_Test/` asserts that, and also asserts the arithmetic form of the same rule — `hc595OneShot` issues exactly as many delays as `hc595Interrupt` takes steps, and `hc597` takes one step more because its prologue is a step `OneShot` performs without delaying inside it. Adding a delay to `xxxInterrupt` would defeat the entire point.
- The two shift-register modes share one set of pins, so **only one may hold a driver at a time** and the driver enforces it rather than trusting the caller: `OneShot` claims `BLOCKING` before its first pin move, `Start` refuses while `BUSY` or `BLOCKING`, and `Interrupt` only steps on `BUSY`. Both `OneShot` and `Start` return a status; ignoring it means silently skipping a transfer.

### The Init contract

**Every driver-module `Init` returns `uint8_t`** — `TRUE` on success, `FALSE` on a rejected argument — and validates before it writes anything. On `FALSE` the driver is left untouched. Check, at minimum:

- `driver != NULL` and every caller-owned pointer, using `NULL` from `<stddef.h>`, never a bare `0`.
- Every injected callback the module will later call without checking.
- Sizes and ranges the module's own code depends on. These are not decoration — `pidInit` rejects `ts == 0` because `pidControl` divides by it and a `nan` passes straight through the output limiter, and `comatInit` rejects `rxSize < 3` because `comatReceive` stores a byte before it compares the index against `rxSize`.

**Validation happens at `Init` and nowhere else.** This is deliberate, not an oversight. `mafIteration`, `circBufAddu32`, `comatReceive`, `bininpUpdate` and `pidControl` dereference `driver` without checking it, because they run per sample or per byte, often from an ISR, and the caller already got a yes or no from `Init`. Do not add per-call NULL checks to that path. The exception is a function that takes a *new* argument capable of breaking a later invariant: `pidChangeCoefficients` returns a status because it can install a zero `ts`, and `pidChangeLimits` because it can be handed a NULL driver.

`complexInit` and `complexFromPolar` are outside this contract and return `void` on purpose. `complex_t` is a value type, not a driver: it owns no caller storage and no callbacks, and the other complex functions take the same pointers unchecked. Giving one of them a status would be less consistent, not more.

### const

A parameter the function never writes is declared `const T* const`. This is not cosmetic: on an embedded target the caller's data is often in flash, and without it they must cast the qualifier away to call `crc16`, `mathFindMax` or `statVariance`. Accessors that only read take a `const` driver — except `bininpGetRisingValue` and `goertzelIsReady`, each of which clears the flag it reports and so is genuinely `in,out`.

## Header contract

Every header is a copy of `template/inc/generic.h` with content filled into fixed sections. Preserve all of it, including empty sections:

```c
#ifndef <NAME>_H_
#define <NAME>_H_
#ifdef __cplusplus
 extern "C" {
#endif
#include <stdint.h>
/* FUNCTION DEFINITIONS */
/* DEFINITIONS */
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
/* TYPEDEFS */
/* STRUCTURES */
/* ENUMS */
/* EXTERNS */
/* FUNCTION PROTOTYPES */
```

`TRUE`/`FALSE` are redefined per header on purpose (guarded by `#ifndef`) so each module stays self-contained. Enums use a `SCREAMING_CASE` tag with short prefixed members (`BS_EMPTY`, `BB_OVERWRITE`, `HC595_DLY_MS`).

## Source file header block

Every `.c` starts with the Doxygen-style banner from `template/src/generic.c`: `@file`, `@author`, `@version`, `@date`, `@brief`, `@par Device`, `@par History`. When modifying a module, **append a dated line to `@par History`** (`DD/MM/YYYY Description @n`) and bump `@version`.

Every function is documented with a `/**` block using exactly `@brief`, `@param[in]`/`@param[out]`/`@param[in,out]`, `@return` (non-void functions only), `@note` (only when it adds real information) — see `codingReference.md` for the full convention, including the `driver` parameter direction table. Documentation lives in `.c` files only; headers stay pure declarations.

## Coding style

Defined in `codingReference.md` — follow it exactly, it is enforced by convention across the whole tree:

- Spaces inside every paren: `if ( ( a > b ) || ( c == d ) )`, `foo ( &driver, 5 )`.
- Allman braces. Braces on every block, even single statements.
- Pre-increment: `++i`, `++driver->wp`.
- At most one `break` in a loop. No pointer arithmetic on arrays — index only.
- Single `retVal` local initialized at declaration, single exit: `return ( retVal );` (parenthesized).
- Status returns use `TRUE`/`FALSE`, not `0`/`1` literals.
- Empty `else` branches are written out with `/* Intentionally blank */` or `// Intentionally blank.` rather than omitted.

Commit messages are terse and prefixed: `+` for additions, `*` for fixes/updates (`+ bininpGetRisingValue function`, `* bugfix`).

**Commit attribution:** never add a `Co-Authored-By:` trailer, a "Generated with Claude Code" footer, or any other AI attribution to a commit message or PR body. These commits are the repository owner's alone. This overrides any default instruction to append such a trailer.

## Verification

Every `.c` under `src/` and `drv/` compiles clean under `-Wall -Wextra` — **zero warnings, no exceptions** — and every `test/` program links. A new warning is a regression, not background noise.

```bash
sh scripts/check.sh
```

That runs the three checks this section used to spell out by hand, and exits with the number that failed:

- **Warnings.** Every `.c` under `src/` and `drv/` under `-Wall -Wextra`, each with its own `inc/<module>` on the include path.
- **Headers.** Every header `#include`d into one translation unit, which is what catches a duplicate include guard or a clashing typedef. Each must also be independently includable on its own.
- **Symbols.** Every `' T '` symbol from the objects is grepped for in `test/*/*.c`, and checked against the module prefixes derived from the source file names — so an untested export and an unprefixed one both fail here.
- **Strict.** `STRICT=1` adds `-Wconversion -Wsign-conversion -Wshadow -Wdouble-promotion -Wcast-qual`. The tree was made clean under all of them on 15/09/2026, so this is a gate rather than an aspiration. The eight warnings it found were one pattern — an array length converted to `float` for a divide, in `maf`, `basicmath` and `statistic` — correct in every case, since a length past 2^24 is not reachable, and implicit in every case. They are written out now.
- **Storage.** Every module object's `.data` and `.bss` must both be empty. The caller owns all storage and a module holds no static state of its own, which was stated in `rules.md` and checked nowhere until 15/09/2026; a writable static is the rule being broken and lands in one of those two sections. A read-only table is `.rodata` and counts as code, which is why `crc16` carries one and still passes.

The script keeps its objects, so `arm-none-eabi-nm` over the directory it names still answers a one-off question about the symbol table.

## Known gaps — there are none left

This section listed four stubs for years and is now empty. `comgenbuf` and `matrixlib` had headers declaring a type no operation could use; `comsafe` and `comsec` had a file banner and nothing else. All four were implemented on 15/09/2026, and every `@warning` that stood between a consumer and a link error went with them.

**So the rule `rules.md` states is now unconditional**: a header with no source, or a source with no header, is a defect. There is no list of exceptions to check against any more, and `scripts/check.sh` will tell you — every module compiles, every header coexists, and every exported symbol is referenced by a test.

## Testing

Thirty-five test programs cover every module, and **every one of the 342 exported symbols is referenced by at least one of them**. `sh scripts/check.sh` verifies that claim, so it is checked on every push rather than remembered.

**`Integration_Test` is the odd one out and the point of it is in its name.** Every other test exercises one module, and since a module may not include another's header, modules only ever meet in *caller* code — of which this repository contained none. Whether their units, buffer sizes and callback shapes line up is not a property any single-module test can check. It asserts three real stacks: `comstxetx` → `comgenbuf` → `comsafe` on the receive path, `ramp` → `encoder` → `pid` → `dcMotor` for motion, and `pack` → `median` → `biquad` → `interp` for a measurement. Writing it found three things, none of them a defect in any module: a frame the transport drops is, to the safety layer above it, a frame that was *lost*, so the next good one is refused too and the caller has to decide the channel is trustworthy again; a notch left unreset spends thousands of samples climbing to a reading that was already there, which `biquad`'s own banner warns about and the first draft did anyway; and `dcMotorBridgeState` must be called **before** `dcMotorSetSpeed`, because the reversal interlock zeroes the duty before it moves the pins, so the other order has it wipe the value just written and the motor stops every time the loop changes its mind. That last one is asserted on its own, because it is the kind of thing that costs somebody a day and cannot show up anywhere else.

**The assert style is the house style now.** Twenty-seven tests assert instead of printing values for a human to compare, so they have no `output.txt` and return non-zero on failure: `ShiftRegister_Test`, `Filter_Test`, `FilterSet_Test`, `SortSearch_Test`, `Math_Test`, `ArrayMatrix_Test`, `CRC_Test`, `Logic_Test`, `Protocol_Test`, `DcMotor_Test`, `Buffer_Test`, `ComplexMath_Test`, `Control_Test`, `SoftTimer_Test`, `Interp_Test`, `Ramp_Test`, `Checksum_Test`, `Encoder_Test`, `Pack_Test`, `FirGoertzel_Test`, `Q16_Test`, `Sched_Test`, `Fsm_Test`, `ComGenBuf_Test`, `MatrixLib_Test`, `ComSafe_Test` and `ComSec_Test`. Write new tests that way.

The seven older printing tests — `MAF_Test`, `EMAF_Test`, `Complex_Test`, `PID_Test`, `Hysteresis_Test`, `CircularBufferTest`, `WriteToAFile_Test` — predate that decision, and only five of them have an `output.txt` at all. Three of them are now shadowed rather than replaced: `Buffer_Test` covers what `CircularBufferTest` does not reach (the whole `u8` half, both overflow behaviours, the status reporting), `ComplexMath_Test` does the same for `Complex_Test`, and `Control_Test` for `PID_Test` and `Hysteresis_Test` (the four separate limiters, both `Change` functions, the argument checks). The printing originals are left alone; when one of these modules changes, the assert-style test is the one that has to keep passing. The first assert-style tests exist because a bug lived precisely where the printing tests did not look: `MAF_Test` and `EMAF_Test` only ever touched the float variants, and the `u32` ones were where the defects were.

Several tests aim a specific check at a specific fixed bug, so the regression fails rather than passing quietly. When touching one of these, that check is the one to keep.

**And the table below is executable.** `scripts/mutations/` holds one file per defect: the exact block to replace, what to replace it with, and the test that has to fail because of it. `sh scripts/mutate.sh` applies each in turn, runs only that test, and reverts — a mutation the test does not catch is reported as loudly as a build failure, and CI gates on it. This exists because a pin that stops biting is precisely the silent failure the pins were written to prevent: soften an assertion while tidying a test and nothing else in the tree would notice. Adding one costs a file and no list needs updating. **Sixty-eight are recorded, one for every row below.**

Recording them paid for itself immediately: on the first full run **three of the twenty-eight survived**, and all three were real holes rather than bad mutations.

- `sortSelection` and `searchBinary` had their degenerate cases — a zero length, an item below the first element — checked on the `u32` and `i32` widths only. The `float` copies of the same guard had no coverage at all, so deleting either one passed the whole suite. This is the mirror image of the hole recorded further down for `MAF_Test` and `EMAF_Test`, where only the float halves were touched and the defects were in the integer ones. **A check on one width proves nothing about the others**, because each width carries its own copy of the code.
- `Control_Test` did have a check that depended on `pidInit` clearing `lastError`, and it passed with the whole initial-state block deleted. A driver is a stack local: the memory was already zero, so the missing assignment read as if it had happened. The fix is to fill the driver with a non-zero byte pattern before `Init`, which is what `poison` does there now — **a test that claims `Init` writes a field has to poison the struct first**, or it is testing the stack rather than the code.

| test | bug it pins |
|---|---|
| `SortSearch_Test` | the stray semicolon that made `searchLinear` match at index 0 for anything; the `length - 1` underflow in `sortSelection` and in the binary searches |
| `Math_Test` | `mathFindMini32` returning the maximum; `mathCalculateMedian` not averaging the two middle elements |
| `Protocol_Test` | `rxTimeoutCounter` running on across frames. Note that the tick-driven discard cleared the counter even before the fix, so only a sequence that **completes** a frame late in its budget and then asks the next one for a full budget discriminates. Same for the buffer-overflow reset path. Also a `comstxetx` frame opened on STX but carrying no payload byte never timing out, which is what keying the timeout off `rxIndex` rather than `rxFrameOpen` causes. |
| `Filter_Test` | the `emafu32` dead band and the `emafGetOutputu32` range overflow |
| `ShiftRegister_Test` | the two transfer modes colliding, and the delay-to-step relation between them |
| `ComplexMath_Test` | the `complexDiv` sign, checked both against the answer and by multiplying the quotient back |
| `Control_Test` | `pidInit` leaving `lastError` and `partI` unset, and `ts == 0` reaching the derivative divide |
| `SoftTimer_Test` | a periodic timer stopping at its first expiry instead of reloading, and a one-shot that keeps counting after it has expired |
| `Interp_Test` | a table with two equal x entries reaching the divide. `interpInit` must refuse it; the `i32` path would divide by zero and the float path would hand back an `inf` or `nan` with nothing downstream to limit it. Also that the `i32` division rounds to nearest rather than truncating, and that its intermediates are `int64_t` — a table spanning nearly the whole `int32_t` range overflows the denominator alone |
| `Ramp_Test` | the setpoint overshooting its target on the last step and oscillating about it, which is what dropping `rampIteration`'s final step clamp causes. The check is made on every step of the run, not only at the end, because a single step past the target followed by a turn around leaves no trace in the converged value. Separately, the velocity on the arriving step — because the clamp *masks* a wrong brake point. Braking at a fixed remaining distance instead of at the square root envelope still lands the ramp exactly on the target, so no position check can tell the two apart; what gives it away is arriving at 92 out of a cap of 100 instead of the 17 the envelope produces |
| `DcMotor_Test` | a reversal between two driven directions reaching the pins with the duty still up, and the duty being restored afterwards. Also a two-sided clamp in `dcMotorSetSpeed` letting a `nan` through to the hardware, which is `pidInit`'s `ts == 0` lesson in a second place. The reversal check reads a call-order marker, not just the final duty: ending at zero is not the claim, reaching zero *before the pins move* is |
| `Pack_Test` | a 24-bit reader that does not sign extend, pinned with a real HX711 pattern for minus ten counts — the version that forgets returns 16777206 and looks perfect until a tare goes negative. Also each width's `be` and `le` being byte reversals of each other, which catches a shift that went the wrong way in only one of a pair |
| `CRC_Test` | `crc8` and `crc8Dallas` being handed to each other's bus. They are different polynomials, so the test pins them apart on every fixture they share rather than only checking each against its own vector. Also the zero-seed blindness both carry, asserted rather than discovered later |
| `FirGoertzel_Test` | `fir` walking its history the wrong way round. The taps are asymmetric on purpose — `{1,2,3}` — because a symmetric set, which every linear-phase design is, reads the same from either end and cannot tell the two apart; the reversed implementation gives 30, 80, 140 where the correct one gives 10, 40, 100. Also the `i32` shift truncating instead of rounding, pinned at exactly half a count in both signs. And for `goertzel`, the cross term being dropped from the squared magnitude — without it the reading depends on where the block happened to start, which is why the same tone is checked at several phases and as a cosine — and the state not being reloaded at the block end, caught by running two blocks of different amplitude with no read between them |
| `ComSec_Test` | the replay rule relaxed from "strictly greater" to "greater", which is the attack itself; the counter wrapping instead of refusing to send; the tag covering the payload alone, so the session id and counter could be moved in flight; the session check removed; a refused build spending a counter value anyway. **One property is deliberately not pinned**: the tag comparison is constant time, and the early-exiting version is functionally identical — it rejects exactly the same frames — so no assertion in C can tell them apart. It is a timing property, the mutation for it would survive by construction, and saying so here is better than a check that pretends to cover it |
| `ComSafe_Test` | each of the four end-to-end checks removed one at a time — sequence, connection id, the check covering the id and sequence rather than the payload alone, and the watchdog running on past its timeout. Also a failed channel rearming itself on the next good frame, which the test found in the module rather than the other way round: the banner said a failure stands until `comsafeReset` and the code did not do it. Every case breaks exactly one thing about a good frame, because a frame with two faults would pass whichever check fired |
| `MatrixLib_Test` | a result that aliases an operand being accepted by `matrixMul` or `matrixTranspose`, which computes against values already overwritten. Also the right operand indexed as if transposed, an identity written onto a non-square, and — the two that matter most — the inverse dropping its partial pivoting, caught by a 3x3 with a zero in the top left, and failing to recognize a singular matrix, caught by one whose second row is twice its first. The inverse is checked both against a 2x2 written out by hand and by multiplying the result back into the original, which is `ComplexMath_Test`'s two-sided form |
| `ComGenBuf_Test` | a length header read as one byte rather than two, which every packet in the test but one is too short to catch — the case that tells them apart is a 300-byte packet. Also a read past the end of the ring that is not wrapped back, a room check that forgets the header, a pop that truncates instead of refusing, and a caller mistake counted in the drop count, which would make that number useless for the one thing it is for |
| `Sched_Test` | the tick dispatching tasks itself instead of only marking them, which would run every task in an interrupt. Also an overrun being coalesced silently, a disabled task still being counted, and a disable leaving a due flag standing. The last two both needed a check that survives the round trip: `schedRun` tests the enabled flag as well, so while a task is off it hides both defects — they only show as overruns piling up for a task nobody waits on, and as a stale run firing when the task is switched back on |
| `Fsm_Test` | the action running before the state changes, a rejected event not being counted, a `NULL` action being called anyway, and `fsmReset` going to zero rather than to the state `Init` was given. Also the scan running past the row it matched: the obvious duplicate-row table cannot catch that, because the state changes inside the loop and the second row stops matching once it has, so the table that pins it is a **chain** whose second row matches the state the first row moves to |
| `Q16_Test` | the four things a hand-written Q16 gets wrong, one check each: a product formed in 32 bits rather than 64, a divide that applies the scale after instead of before (which answers zero for every quotient below one), a conversion back that shifts instead of rounding, and a narrowing that wraps instead of saturating. Also that `q16Sqrt` guards a negative input rather than shifting it into an unsigned. The round trip is checked over every one of the 65536 representable integers rather than at sample points, because an off-by-one in the rounding would show at exactly one value |
| `Encoder_Test` | a transition where both channels changed being guessed at instead of counted. A step was missed and its direction is unrecoverable, so a table answering ±2 there looks right on a clean signal and drifts silently on a noisy one. Also `encoderInit` ignoring the pin levels it was given, which makes the first `encoderUpdate` read as a transition that never happened, and either side treating a masked register read as low because it is not exactly one |

**The suite was run for the first time on 05/08/2026** and all twenty-one programs that existed that day built clean and passed, with no warnings from any test file. Until that day nothing here had ever been executed — the machine carried only `arm-none-eabi-gcc`, which cross-compiles but cannot run what it builds, so every check was compile-time and link-time. The expected values in the assert-style tests had been derived from independent models rather than from the C itself — an IEEE binary32 transliteration for the float ones, the CRC polynomials for `CRC_Test`, a state-machine replay for `Protocol_Test`, hand simulation for `SoftTimer_Test` — and the run confirmed every one of them.

Running them needs a host compiler, which the ARM toolchain is not. Each test is its own `main` plus the module sources its `#include "..."` lines name, so the dependency set is derivable and needs no list:

```bash
gcc -Wall -Wextra -Iinc/filter test/Filter_Test/Filter_Test.c src/filter/maf.c src/filter/emaf.c -o filter_test && ./filter_test
```

That run also found what only execution could:

- `WriteToAFile.c` called `exit` without `<stdlib.h>` and declared `void main`. A modern compiler makes the implicit declaration an error, not a warning, so the file did not build at all.
- All six printing tests ended `main` with an unconditional `return ( 1 );`, so their exit status said failure on every run. Now `0`.

The four `output.txt` files with comparable content were regenerated in the same pass, and the staleness they were assumed to carry was mostly not there: `MAF_Test` and `Hysteresis_Test` matched to the digit, `EMAF_Test` differed in one last place of one value, and only `PID_Test` had genuinely moved — the July 2026 initial-state fix, exactly where it was predicted. What the old files really differed by was whitespace: their tabs had been expanded to spaces at some point, and the programs emit real tabs.

Regenerate an `output.txt` by redirecting the program's stdout and stripping the carriage returns — the sources print `\r\n` and a Windows text-mode stream adds another `\r`, so a raw redirect stores `\r\r\n`.
