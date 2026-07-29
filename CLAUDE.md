# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this repo is

`esclib` is a freestanding general-purpose C library for embedded targets: filters, PID/hysteresis control, circular buffer, CRC, sort/search, matrix/complex math, serial protocol handlers, and shift-register drivers. No heap allocation, no OS dependency, C89-compatible style, `<stdint.h>` types throughout.

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

Modules are **fully independent**: every `.c` includes only its own header (plus `<math.h>` where needed). No module includes another module's header. Do not introduce cross-module includes — that independence is what makes single-module copy-out work.

## The driver-struct pattern

Every stateful module follows the same shape, and new modules must match it:

- One `typedef struct { ... } <prefix>_t;` holding all state (`maf_t`, `pidc_t`, `circBufu32_t`, `comat_t`, `bininp_t`).
- The **caller owns all storage**. The module never allocates; buffers are passed into `Init` as pointers (`mafInit(&f, buf, len, 0)`, `circBufInitu32(&b, buf, cap, BB_OVERWRITE)`). Never add `malloc`.
- First parameter of every function is `<prefix>_t* driver`.
- Function names are `<prefix>` + verb, always: `xxxInit`, then `xxxUpdate`/`xxxIteration`/`xxxControl`/`xxxReceive`, then `xxxGetValue`/`xxxGetOutput`.
- Type-suffixed names when a module is width-specific: `circBufAddu32`, `variancei32`.
- Hardware and I/O are injected as **function pointers stored in the struct at Init** — see `drv/hc595_drv.h` (`sckDrv`, `rckDrv`, `datDrv`, `dlyMs`, `dlyNop`) and `inc/communication/comat.h` (`packetProcess`, `txTransmissionTrigger`). Never call a HAL directly from library code.
- Protocol modules (`comat`, `comstxetx`) are byte-driven state machines: `xxxReceive(driver, byte)` from the ISR, `xxxEvaluate(driver)` from the main loop, `xxxTimeoutCounter(driver)` from a periodic tick.

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

Every `.c` under `src/` and `drv/` compiles clean under `-Wall -Wextra`, and every `test/` program links. Check the whole tree with:

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

- `drv/hc595_drv.c` and `drv/hc597_drv.c` — `hc595DrvLoop`, `hc595DrvInterrupt`, `hc597DrvLoop`, `hc597DrvInterrupt` are empty bodies. They are the only four `-Wunused-parameter` warnings in the tree; that warning is deliberate signal, do not silence it with a `(void)driver;` cast.
- `src/communication/comsec.c` and `src/communication/comsafe.c` contain only a file banner. `inc/communication/comsec.h`, `comsafe.h`, `comgenbuf.h` and `inc/matrix/matrixlib.h` declare types but no function prototypes.
- `rules.md` is an empty placeholder.

Test `output.txt` files predate the July 2026 bug fixes. Several fixes change numeric results (`findMini32`, `calculateMedian`, `complexDiv`, `complexToPolar`, PID initial state), so those files are stale until regenerated on a machine with a host compiler.
