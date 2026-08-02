# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this repo is

`esclib` is a freestanding general-purpose C library for embedded targets: filters, PID/hysteresis control, circular buffer, CRC, sort/search, matrix/complex math, serial protocol handlers, and shift-register drivers. No heap allocation, no OS dependency, C89-compatible style, `<stdint.h>` types throughout.

The `filter/` group is the largest and each module there answers a different problem, so pick by what is wrong with the signal rather than by habit: `maf`/`emaf` smooth, `median` rejects impulses outright, `biquad` shapes a response in hertz (and is the only way to notch mains hum), `slew` bounds the rate of change, `deadband` holds the output still until the input really moves, and `alphabeta` estimates position *and* velocity. `emaf` also carries an integer-only variant, `emafIniti32`/`emafIterationi32`, for parts with no FPU — it is a width variant of `emaf` rather than a module of its own, so the float code shares the file with it.

`sort` and `search` pair up and the choice within each is the same kind of decision. Among the sorts, `sortInsertion` wins on short or nearly-sorted arrays and is stable, `sortHeap` is the only one with an O(N log N) *guarantee* and stays in place without recursing, and `sortSelection`/`sortBubble` are there for teaching rather than for speed. Every sort produces ascending order; `sortReverse` turns that into descending in one pass, which is why there is no descending variant of each. Among the searches, `searchBinary` answers whether a value is present, `searchLowerBound`/`searchUpperBound` answer where it belongs — the insertion point, and their difference is the number of duplicates — and `searchClosest` answers which entry to read, which is what a calibration or linearisation table actually needs. All of the binary ones require ascending order and give a confident wrong answer without it, so `sortIsSorted` exists to check that precondition cheaply.

There is **no build system** — no Makefile, no CMake. The library is consumed by copying/including the module source pairs into a target project. Nothing here produces an artifact by itself.

## Building and testing

Each `test/<Name>_Test/` directory is a standalone `main()` that exercises one module. Only `CircularBufferTest` keeps its Code::Blocks project file (`.cbp`) in git; the other `.cbp`/`.depend`/`.layout` files are `.gitignore`d, so build tests directly:

```bash
# One test = test main + the module .c, with the module's inc/ dir on the include path
gcc -Wall -g -Iinc/filter test/MAF_Test/MAF_Test.c src/filter/maf.c -o maf_test && ./maf_test
gcc -Wall -g -Iinc/math   test/Complex_Test/Complex_Test.c src/complex/complex.c -lm -o complex_test
```

Tests print to stdout and are verified by eye against the checked-in `output.txt` next to each test — there is no assertion framework and no runner. When changing a module with a test, regenerate `output.txt` and diff it.

`gcc` is not on PATH in this environment; `arm-none-eabi-gcc` is. For syntax/warning checking use:

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

Stateful modules use their own name (`maf`, `emaf`, `median`, `biquad`, `slew`, `deadband`, `alphabeta`, `pid`, `circBuf`, `comat`, `comstxetx`, `bininp`, `hysteresis`, `complex`, `hc595`, `hc597`, `dcMotor`). Check with `nm` after adding anything:

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

Twenty test programs cover every module that has functions, and **every one of the 181 exported symbols is referenced by at least one of them**. The only files with no test are `comsec`, `comsafe`, `comgenbuf` and `matrixlib`, which have nothing to test — see the known gaps above. Check that coverage claim still holds after adding an exported function:

```bash
cat test/*/*.c > /tmp/alltests.c
arm-none-eabi-nm /tmp/objs/*.o | grep ' T ' | awk '{print $3}' | sort -u | \
  while read s; do grep -q "\b$s\b" /tmp/alltests.c || echo "UNCALLED: $s"; done
```

**The assert style is the house style now.** Thirteen tests assert instead of printing values for a human to compare, so they have no `output.txt` and return non-zero on failure: `ShiftRegister_Test`, `Filter_Test`, `FilterSet_Test`, `SortSearch_Test`, `Math_Test`, `ArrayMatrix_Test`, `CRC_Test`, `Logic_Test`, `Protocol_Test`, `DcMotor_Test`, `Buffer_Test`, `ComplexMath_Test` and `Control_Test`. Write new tests that way.

The seven older printing tests — `MAF_Test`, `EMAF_Test`, `Complex_Test`, `PID_Test`, `Hysteresis_Test`, `CircularBufferTest`, `WriteToAFile_Test` — predate that decision, and only five of them have an `output.txt` at all. Three of them are now shadowed rather than replaced: `Buffer_Test` covers what `CircularBufferTest` does not reach (the whole `u8` half, both overflow behaviours, the status reporting), `ComplexMath_Test` does the same for `Complex_Test`, and `Control_Test` for `PID_Test` and `Hysteresis_Test` (the four separate limiters, both `Change` functions, the argument checks). The printing originals are left alone; when one of these modules changes, the assert-style test is the one that has to keep passing. The first assert-style tests exist because a bug lived precisely where the printing tests did not look: `MAF_Test` and `EMAF_Test` only ever touched the float variants, and the `u32` ones were where the defects were.

Several tests aim a specific check at a specific fixed bug, so the regression fails rather than passing quietly. When touching one of these, that check is the one to keep:

| test | bug it pins |
|---|---|
| `SortSearch_Test` | the stray semicolon that made `searchLinear` match at index 0 for anything; the `length - 1` underflow in `sortSelection` and in the binary searches |
| `Math_Test` | `mathFindMini32` returning the maximum; `mathCalculateMedian` not averaging the two middle elements |
| `Protocol_Test` | `rxTimeoutCounter` running on across frames. Note that the tick-driven discard cleared the counter even before the fix, so only a sequence that **completes** a frame late in its budget and then asks the next one for a full budget discriminates. Same for the buffer-overflow reset path. |
| `Filter_Test` | the `emafu32` dead band and the `emafGetOutputu32` range overflow |
| `ShiftRegister_Test` | the two transfer modes colliding, and the delay-to-step relation between them |
| `ComplexMath_Test` | the `complexDiv` sign, checked both against the answer and by multiplying the quotient back |
| `Control_Test` | `pidInit` leaving `lastError` and `partI` unset, and `ts == 0` reaching the derivative divide |

Nothing in this repo has ever been *executed* here: there is no host compiler on this machine, only `arm-none-eabi-gcc`, which cross-compiles but cannot run what it builds. Every verification below is compile-time and link-time only. Treat any claim about numeric results as unverified until it is run. Expected values in the assert-style tests were derived from independent models — an IEEE binary32 transliteration for the float ones, the CRC polynomials for `CRC_Test`, a state-machine replay for `Protocol_Test` — rather than from the C itself, which is the only thing that makes them worth anything without a run.

Test `output.txt` files predate the July 2026 bug fixes and are stale until regenerated on a machine with a host compiler. What moved the numbers:

- July 2026 bug fixes — `mathFindMini32`, `mathCalculateMedian`, `complexDiv`, `complexToPolar`, PID initial state.
- August 2026 — the switch from `sqrt`/`atan2`/`cos`/`sin` to the `f` variants. Single precision changes the trailing digits of `complexToPolar`, `complexFromPolar` and `statStandardDeviation`, so `Complex_Test` output will differ even where the fix itself was behaviour-neutral.

Do not treat a diff against a checked-in `output.txt` as a regression until the file has been regenerated once against current source.
