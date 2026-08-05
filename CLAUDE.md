# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this repo is

`esclib` is a freestanding general-purpose C library for embedded targets: filters, PID/hysteresis control, circular buffer, CRC, sort/search, matrix/complex math, serial protocol handlers, soft timers, and shift-register drivers. No heap allocation, no OS dependency, C89-compatible style, `<stdint.h>` types throughout.

The `filter/` group is the largest and each module there answers a different problem, so pick by what is wrong with the signal rather than by habit: `maf`/`emaf` smooth, `median` rejects impulses outright, `biquad` shapes a response in hertz (and is the only way to notch mains hum), `slew` bounds the rate of change, `deadband` holds the output still until the input really moves, and `alphabeta` estimates position *and* velocity. `emaf` also carries an integer-only variant, `emafIniti32`/`emafIterationi32`, for parts with no FPU — it is a width variant of `emaf` rather than a module of its own, so the float code shares the file with it.

`sort` and `search` pair up and the choice within each is the same kind of decision. Among the sorts, `sortInsertion` wins on short or nearly-sorted arrays and is stable, `sortHeap` is the only one with an O(N log N) *guarantee* and stays in place without recursing, and `sortSelection`/`sortBubble` are there for teaching rather than for speed. Every sort produces ascending order; `sortReverse` turns that into descending in one pass, which is why there is no descending variant of each. Among the searches, `searchBinary` answers whether a value is present, `searchLowerBound`/`searchUpperBound` answer where it belongs — the insertion point, and their difference is the number of duplicates — and `searchClosest` answers which entry to read, which is the first half of what a calibration or linearisation table needs. All of the binary ones require ascending order and give a confident wrong answer without it, so `sortIsSorted` exists to check that precondition cheaply.

`interp` is the other half of that. `searchClosest` says which table entry to read, so the caller gets a value that is actually in the table; `interp` gives the value *between* two entries, which is what a calibration or linearisation curve is usually asked for. It is a driver rather than a stateless function for one reason: ascending order is a precondition here exactly as it is for the binary searches, and a driver has somewhere to check it. `interpInit` verifies strict ascent once, at boot, so `interpCalculate` can divide without testing the divisor — a repeated x would zero it. Past either end the value is held rather than extrapolated, and `interpInRange` answers separately whether that happened, for the caller who has to tell a saturated reading from a broken sensor. Reverse lookup needs no function of its own: a second `interp_t` initialized with the x and y tables exchanged reads the same curve backwards. The `i32` variant exists for the same reason `emaf`'s does — a part with no FPU — and carries every intermediate in `int64_t`, which is the only place in this tree that width appears.

`softtimer` is the library's only time abstraction. It counts calls to `softtimerTick`, which the caller makes from a fixed-rate ISR, so the period is expressed in ticks and the interrupt rate is the unit — the same rule `hc595Interrupt` follows. One-shot and periodic modes share one struct; the periodic reload happens inside the tick rather than at the read, which is what lets an expiry the main loop fails to read cost the event but never the phase. The reload itself subtracts the period rather than clearing the counter, a form that stays correct if the counter ever overshoots today's invariant that it lands exactly on the period — the two forms are equivalent as things stand, and no test of this API distinguishes them. It is deliberately not consumed by `comat` or the shift-register drivers: they keep their own counters, because module independence forbids one module including another's header.

The two protocol modules answer different problems and only one of them checks what it receives. `comstxetx` frames binary: a payload byte equal to STX, ETX or the caller-chosen DLE travels preceded by DLE, and the byte after DLE is always data, so any byte value can cross the link. Every frame carries a two-byte check computed over the unescaped payload by a function the caller installs at `Init` — the signature is `crc16`'s, so `crc16` goes in directly, and the indirection is what lets the module keep its independence from `crc/`. `comstxetxBuildFrame` is the matching encoder, filling the transmit buffer the module owns, so the two halves cannot drift apart. A frame that fails its check is dropped and counted in `comstxetxGetRejectCount` rather than silently. `comat` has none of this on purpose: it speaks AT, an ASCII command protocol whose real peers do not checksum, and adding one would invent a dialect nothing else speaks.

There is **no build system** — no Makefile, no CMake. The library is consumed by copying/including the module source pairs into a target project. Nothing here produces an artifact by itself.

## Building and testing

Each `test/<Name>_Test/` directory is a standalone `main()` that exercises one module. Only `CircularBufferTest` keeps its Code::Blocks project file (`.cbp`) in git; the other `.cbp`/`.depend`/`.layout` files are `.gitignore`d, so build tests directly:

```bash
# One test = test main + the module .c, with the module's inc/ dir on the include path
gcc -Wall -g -Iinc/filter test/MAF_Test/MAF_Test.c src/filter/maf.c -o maf_test && ./maf_test
gcc -Wall -g -Iinc/complex test/Complex_Test/Complex_Test.c src/complex/complex.c -lm -o complex_test
```

Tests print to stdout and are verified by eye against the checked-in `output.txt` next to each test — there is no assertion framework and no runner. When changing a module with a test, regenerate `output.txt` and diff it.

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
sample/                                                standalone C examples, not part of the library
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

Stateful modules use their own name (`maf`, `emaf`, `median`, `biquad`, `slew`, `deadband`, `alphabeta`, `pid`, `circBuf`, `comat`, `comstxetx`, `bininp`, `hysteresis`, `complex`, `interp`, `hc595`, `hc597`, `dcMotor`). Check with `nm` after adding anything:

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

A parameter the function never writes is declared `const T* const`. This is not cosmetic: on an embedded target the caller's data is often in flash, and without it they must cast the qualifier away to call `crc16`, `mathFindMax` or `statVariance`. Accessors that only read take a `const` driver — except `bininpGetRisingValue`, which clears the flag it reports and so is genuinely `in,out`.

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

Every `.c` under `src/` and `drv/` compiles clean under `-Wall -Wextra` — **zero warnings, no exceptions** — and every `test/` program links. A new warning is a regression, not background noise. Check the whole tree with:

```bash
for f in src/*/*.c drv/*.c; do m=$(basename $(dirname "$f")); inc="inc/$m"; [ -d "$inc" ] || inc="drv"; \
  arm-none-eabi-gcc -c -Wall -Wextra -I"$inc" -Idrv "$f" -o /dev/null; done
```

Because every header must be independently includable, also check that they all coexist in one translation unit — this is what catches duplicate include guards and clashing typedefs:

```bash
for h in inc/*/*.h drv/*.h; do echo "#include \"$(basename $h)\""; done > /tmp/allhdr.c
echo "int main(void){return 0;}" >> /tmp/allhdr.c
arm-none-eabi-gcc -c -Wall $(for d in inc/*/ drv/; do echo -n " -I$d"; done) /tmp/allhdr.c -o /dev/null
```

## Known gaps — not bugs, just unwritten

These are stubs awaiting design, not defects. Leave them alone unless implementing the feature is the task.

- `src/communication/comsec.c` and `src/communication/comsafe.c` contain only a file banner. `inc/communication/comsec.h`, `comsafe.h`, `comgenbuf.h` and `inc/matrix/matrixlib.h` declare types but no function prototypes. Each of those four headers opens with a Doxygen `@warning` saying so — keep it there, it is the only thing standing between a consumer and a link error.
- `rules.md` is an empty placeholder.

## Testing

Twenty-two test programs cover every module that has functions, and **every one of the 198 exported symbols is referenced by at least one of them**. The only files with no test are `comsec`, `comsafe`, `comgenbuf` and `matrixlib`, which have nothing to test — see the known gaps above. Check that coverage claim still holds after adding an exported function:

```bash
cat test/*/*.c > /tmp/alltests.c
arm-none-eabi-nm /tmp/objs/*.o | grep ' T ' | awk '{print $3}' | sort -u | \
  while read s; do grep -q "\b$s\b" /tmp/alltests.c || echo "UNCALLED: $s"; done
```

**The assert style is the house style now.** Fifteen tests assert instead of printing values for a human to compare, so they have no `output.txt` and return non-zero on failure: `ShiftRegister_Test`, `Filter_Test`, `FilterSet_Test`, `SortSearch_Test`, `Math_Test`, `ArrayMatrix_Test`, `CRC_Test`, `Logic_Test`, `Protocol_Test`, `DcMotor_Test`, `Buffer_Test`, `ComplexMath_Test`, `Control_Test`, `SoftTimer_Test` and `Interp_Test`. Write new tests that way.

The seven older printing tests — `MAF_Test`, `EMAF_Test`, `Complex_Test`, `PID_Test`, `Hysteresis_Test`, `CircularBufferTest`, `WriteToAFile_Test` — predate that decision, and only five of them have an `output.txt` at all. Three of them are now shadowed rather than replaced: `Buffer_Test` covers what `CircularBufferTest` does not reach (the whole `u8` half, both overflow behaviours, the status reporting), `ComplexMath_Test` does the same for `Complex_Test`, and `Control_Test` for `PID_Test` and `Hysteresis_Test` (the four separate limiters, both `Change` functions, the argument checks). The printing originals are left alone; when one of these modules changes, the assert-style test is the one that has to keep passing. The first assert-style tests exist because a bug lived precisely where the printing tests did not look: `MAF_Test` and `EMAF_Test` only ever touched the float variants, and the `u32` ones were where the defects were.

Several tests aim a specific check at a specific fixed bug, so the regression fails rather than passing quietly. When touching one of these, that check is the one to keep:

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

**The suite was run for the first time on 05/08/2026** and all twenty-one programs build clean and pass, with no warnings from any test file. Until that day nothing here had ever been executed — the machine carried only `arm-none-eabi-gcc`, which cross-compiles but cannot run what it builds, so every check was compile-time and link-time. The expected values in the assert-style tests had been derived from independent models rather than from the C itself — an IEEE binary32 transliteration for the float ones, the CRC polynomials for `CRC_Test`, a state-machine replay for `Protocol_Test`, hand simulation for `SoftTimer_Test` — and the run confirmed every one of them.

Running them needs a host compiler, which the ARM toolchain is not. Each test is its own `main` plus the module sources its `#include "..."` lines name, so the dependency set is derivable and needs no list:

```bash
gcc -Wall -Wextra -Iinc/filter test/Filter_Test/Filter_Test.c src/filter/maf.c src/filter/emaf.c -o filter_test && ./filter_test
```

That run also found what only execution could:

- `WriteToAFile.c` called `exit` without `<stdlib.h>` and declared `void main`. A modern compiler makes the implicit declaration an error, not a warning, so the file did not build at all.
- All six printing tests ended `main` with an unconditional `return ( 1 );`, so their exit status said failure on every run. Now `0`.

The four `output.txt` files with comparable content were regenerated in the same pass, and the staleness they were assumed to carry was mostly not there: `MAF_Test` and `Hysteresis_Test` matched to the digit, `EMAF_Test` differed in one last place of one value, and only `PID_Test` had genuinely moved — the July 2026 initial-state fix, exactly where it was predicted. What the old files really differed by was whitespace: their tabs had been expanded to spaces at some point, and the programs emit real tabs.

Regenerate an `output.txt` by redirecting the program's stdout and stripping the carriage returns — the sources print `\r\n` and a Windows text-mode stream adds another `\r`, so a raw redirect stores `\r\r\n`.
