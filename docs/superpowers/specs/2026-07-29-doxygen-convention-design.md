# Doxygen Comment Convention for esclib

Date: 2026-07-29
Status: approved

## Problem

The library carries documentation that Doxygen cannot read.

| Measurement | Count |
|---|---|
| `/**` blocks in `.c` files (Doxygen reads these) | 34 |
| `/*` blocks in `.c` files (Doxygen ignores these) | 97 |
| `@about` occurrences (not a Doxygen command) | 120 |
| `@brief` occurrences | 11 |
| `@return` occurrences | 8 |
| Doxyfile in repository | none |

Two problems follow from this. About three quarters of the function comments open with a single asterisk, so Doxygen never sees them. The file banners do open with `/**`, so Doxygen does read them, but they are built from commands it does not recognise — `@about`, `@cdate`, `@history`, `@device`, `@content`, `@email`, `@address` — and each one raises an unknown command warning.

The goal is a light, uniform convention: enough structure to generate useful output, not so much that writing it becomes a chore.

## Decisions

Four questions were settled before design:

1. **Documentation lives in `.c` files only.** Headers stay pure declarations. This matches how the library is written today and keeps one copy of every description.
2. **`@return` is part of the tag set.** 47 of 97 public functions return a value — 18 return a `TRUE`/`FALSE` status, 29 return a computed result. `@param[out]` documents pointer outputs but says nothing about the return value, so `brief`/`in`/`out`/`note` alone would leave those 47 undocumented.
3. **File banners convert to Doxygen-valid equivalents.** Same information, same visual shape, no warnings.
4. **A minimal Doxyfile is added**, so the author can run Doxygen and inspect the output directly.

## Function comment format

```c
/**
 * @brief   Adds one word to the circular buffer.
 * @param[in,out] driver  Buffer state.
 * @param[in]     data    Word to store.
 * @return  TRUE when the word was stored, FALSE when the buffer is
 *          full and the behaviour is BB_STOP.
 * @note    Only present when there is something non obvious to say.
 */
uint8_t circBufAddu32 ( circBufu32_t* driver, uint32_t data )
```

Rules:

- The block opens with `/**`. This is the change that makes the existing 97 invisible comments visible.
- `@brief` is one sentence and ends with a period.
- Every parameter is documented, and every parameter carries an explicit direction.
- `@return` appears on non void functions only.
- `@note` appears only when it carries information. It is not filler.
- The six `static` helpers (`isEqualf`, `swapForSort`, `swapForSortu32`, `swapForSorti32`, `hc595DlyCtrl`, `hc597DlyCtrl`) are documented the same way, because the Doxyfile sets `EXTRACT_STATIC = YES`.
- Parameter names and their descriptions are column aligned, matching how the rest of the codebase aligns things.

### Direction of the `driver` parameter

The direction depends on what the function does to the state, so it follows the function family:

| Family | Direction | Reason |
|---|---|---|
| `xxxInit` | `[out]` | The struct is fully written. |
| `xxxUpdate`, `xxxIteration`, `xxxControl`, `xxxReceive`, `xxxEvaluate`, `xxxAdd`, `xxxRead`, `xxxTimeoutCounter`, `xxxChange*` | `[in,out]` | Existing state is read and then modified. |
| `xxxGetValue`, `xxxGetOutput`, `xxxGetLength`, `xxxGetStatus` | `[in]` | State is only read. |

`bininpGetRisingValue` is the one exception in the read family: it clears the `rising` flag, so it is `[in,out]`.

## File banner format

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
  * @note
  * Free text notes, when the file has any.
  *
  ******************************************************************************
  */
```

Mapping from the old banner:

| Old | New | Note |
|---|---|---|
| `@file:` | `@file` | The trailing colon is dropped; Doxygen treats it as part of the filename. |
| `@author:`, `@email:`, `@address:` | `@author` | Folded into one line. `@email` and `@address` are not Doxygen commands. |
| `@version:` | `@version` | The `v` prefix is dropped. |
| `@cdate:` | `@date` | `@cdate` is not a Doxygen command. |
| `@about:` | `@brief` | |
| `@device:` | `@par Device` | `@par` renders a titled paragraph. |
| `@history:` | `@par History` | Entries end with `@n` to force line breaks. |
| `@notes:` | `@note` | |
| `@content:` | dropped | See below. |

The `@content` FUNCTIONS list is removed. Doxygen generates the function list for each file automatically, and the hand maintained lists have already drifted from reality — `basicmath.c` lists a `findMin32` that does not exist, and `sort.c` lists `bubbleSort added` twice. A generated list cannot drift.

## Scope

| Target | Count |
|---|---|
| `.c` files getting function comments | 21 |
| `.c` files getting banner conversion | 23 |

Of the 24 `.c` files, three contain no function definitions and so get a banner only: `src/communication/comsafe.c` and `src/matrix/matrixlib.c` hold nothing but a banner, and `src/communication/comsec.c` is an empty file with no banner to convert.
| Template files | 2 — `template/src/generic.c`, `template/inc/generic.h` |
| New `Doxyfile` | 1 |
| `.gitignore` | one added line for the generated `doc/` directory |

Headers under `inc/` and `drv/` are not touched.

The template files must be updated. New modules are started by copying them, so leaving them on the old format would reintroduce the old style with the first new module.

## Doxyfile

```
PROJECT_NAME          = esclib
OUTPUT_DIRECTORY      = doc
INPUT                 = inc src drv
RECURSIVE             = YES
OPTIMIZE_OUTPUT_FOR_C = YES
EXTRACT_STATIC        = YES
WARN_NO_PARAMDOC      = YES
GENERATE_LATEX        = NO
```

`WARN_NO_PARAMDOC = YES` makes Doxygen report any parameter left undocumented, which turns the convention into something the tool enforces rather than something reviewers have to catch.

## Verification

This change touches comments only. No statement, expression, or declaration changes.

That claim is checkable. Every `.c` file is compiled to an object file before the change and again after, and the two are compared byte for byte. Every file must match. A file that differs means code was altered by accident.

Doxygen itself is not installed on the development machine, so the generated output and the warning count cannot be demonstrated here. The Doxyfile ships untested, and running it is left to the author.

## Out of scope

- Header files.
- The four empty driver stubs and the empty communication modules. They stay undocumented beyond a brief, because documenting behaviour that does not exist yet would be fiction.
- Any change to code behaviour.
