#!/bin/sh
#
# Portability checks over the esclib tree.
#
# Run from the repository root:
#     sh scripts/portable.sh
#
# Three things the tree claims and nothing checked until 15/09/2026:
#
#   1. C99.        Every .c under src/ and drv/ compiles under -std=c99
#                  -pedantic-errors. The style is C89 — declarations at the
#                  top of a block, /* */ comments — but the types are not:
#                  <stdint.h> and int64_t are C99, so C99 is the real floor
#                  and claiming C89 would be claiming something the tree
#                  cannot hold. What -pedantic-errors buys is the GNU
#                  extension that compiles quietly with this compiler and
#                  stops somebody else's.
#   2. C++ headers. Every header is includable from a C++ translation unit,
#                  alone and all together. That is what the extern "C" block
#                  in every header is for, and this is the only thing that
#                  checks one is there. A library copied into other people's
#                  projects meets a C++ one sooner or later.
#   3. C++ modules. Every .c compiles as C++ too. That is stricter than the
#                  tree needs, since the modules are built as C in every real
#                  use, and it is worth having anyway: C++ refuses the
#                  implicit void* conversion and the implicit narrowing that
#                  C accepts, so it reads each module the way a second
#                  compiler would.
#
# Like the other scripts this needs no maintenance — the file lists come from
# the tree and the include directories from the layout, so a new module
# requires no edit here. Nothing is executed and nothing is linked.
#
# Environment:
#     CC   C compiler, default arm-none-eabi-gcc, falling back to gcc.
#     CXX  C++ compiler, default derived from CC, falling back to g++.
#
# Exit status is the number of checks that failed.

if [ ! -d inc ] || [ ! -d src ]; then
    echo "run from the repository root (no inc/ or src/ here)" >&2
    exit 126
fi

if [ -z "$CC" ]; then
    if command -v arm-none-eabi-gcc >/dev/null 2>&1; then
        CC=arm-none-eabi-gcc
    else
        CC=gcc
    fi
fi

# arm-none-eabi-gcc -> arm-none-eabi-g++, gcc -> g++, /path/to/gcc -> /path/to/g++
if [ -z "$CXX" ]; then
    CXX=$(printf '%s' "$CC" | sed 's/gcc$/g++/')
    command -v "$CXX" >/dev/null 2>&1 || CXX=g++
fi

outdir=$(mktemp -d 2>/dev/null || echo /tmp/esclib_portable.$$)
mkdir -p "$outdir"

failed=0

echo "using CC=$CC CXX=$CXX"
echo

idirs=""
for d in inc/*/ drv/; do
    [ -d "$d" ] || continue
    idirs="$idirs -I${d%/}"
done

# ---------------------------------------------------------------------------
# 1. C99, pedantically
# ---------------------------------------------------------------------------
echo "== c99 =="

bad=0
count=0

for f in src/*/*.c drv/*.c; do
    [ -f "$f" ] || continue

    m=$(basename "$(dirname "$f")")
    inc="inc/$m"
    [ -d "$inc" ] || inc="drv"

    if $CC -c -std=c99 -pedantic-errors -Wall -Wextra -I"$inc" -Idrv \
           "$f" -o "$outdir/c99.o" 2>"$outdir/c99.err" \
       && [ ! -s "$outdir/c99.err" ]; then
        count=$((count + 1))
    else
        echo "FAIL  $f"
        sed 's/^/    /' "$outdir/c99.err"
        bad=$((bad + 1))
    fi
done

if [ "$bad" -eq 0 ]; then
    echo "OK  $count files under -std=c99 -pedantic-errors"
else
    echo "FAIL  $bad of $((count + bad)) files"
    failed=$((failed + 1))
fi
echo

# ---------------------------------------------------------------------------
# 2. Headers from C++
# ---------------------------------------------------------------------------
echo "== c++ headers =="

hcount=0
: > "$outdir/allhdr.cpp"
for h in inc/*/*.h drv/*.h; do
    [ -f "$h" ] || continue
    echo "#include \"$(basename "$h")\"" >> "$outdir/allhdr.cpp"
    hcount=$((hcount + 1))
done
echo "int main ( void ) { return ( 0 ); }" >> "$outdir/allhdr.cpp"

if $CXX -c -Wall -Wextra $idirs "$outdir/allhdr.cpp" -o "$outdir/allhdr.o" \
        2>"$outdir/hdr.err" \
   && [ ! -s "$outdir/hdr.err" ]; then
    echo "OK  $hcount headers coexist in one C++ translation unit"
else
    echo "FAIL  headers do not coexist as C++"
    sed 's/^/    /' "$outdir/hdr.err"
    failed=$((failed + 1))
fi

bad=0

for h in inc/*/*.h drv/*.h; do
    [ -f "$h" ] || continue

    printf '#include "%s"\nint main ( void ) { return ( 0 ); }\n' \
           "$(basename "$h")" > "$outdir/one.cpp"

    if ! $CXX -c -Wall -Wextra $idirs "$outdir/one.cpp" -o "$outdir/one.o" \
              2>"$outdir/one.err" || [ -s "$outdir/one.err" ]; then
        echo "FAIL  $h alone"
        sed 's/^/    /' "$outdir/one.err"
        bad=$((bad + 1))
    fi
done

if [ "$bad" -eq 0 ]; then
    echo "OK  every header includable on its own from C++"
else
    echo "FAIL  $bad headers"
    failed=$((failed + 1))
fi
echo

# ---------------------------------------------------------------------------
# 3. Modules as C++
# ---------------------------------------------------------------------------
echo "== c++ modules =="

bad=0
count=0

for f in src/*/*.c drv/*.c; do
    [ -f "$f" ] || continue

    m=$(basename "$(dirname "$f")")
    inc="inc/$m"
    [ -d "$inc" ] || inc="drv"

    if $CXX -c -x c++ -Wall -Wextra -I"$inc" -Idrv \
            "$f" -o "$outdir/cpp.o" 2>"$outdir/cpp.err" \
       && [ ! -s "$outdir/cpp.err" ]; then
        count=$((count + 1))
    else
        echo "FAIL  $f"
        sed 's/^/    /' "$outdir/cpp.err"
        bad=$((bad + 1))
    fi
done

if [ "$bad" -eq 0 ]; then
    echo "OK  $count modules compile as C++"
else
    echo "FAIL  $bad of $((count + bad)) modules"
    failed=$((failed + 1))
fi
echo

echo "checks failed: $failed"
rm -rf "$outdir"
exit "$failed"
