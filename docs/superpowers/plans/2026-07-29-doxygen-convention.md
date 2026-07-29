# Doxygen Comment Convention Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Convert every function and file comment in `src/` and `drv/` to a uniform, Doxygen-readable format without changing a single line of code.

**Architecture:** Documentation lives in `.c` files only; headers stay pure declarations. Each task takes one module group, rewrites its comments, then proves nothing executable changed by recompiling and comparing object files byte for byte against a baseline captured in Task 1.

**Tech Stack:** C99, `arm-none-eabi-gcc` 13.2 (the only compiler on this machine), Doxygen (not installed here — the Doxyfile ships untested).

Spec: `docs/superpowers/specs/2026-07-29-doxygen-convention-design.md`

## Global Constraints

- Documentation goes in `.c` files only. Files under `inc/` and `drv/*.h` are **not** modified, with one exception: nothing. Headers are untouched.
- Comment blocks open with `/**`. A block opening with `/*` is invisible to Doxygen and is the main defect being fixed.
- Tag set is exactly: `@brief`, `@param[in]`, `@param[out]`, `@param[in,out]`, `@return`, `@note`. No other function-level tags.
- `@brief` is one sentence ending in a period.
- Every parameter gets a `@param` with an explicit direction. No bare `@param`.
- `@return` appears on non-void functions only, and on every one of them.
- `@note` appears only when it carries real information. Never write an empty or filler note.
- `static` helpers are documented identically to public functions.
- All prose is English (repository rule, see `CLAUDE.md`).
- Existing code style is untouched: spaces inside parens, Allman braces, `retVal` single-exit. This plan changes comments only.
- Commit messages carry no `Co-Authored-By` trailer or AI attribution (`CLAUDE.md` rule).
- Commit subject prefix follows repo habit: `*` for changes to existing files, `+` for additions.

## Reference: the two formats

Every task applies these. They are repeated here so no task has to look elsewhere.

**Function comment:**

```c
/**
 * @brief   Adds one word to the circular buffer.
 * @param[in,out] driver  Buffer state.
 * @param[in]     data    Word to store.
 * @return  TRUE when the word was stored, FALSE when the buffer is
 *          full and the behaviour is BB_STOP.
 */
uint8_t circBufAddu32 ( circBufu32_t* driver, uint32_t data )
```

**Direction of the `driver` parameter**, decided by what the function does to the state:

| Family | Direction |
|---|---|
| `xxxInit` | `[out]` |
| `xxxUpdate`, `xxxIteration`, `xxxControl`, `xxxReceive`, `xxxEvaluate`, `xxxAdd`, `xxxRead`, `xxxTimeoutCounter`, `xxxChange*` | `[in,out]` |
| `xxxGetValue`, `xxxGetOutput`, `xxxGetLength`, `xxxGetStatus` | `[in]` |

One exception in the whole library: `bininpGetRisingValue` reads like a getter but clears the `rising` flag, so it takes `[in,out]`.

**File banner:**

```c
/**
  ******************************************************************************
  *
  * @file      maf.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   2.1.0
  * @date      26/04/2020
  *
  * @brief     Moving average filter.
  *
  * @par Device
  * Generic
  *
  * @par History
  * 26/04/2020 Created @n
  * 07/06/2020 Naming style changed @n
  * 24/08/2020 Data type changed from double to float @n
  *
  ******************************************************************************
  */
```

Banner conversion rules: `@file:` → `@file` (drop the colon), `@email:`/`@address:` fold into the `@author` line, `@version: v 2.1.0` → `@version 2.1.0` (drop the `v`), `@cdate:` → `@date`, `@about:` → `@brief`, `@device:` → `@par Device`, `@history:` → `@par History` with `@n` ending each entry, `@notes:` → `@note` (omit the tag entirely when there are no notes), `@content:` → **deleted**.

---

### Task 1: Baseline, Doxyfile, and templates

Establishes the verification baseline and the canonical format that every later task copies.

**Files:**
- Create: `Doxyfile`
- Modify: `.gitignore` (append one line)
- Modify: `template/src/generic.c`
- Inspect only, no change expected: `template/inc/generic.h`

**Interfaces:**
- Produces: the baseline object hashes at `/tmp/esclib-baseline/` that Tasks 2-9 compare against, and the reference banner/function format in `template/`.

- [ ] **Step 1: Capture the object hash baseline before touching anything**

```bash
cd "C:/Users/engin/Documents/GitHub/esclib"
rm -rf /tmp/esclib-baseline && mkdir -p /tmp/esclib-baseline
for f in src/*/*.c drv/*.c; do
  m=$(basename $(dirname "$f")); inc="inc/$m"; [ -d "$inc" ] || inc="drv"
  n=$(echo "$f" | tr '/' '_')
  arm-none-eabi-gcc -c -O2 -mcpu=cortex-m3 -mthumb -I"$inc" -Idrv "$f" -o "/tmp/esclib-baseline/$n.o" 2>/dev/null
done
sha256sum /tmp/esclib-baseline/*.o > /tmp/esclib-baseline/HASHES
wc -l < /tmp/esclib-baseline/HASHES
```

Expected: `24`. All 24 `.c` files produce an object, including the empty `src/communication/comsec.c` — an empty translation unit is legal C and compiles to an object with no symbols.

Note the flags: `-O2` and **no** `-g`. Debug info would embed line numbers and make comment edits shift the hash. This exact command was validated — a comment-only edit leaves the object byte identical.

- [ ] **Step 2: Create the Doxyfile**

```
PROJECT_NAME          = esclib
PROJECT_BRIEF         = General purpose C library for embedded targets
OUTPUT_DIRECTORY      = doc
INPUT                 = inc src drv
RECURSIVE             = YES
OPTIMIZE_OUTPUT_FOR_C = YES
EXTRACT_STATIC        = YES
EXTRACT_ALL           = NO
WARN_NO_PARAMDOC      = YES
WARN_AS_ERROR         = NO
GENERATE_LATEX        = NO
GENERATE_HTML         = YES
JAVADOC_AUTOBRIEF     = NO
```

`EXTRACT_ALL = NO` matters: it means an undocumented function is simply absent from the output rather than silently included empty, so gaps are visible. `WARN_NO_PARAMDOC = YES` makes Doxygen report any parameter left undocumented.

- [ ] **Step 3: Ignore the generated output directory**

Append to `.gitignore`:

```
# Doxygen generated output
doc/
```

- [ ] **Step 4: Update `template/inc/generic.h`**

Headers are not documented, and this one has no banner and no functions — it holds only include guards, the `TRUE`/`FALSE` defines, and empty section markers. **No change is needed.** Confirm before moving on:

```bash
grep -c '@' template/inc/generic.h
```

Expected: `0`. A non-zero count means the file gained tags since this plan was written; re-read it and decide.

- [ ] **Step 5: Rewrite `template/src/generic.c` to the new format**

```c
/**
  ******************************************************************************
  *
  * @file      generic.c
  * @author    Name Surname <mail@mail.com>, github.com/enginsubasi
  * @version   0.0.1
  * @date      20/02/2020
  *
  * @brief     Generic template file.
  *
  * @par Device
  * Generic
  *
  * @par History
  * 20/02/2020 Created @n
  *
  ******************************************************************************
  */

#include "generic.h"

/**
 * @brief   One sentence describing what the function does.
 * @param[in,out] driver  Module state. Use [out] in an Init function and
 *                        [in] in a getter that only reads.
 * @param[in]     value   An input parameter.
 * @return  What the caller gets back. Omit this tag for a void function.
 * @note    Only when there is something non obvious to say. Omit otherwise.
 */
```

- [ ] **Step 6: Confirm nothing compiled changed**

```bash
cd "C:/Users/engin/Documents/GitHub/esclib"
for f in src/*/*.c drv/*.c; do
  m=$(basename $(dirname "$f")); inc="inc/$m"; [ -d "$inc" ] || inc="drv"
  n=$(echo "$f" | tr '/' '_')
  arm-none-eabi-gcc -c -O2 -mcpu=cortex-m3 -mthumb -I"$inc" -Idrv "$f" -o "/tmp/esclib-now-$n.o" 2>/dev/null
  cmp -s "/tmp/esclib-baseline/$n.o" "/tmp/esclib-now-$n.o" || echo "CHANGED: $f"
done
echo "hash check done"
```

Expected: no `CHANGED:` lines. Task 1 touches no `.c` under `src/` or `drv/`, so this is a sanity check that the comparison harness itself works.

- [ ] **Step 7: Commit**

```bash
git add Doxyfile .gitignore template/src/generic.c
git commit -m "+ Doxyfile and template updated to the Doxygen comment format"
```

---

### Task 2: filter and control modules

**Files:**
- Modify: `src/filter/maf.c` (6 functions), `src/filter/emaf.c` (3), `src/control/pid.c` (5), `src/control/hysteresis.c` (3)

**Interfaces:**
- Consumes: the baseline hashes and the format from Task 1.

- [ ] **Step 1: Convert the four file banners**

Apply the banner rules from the Reference section. Preserve every `@history` entry verbatim, including the July 2026 bug-fix entries added in commit `1861aa7`. Drop the `@content` block.

- [ ] **Step 2: Convert the function comments in `src/filter/maf.c`**

Worked example for the first function; apply the same shape to the other five (`mafIteration`, `mafGetOutput`, `mafInitu32`, `mafIterationu32`, `mafGetOutputu32`).

```c
/**
 * @brief   Initializes the moving average filter.
 * @param[out] driver      Filter state to initialize.
 * @param[in]  buffer      Caller owned sample buffer of at least length entries.
 * @param[in]  length      Number of samples in the averaging window.
 * @param[in]  outputInit  Value the whole window is preloaded with.
 * @return  TRUE on success, FALSE when length is zero or buffer is NULL.
 * @note    The buffer is not copied. It must outlive the filter.
 */
int8_t mafInit ( maf_t* driver, float* buffer, uint32_t length, float outputInit )
```

`mafGetOutput` and `mafGetOutputu32` take `[in]` on `driver`. `mafIteration` and `mafIterationu32` take `[in,out]`.

- [ ] **Step 3: Convert `src/filter/emaf.c`, `src/control/pid.c`, `src/control/hysteresis.c`**

`pidInit` takes 13 parameters — every one gets a `@param[in]` line, `driver` gets `[out]`. `pidChangeCoefficients` and `pidChangeLimits` take `[in,out]` on `driver`. `pidControl` takes `[in,out]`; note that its second parameter is named `error`, not `input`, and the brief must say the caller passes the error signal, not the raw measurement. `pidGetOutput` and `hysteresisGetOutput` take `[in]`.

- [ ] **Step 4: Verify the objects are unchanged**

```bash
cd "C:/Users/engin/Documents/GitHub/esclib"
for f in src/filter/maf.c src/filter/emaf.c src/control/pid.c src/control/hysteresis.c; do
  m=$(basename $(dirname "$f")); n=$(echo "$f" | tr '/' '_')
  arm-none-eabi-gcc -c -O2 -mcpu=cortex-m3 -mthumb -I"inc/$m" "$f" -o "/tmp/chk.o"
  cmp -s "/tmp/esclib-baseline/$n.o" /tmp/chk.o && echo "OK   $f" || echo "CHANGED $f"
done
```

Expected: four `OK` lines. A `CHANGED` line means code was edited by accident — revert that file and redo it as comments only.

- [ ] **Step 5: Commit**

```bash
git add src/filter src/control
git commit -m "* Doxygen comments for the filter and control modules"
```

---

### Task 3: buffer, bininp, and logic modules

**Files:**
- Modify: `src/buffer/circBuf.c` (5 functions), `src/bininp/bininp.c` (4), `src/logic/logic.c` (2)

- [ ] **Step 1: Convert the three file banners**

Same rules as Task 2. Preserve all `@history` entries.

- [ ] **Step 2: Convert `src/buffer/circBuf.c`**

```c
/**
 * @brief   Initializes the circular buffer.
 * @param[out] driver     Buffer state to initialize.
 * @param[in]  buffer     Caller owned storage of at least capacity words.
 * @param[in]  capacity   Number of words the buffer holds.
 * @param[in]  behaviour  BB_OVERWRITE to drop the oldest word when full,
 *                        BB_STOP to reject the new word instead.
 * @return  TRUE on success, FALSE when a pointer is NULL or capacity is zero.
 */
uint8_t circBufInitu32 ( circBufu32_t* driver, uint32_t* buffer, uint32_t capacity, uint8_t behaviour )
```

`circBufGetLengthu32` and `circBufGetStatusu32` take `[in]`. `circBufAddu32` and `circBufReadu32` take `[in,out]`; `circBufReadu32`'s `data` parameter is `[out]` and its brief must state that it is set to zero when the buffer is empty.

- [ ] **Step 3: Convert `src/bininp/bininp.c`**

`bininpInit` takes `[out]`. `bininpUpdate` takes `[in,out]`. `bininpGetValue` takes `[in]`. `bininpGetRisingValue` takes **`[in,out]`** — it is the library's one getter that mutates state, and its `@note` must say it clears the flag so a second call returns FALSE.

- [ ] **Step 4: Convert `src/logic/logic.c`**

`rsff` and `dff` take no driver struct. Their `mem` parameter is `[in,out]` — it is the flip-flop's stored state, read and then written.

- [ ] **Step 5: Verify the objects are unchanged**

```bash
cd "C:/Users/engin/Documents/GitHub/esclib"
for f in src/buffer/circBuf.c src/bininp/bininp.c src/logic/logic.c; do
  m=$(basename $(dirname "$f")); n=$(echo "$f" | tr '/' '_')
  arm-none-eabi-gcc -c -O2 -mcpu=cortex-m3 -mthumb -I"inc/$m" "$f" -o "/tmp/chk.o"
  cmp -s "/tmp/esclib-baseline/$n.o" /tmp/chk.o && echo "OK   $f" || echo "CHANGED $f"
done
```

Expected: three `OK` lines.

- [ ] **Step 6: Commit**

```bash
git add src/buffer src/bininp src/logic
git commit -m "* Doxygen comments for the buffer, bininp and logic modules"
```

---

### Task 4: math modules

The largest task by function count: 23 functions.

**Files:**
- Modify: `src/math/basicmath.c` (18 functions), `src/math/statistic.c` (5)

- [ ] **Step 1: Convert both file banners**

- [ ] **Step 2: Convert `src/math/basicmath.c`**

```c
/**
 * @brief   Finds the largest element of the array.
 * @param[in] array   Array to scan.
 * @param[in] length  Number of elements in the array.
 * @return  The largest element, or zero when length is zero.
 */
float findMax ( float* array, uint32_t length )
```

Points that must be stated rather than left implicit:

- Every `find*` and `calculate*` function returns zero for a zero length array. Say so in the `@return`.
- `findMinMax` and `findMinMaxu32` are void; their `min` and `max` parameters are `[out]`.
- `calculateMedian` and `calculateMedianu32` require a **sorted** array. That belongs in a `@note`, because passing an unsorted array returns a wrong answer silently.
- `calculateMedianu32` computes the even-length midpoint as `low + ( ( high - low ) / 2 )` to avoid overflow. That is worth a `@note`.

- [ ] **Step 3: Convert `src/math/statistic.c`**

All five functions take `[in]` on their arrays. Each returns zero for a zero length array — state it in `@return`. `standardDeviation` and `standardDeviationi32` depend on `math.h`; the existing inline `// Dep. math.h` comment stays where it is.

- [ ] **Step 4: Verify the objects are unchanged**

```bash
cd "C:/Users/engin/Documents/GitHub/esclib"
for f in src/math/basicmath.c src/math/statistic.c; do
  n=$(echo "$f" | tr '/' '_')
  arm-none-eabi-gcc -c -O2 -mcpu=cortex-m3 -mthumb -Iinc/math "$f" -o "/tmp/chk.o"
  cmp -s "/tmp/esclib-baseline/$n.o" /tmp/chk.o && echo "OK   $f" || echo "CHANGED $f"
done
```

Expected: two `OK` lines.

- [ ] **Step 5: Commit**

```bash
git add src/math
git commit -m "* Doxygen comments for the math modules"
```

---

### Task 5: array and matrix modules

**Files:**
- Modify: `src/array/basicarray.c` (3 functions), `src/matrix/basicmatrix.c` (6), `src/matrix/matrixlib.c` (banner only, no functions)

- [ ] **Step 1: Convert the three file banners**

`src/matrix/matrixlib.c` has a banner but no functions. Convert its banner and leave the rest of the file alone. Its `@brief` stays "Matrix function library file based on matrix structure."

- [ ] **Step 2: Convert `src/array/basicarray.c`**

All three `limitUpDw1D*` functions take `[in,out]` on `array` — they clamp in place. This is the detail most likely to be documented wrongly as `[in]`.

- [ ] **Step 3: Convert `src/matrix/basicmatrix.c`**

```c
/**
 * @brief   Replaces every element of a 2D matrix with upValue or dwValue.
 * @param[in,out] matrix          Row major matrix, modified in place.
 * @param[in]     thresholdValue  Elements above this become upValue.
 * @param[in]     upValue         Value written above the threshold.
 * @param[in]     dwValue         Value written at or below the threshold.
 * @param[in]     iSize           Row count.
 * @param[in]     jSize           Column count, which is also the row stride.
 * @return  void
 */
void threshold2D ( float* matrix, float thresholdValue, float upValue, float dwValue, uint32_t iSize, uint32_t jSize )
```

Drop the `@return void` line — void functions get no `@return` at all. The example above shows it only to be explicit that it must not appear.

The row major layout and the `iSize` / `jSize` meaning must appear in the parameter descriptions of all five 2D functions. This was the source of a real bug fixed in `1861aa7`, so the documentation is what stops it recurring.

`limitUpDw2D` takes five parameters, not six — it has no `thresholdValue`.

- [ ] **Step 4: Verify the objects are unchanged**

```bash
cd "C:/Users/engin/Documents/GitHub/esclib"
for f in src/array/basicarray.c src/matrix/basicmatrix.c src/matrix/matrixlib.c; do
  m=$(basename $(dirname "$f")); n=$(echo "$f" | tr '/' '_')
  arm-none-eabi-gcc -c -O2 -mcpu=cortex-m3 -mthumb -I"inc/$m" "$f" -o "/tmp/chk.o"
  cmp -s "/tmp/esclib-baseline/$n.o" /tmp/chk.o && echo "OK   $f" || echo "CHANGED $f"
done
```

Expected: three `OK` lines.

- [ ] **Step 5: Commit**

```bash
git add src/array src/matrix
git commit -m "* Doxygen comments for the array and matrix modules"
```

---

### Task 6: search and sort modules

**Files:**
- Modify: `src/search/search.c` (7 functions, 1 static), `src/sort/sort.c` (12 functions, 3 static)

- [ ] **Step 1: Convert both file banners**

- [ ] **Step 2: Convert `src/search/search.c`**

```c
/**
 * @brief   Searches the array element by element for a matching item.
 * @param[in]  array       Array to search.
 * @param[in]  length      Number of elements in the array.
 * @param[in]  item        Value to look for.
 * @param[out] foundIndex  Index of the first match. Untouched when no match.
 * @param[in]  epsilon     Largest difference still counted as equal.
 * @return  TRUE when a match was found, FALSE otherwise.
 */
uint8_t linearSearch ( const float* const array, uint32_t length, float item, uint32_t* const foundIndex, float epsilon )
```

The static helper `isEqualf` gets the same treatment. The three `binarySearch*` functions need a `@note` stating the array must be sorted ascending — an unsorted array yields a wrong answer, not an error. All six search functions return FALSE for a zero length array; say so in `@return`.

- [ ] **Step 3: Convert `src/sort/sort.c`**

The three static `swapForSort*` helpers take `[in,out]` on both pointers. All nine public sort functions take `[in,out]` on `array` and are void, so they get no `@return`. Each needs a `@note` that an array of zero or one element is left untouched.

- [ ] **Step 4: Verify the objects are unchanged**

```bash
cd "C:/Users/engin/Documents/GitHub/esclib"
for f in src/search/search.c src/sort/sort.c; do
  m=$(basename $(dirname "$f")); n=$(echo "$f" | tr '/' '_')
  arm-none-eabi-gcc -c -O2 -mcpu=cortex-m3 -mthumb -I"inc/$m" "$f" -o "/tmp/chk.o"
  cmp -s "/tmp/esclib-baseline/$n.o" /tmp/chk.o && echo "OK   $f" || echo "CHANGED $f"
done
```

Expected: two `OK` lines.

- [ ] **Step 5: Commit**

```bash
git add src/search src/sort
git commit -m "* Doxygen comments for the search and sort modules"
```

---

### Task 7: crc and complex modules

**Files:**
- Modify: `src/crc/crc16.c` (2 functions), `src/crc/crc32.c` (1), `src/complex/complex.c` (7)

- [ ] **Step 1: Convert the three file banners**

- [ ] **Step 2: Convert the crc modules**

```c
/**
 * @brief   Calculates the MODBUS CRC16 of a byte array using a lookup table.
 * @param[in] array  Bytes to run the CRC over.
 * @param[in] size   Number of bytes.
 * @return  The CRC16 value, seeded with 0xFFFF.
 * @note    Trades 512 bytes of table space for speed. Use crc16Alt when
 *          code size matters more than throughput.
 */
uint16_t crc16 ( uint8_t* array, uint32_t size )
```

`crc32` implements the CRC-32/MPEG-2 variant: initial value `0xFFFFFFFF`, no input or output reflection, no final XOR. State that in a `@note`, because callers comparing against a generic "CRC32" will otherwise get a mismatch and assume a bug.

- [ ] **Step 3: Convert `src/complex/complex.c`**

`complexInit` takes `[out]` on `cprm1`. The four arithmetic functions take `[in]` on `cprm1` and `cprm2` and `[out]` on `result`. `complexDiv` needs a `@note` that a zero divisor yields a zero result rather than an error. `complexToPolar` takes `[in]` on `prm1` and `[out]` on `r` and `a`, and its `@note` states the angle is in degrees. `complexFromPolar` takes `[out]` on `prm1` and `[in]` on `r` and `a`, also degrees.

- [ ] **Step 4: Verify the objects are unchanged**

```bash
cd "C:/Users/engin/Documents/GitHub/esclib"
for f in src/crc/crc16.c src/crc/crc32.c src/complex/complex.c; do
  m=$(basename $(dirname "$f")); n=$(echo "$f" | tr '/' '_')
  arm-none-eabi-gcc -c -O2 -mcpu=cortex-m3 -mthumb -I"inc/$m" "$f" -o "/tmp/chk.o"
  cmp -s "/tmp/esclib-baseline/$n.o" /tmp/chk.o && echo "OK   $f" || echo "CHANGED $f"
done
```

Expected: three `OK` lines.

- [ ] **Step 5: Commit**

```bash
git add src/crc src/complex
git commit -m "* Doxygen comments for the crc and complex modules"
```

---

### Task 8: communication modules

**Files:**
- Modify: `src/communication/comat.c` (4 functions), `src/communication/comstxetx.c` (4), `src/communication/comsafe.c` (banner only), `src/communication/comsec.c` (empty, see Step 4)

- [ ] **Step 1: Convert the banners of `comat.c`, `comstxetx.c`, `comsafe.c`**

Preserve every `@history` entry, including the four bug-fix lines added to `comat.c` and the three added to `comstxetx.c` in commit `1861aa7`.

- [ ] **Step 2: Convert `src/communication/comat.c`**

```c
/**
 * @brief   Initializes the AT command framework.
 * @param[out] driver                 Framework state to initialize.
 * @param[in]  rxBuffer               Caller owned receive buffer.
 * @param[in]  txBuffer               Caller owned transmit buffer.
 * @param[in]  rxSize                 Size of rxBuffer in bytes.
 * @param[in]  txSize                 Size of txBuffer in bytes.
 * @param[in]  rxTimeout              Ticks of silence before a partial frame
 *                                    is discarded.
 * @param[in]  packetProcess          Called with a complete frame. Receives the
 *                                    byte count by value and writes the reply
 *                                    length through txInd.
 * @param[in]  txTransmissionTrigger  Called to start transmitting the reply.
 * @note    Both buffers are zero filled here and are not copied. They must
 *          outlive the driver.
 */
```

The three remaining functions are the byte-driven state machine and their briefs must say where each one is called from: `comatReceive` from the receive interrupt, `comatEvaluate` from the main loop, `comatTimeoutCounter` from a periodic tick. All three take `[in,out]` on `driver`.

- [ ] **Step 3: Convert `src/communication/comstxetx.c`**

Same call-site pattern as `comat.c`. `comstxetxInit` takes `[out]`, the other three take `[in,out]`. `comstxetxReceive`'s brief states that a frame starts at the STX byte and completes at the ETX byte.

- [ ] **Step 4: Handle the two files with no functions**

`comsafe.c` holds only a banner — convert it and stop. Its `@brief` stays "Safe communication framework."

`comsec.c` is a zero byte file. Leave it empty. Do not add a banner to it; an empty file is a clearer signal that the module is unwritten than a banner promising a module that does not exist.

- [ ] **Step 5: Verify the objects are unchanged**

```bash
cd "C:/Users/engin/Documents/GitHub/esclib"
for f in src/communication/comat.c src/communication/comstxetx.c src/communication/comsafe.c; do
  n=$(echo "$f" | tr '/' '_')
  arm-none-eabi-gcc -c -O2 -mcpu=cortex-m3 -mthumb -Iinc/communication "$f" -o "/tmp/chk.o"
  cmp -s "/tmp/esclib-baseline/$n.o" /tmp/chk.o && echo "OK   $f" || echo "CHANGED $f"
done
```

Expected: three `OK` lines.

- [ ] **Step 6: Commit**

```bash
git add src/communication
git commit -m "* Doxygen comments for the communication modules"
```

---

### Task 9: driver modules

**Files:**
- Modify: `drv/dcmotor.c` (2 functions), `drv/hc595_drv.c` (5), `drv/hc597_drv.c` (5)

- [ ] **Step 1: Convert the three file banners**

`dcmotor.c`'s banner was corrected in `1861aa7` and now names the right file; keep that. Preserve all `@history` entries.

- [ ] **Step 2: Convert `drv/hc595_drv.c`**

```c
/**
 * @brief   Initializes the HC595 shift register driver and idles its pins.
 * @param[out] driver     Driver state to initialize.
 * @param[in]  dataPtr    Caller owned array shifted out on each transfer.
 * @param[in]  dataSize   Number of bytes in dataPtr.
 * @param[in]  dlyType    HC595_DLY_NO, HC595_DLY_MS or HC595_DLY_NOP.
 * @param[in]  dlyCount   Delay length, in the unit chosen by dlyType.
 * @param[in]  sckDrvFnc  Drives the shift clock pin.
 * @param[in]  rckDrvFnc  Drives the latch clock pin.
 * @param[in]  datDrvFnc  Drives the serial data pin.
 * @param[in]  dlyMsFnc   Blocks for the given number of milliseconds.
 * @param[in]  dlyNopFnc  Spins for the given number of no-op cycles.
 * @note    Drives all three output pins low before returning, so the hardware
 *          is in a known state.
 */
```

`hc595DrvOneShoot` takes `[in,out]` and its brief states that it shifts the whole array out, least significant bit first, then pulses the latch. The static `hc595DlyCtrl` takes `[in,out]` because it repairs an invalid `dlyType` in place — that is a real side effect and belongs in a `@note`.

- [ ] **Step 3: Convert `drv/hc597_drv.c`**

Mirrors `hc595_drv.c` but reads instead of writes: `datDrvFnc` returns the pin level rather than setting it, so `hc597DrvOneShoot` takes `[in,out]` on `driver` and fills `driver->data`. Its brief states it latches the parallel inputs once and then clocks `driver->size` bytes in.

- [ ] **Step 4: Document the four empty stubs honestly**

`hc595DrvLoop`, `hc595DrvInterrupt`, `hc597DrvLoop`, `hc597DrvInterrupt` have empty bodies. Give each one a `@brief` and a `@note` saying it is not implemented yet. Do not invent parameter descriptions for behaviour that does not exist — a single `@param[in,out] driver  Driver state.` line plus the note is the whole comment.

```c
/**
 * @brief   Not implemented.
 * @param[in,out] driver  Driver state.
 * @note    Reserved for a non blocking transfer driven from the main loop.
 *          The body is empty, which is why the compiler reports driver as
 *          an unused parameter.
 */
void hc595DrvLoop ( struct HC595_Driver* driver )
```

- [ ] **Step 5: Convert `drv/dcmotor.c`**

`dcMotorInit` takes `[out]` on `driver`, and its `@note` states that it sets PWM to zero and the bridge to `BRIDGE_NO` before returning. `dcMotorBridgeState` takes `[in,out]` and its `@param[in] bridgeState` lists the four `BRIDGE_*` values, noting that an unknown value falls back to `BRIDGE_NO`.

- [ ] **Step 6: Verify the objects are unchanged**

```bash
cd "C:/Users/engin/Documents/GitHub/esclib"
for f in drv/dcmotor.c drv/hc595_drv.c drv/hc597_drv.c; do
  n=$(echo "$f" | tr '/' '_')
  arm-none-eabi-gcc -c -O2 -mcpu=cortex-m3 -mthumb -Idrv "$f" -o "/tmp/chk.o"
  cmp -s "/tmp/esclib-baseline/$n.o" /tmp/chk.o && echo "OK   $f" || echo "CHANGED $f"
done
```

Expected: three `OK` lines.

- [ ] **Step 7: Commit**

```bash
git add drv
git commit -m "* Doxygen comments for the driver modules"
```

---

### Task 10: Final sweep and convention documentation

**Files:**
- Modify: `codingReference.md` (append the convention)
- Modify: `CLAUDE.md` (point at the convention)

- [ ] **Step 1: Verify every object across the whole tree is unchanged**

```bash
cd "C:/Users/engin/Documents/GitHub/esclib"
fail=0
for f in src/*/*.c drv/*.c; do
  m=$(basename $(dirname "$f")); inc="inc/$m"; [ -d "$inc" ] || inc="drv"
  n=$(echo "$f" | tr '/' '_')
  [ -f "/tmp/esclib-baseline/$n.o" ] || continue
  arm-none-eabi-gcc -c -O2 -mcpu=cortex-m3 -mthumb -I"$inc" -Idrv "$f" -o /tmp/chk.o
  cmp -s "/tmp/esclib-baseline/$n.o" /tmp/chk.o || { echo "CHANGED: $f"; fail=1; }
done
[ $fail -eq 0 ] && echo "ALL 24 OBJECTS IDENTICAL" || echo "CODE WAS ALTERED - investigate"
```

Expected: `ALL 24 OBJECTS IDENTICAL`.

- [ ] **Step 2: Verify the tree still compiles clean with warnings on**

```bash
cd "C:/Users/engin/Documents/GitHub/esclib"
for f in src/*/*.c drv/*.c; do
  m=$(basename $(dirname "$f")); inc="inc/$m"; [ -d "$inc" ] || inc="drv"
  o=$(arm-none-eabi-gcc -c -Wall -Wextra -I"$inc" -Idrv "$f" -o /dev/null 2>&1)
  [ -n "$o" ] && { echo "--- $f"; echo "$o" | head -3; }
done
```

Expected: only the four known `unused parameter 'driver'` warnings from the empty driver stubs. Anything else is a regression.

- [ ] **Step 3: Confirm no stale tags survive anywhere**

```bash
cd "C:/Users/engin/Documents/GitHub/esclib"
grep -rn '@about\|@cdate\|@content\|@device:\|@email\|@address\|@history:\|@notes:' src drv template --include=*.c --include=*.h
echo "exit: $?  (1 means no matches, which is what we want)"
```

Expected: no output, exit 1.

- [ ] **Step 4: Confirm no function comment still opens with a single asterisk**

```bash
cd "C:/Users/engin/Documents/GitHub/esclib"
grep -rn -B0 -A1 '^/\*$' src drv --include=*.c | head -20
echo "--- count:"; grep -rc '^/\*$' src drv --include=*.c | grep -v ':0' || echo "none"
```

Expected: `none`. Every documentation block now opens with `/**`.

- [ ] **Step 5: Append the convention to `codingReference.md`**

This is where contributors already look for style rules, so the convention belongs beside the brace and loop rules rather than only in a spec file. Add a `## Doxygen Comments` section containing the function format, the `driver` direction table, and the banner format from the Reference section of this plan.

- [ ] **Step 6: Point `CLAUDE.md` at it**

In the "Source file header block" section of `CLAUDE.md`, replace the description of the old `@about`/`@cdate`/`@content` banner with the new format and add a line: documentation lives in `.c` files only, headers stay pure declarations, and the full convention is in `codingReference.md`.

- [ ] **Step 7: Commit**

```bash
git add codingReference.md CLAUDE.md
git commit -m "* Document the Doxygen comment convention"
```

---

## Notes on verification limits

Doxygen is not installed on this machine. The plan proves that no code changed and that the tree still compiles, but it cannot demonstrate the generated HTML or a zero warning run. After Task 10, the author should run `doxygen` at the repository root and check two things: that `doc/html/index.html` lists every module, and that the warning log is empty. `WARN_NO_PARAMDOC = YES` means any parameter missed by this plan will show up there.
