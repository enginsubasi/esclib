#!/bin/sh
#
# Static checks over the esclib tree.
#
# Run from the repository root:
#     sh scripts/check.sh
#
# Three things are checked, and all three are rules the tree already holds:
#
#   1. Warnings.   Every .c under src/ and drv/ compiles clean under
#                  -Wall -Wextra. A new warning is a regression here, not
#                  background noise.
#   2. Headers.    Every header is independently includable and all of them
#                  coexist in one translation unit. This is what catches a
#                  duplicate include guard or a clashing typedef.
#   3. Symbols.    Every exported symbol is referenced by at least one test,
#                  and every exported symbol carries its module prefix.
#
# Like run_tests.sh this needs no maintenance: the file lists come from the
# tree and the include directories come from the layout, so adding a module
# requires no edit here.
#
# Environment:
#     CC   compiler to use, default arm-none-eabi-gcc, falling back to gcc.
#          Any compiler works; the sources are never executed by this script.
#     NM   symbol lister, default derived from CC.
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

# arm-none-eabi-gcc -> arm-none-eabi-nm, gcc -> nm, /path/to/gcc -> /path/to/nm
if [ -z "$NM" ]; then
    NM=$(printf '%s' "$CC" | sed 's/gcc$/nm/')
    command -v "$NM" >/dev/null 2>&1 || NM=nm
fi

outdir=$(mktemp -d 2>/dev/null || echo /tmp/esclib_check.$$)
mkdir -p "$outdir/objs"

failed=0

echo "using CC=$CC NM=$NM"
echo

# ---------------------------------------------------------------------------
# 1. Warnings
# ---------------------------------------------------------------------------
echo "== warnings =="

warned=0
compiled=0

for f in src/*/*.c drv/*.c; do
    [ -f "$f" ] || continue

    m=$(basename "$(dirname "$f")")
    inc="inc/$m"
    [ -d "$inc" ] || inc="drv"

    obj="$outdir/objs/$(basename "$f" .c).o"

    if $CC -c -Wall -Wextra -I"$inc" -Idrv "$f" -o "$obj" 2>"$outdir/cc.err"; then
        compiled=$((compiled + 1))
    else
        echo "COMPILE FAIL  $f"
        sed 's/^/    /' "$outdir/cc.err"
        warned=$((warned + 1))
        continue
    fi

    if [ -s "$outdir/cc.err" ]; then
        echo "WARNINGS  $f"
        sed 's/^/    /' "$outdir/cc.err"
        warned=$((warned + 1))
    fi
done

if [ "$warned" -eq 0 ]; then
    echo "OK  $compiled files, no warnings"
else
    echo "FAIL  $warned of $((compiled + warned)) files"
    failed=$((failed + 1))
fi
echo

# ---------------------------------------------------------------------------
# 2. Header coexistence
# ---------------------------------------------------------------------------
echo "== headers =="

idirs=""
for d in inc/*/ drv/; do
    [ -d "$d" ] || continue
    idirs="$idirs -I${d%/}"
done

hcount=0
: > "$outdir/allhdr.c"
for h in inc/*/*.h drv/*.h; do
    [ -f "$h" ] || continue
    echo "#include \"$(basename "$h")\"" >> "$outdir/allhdr.c"
    hcount=$((hcount + 1))
done
echo "int main ( void ) { return ( 0 ); }" >> "$outdir/allhdr.c"

if $CC -c -Wall -Wextra $idirs "$outdir/allhdr.c" -o "$outdir/allhdr.o" 2>"$outdir/hdr.err" \
   && [ ! -s "$outdir/hdr.err" ]; then
    echo "OK  $hcount headers coexist in one translation unit"
else
    echo "FAIL  headers do not coexist"
    sed 's/^/    /' "$outdir/hdr.err"
    failed=$((failed + 1))
fi
echo

# ---------------------------------------------------------------------------
# 3. Exported symbols
# ---------------------------------------------------------------------------
echo "== symbols =="

$NM "$outdir"/objs/*.o 2>/dev/null | grep ' T ' | awk '{print $3}' | sort -u > "$outdir/syms.txt"
symcount=$(wc -l < "$outdir/syms.txt" | tr -d ' ')

cat test/*/*.c > "$outdir/alltests.c" 2>/dev/null

# Module prefixes, taken from the source file names rather than from a list.
# A header and its source share a name, and the prefix is that name with the
# known group renames applied.
: > "$outdir/prefixes.txt"
for f in src/*/*.c drv/*.c; do
    [ -f "$f" ] || continue
    n=$(basename "$f" .c)
    case "$n" in
        basicmath)   n=math ;;
        basicarray)  n=array ;;
        basicmatrix) n=matrix ;;
        statistic)   n=stat ;;
        circBuf)     n=circBuf ;;
        hc595_drv)   n=hc595 ;;
        hc597_drv)   n=hc597 ;;
        dcmotor)     n=dcMotor ;;
        matrixlib)   n=matrix ;;
    esac
    echo "$n" >> "$outdir/prefixes.txt"
done

uncalled=0
unprefixed=0

while read -r s; do
    [ -n "$s" ] || continue

    if ! grep -q "\\b$s\\b" "$outdir/alltests.c" 2>/dev/null; then
        echo "UNCALLED    $s"
        uncalled=$((uncalled + 1))
    fi

    hit=0
    while read -r p; do
        case "$s" in
            "$p"*) hit=1 ;;
        esac
    done < "$outdir/prefixes.txt"

    if [ "$hit" -eq 0 ]; then
        echo "UNPREFIXED  $s"
        unprefixed=$((unprefixed + 1))
    fi
done < "$outdir/syms.txt"

if [ "$uncalled" -eq 0 ] && [ "$unprefixed" -eq 0 ]; then
    echo "OK  $symcount exported symbols, all tested and all prefixed"
else
    echo "FAIL  $uncalled untested, $unprefixed unprefixed, of $symcount"
    failed=$((failed + 1))
fi
echo

echo "artifacts in $outdir"

if [ "$failed" -eq 0 ]; then
    echo "all checks passed"
else
    echo "$failed check(s) failed"
fi

exit "$failed"
