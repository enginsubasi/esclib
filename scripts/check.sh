#!/bin/sh
#
# Static checks over the esclib tree.
#
# Run from the repository root:
#     sh scripts/check.sh
#
# Four things are checked, and all four are rules the tree already holds:
#
#   1. Warnings.   Every .c under src/ and drv/ compiles clean under
#                  -Wall -Wextra. A new warning is a regression here, not
#                  background noise. STRICT=1 adds -Wconversion and its
#                  neighbours, which the tree is also clean under.
#   2. Headers.    Every header is independently includable and all of them
#                  coexist in one translation unit. This is what catches a
#                  duplicate include guard or a clashing typedef.
#   3. Symbols.    Every exported symbol is referenced by at least one test,
#                  and every exported symbol carries its module prefix.
#   4. Storage.    No module holds static state of its own. The caller owns
#                  every buffer, so a module's .data and .bss must both be
#                  empty — a writable static is the rule being broken.
#
# Like run_tests.sh this needs no maintenance: the file lists come from the
# tree and the include directories come from the layout, so adding a module
# requires no edit here.
#
# Environment:
#     CC     compiler to use, default arm-none-eabi-gcc, falling back to gcc.
#            Any compiler works; the sources are never executed by this script.
#     STRICT set to 1 to add -Wconversion -Wsign-conversion -Wshadow
#            -Wdouble-promotion -Wcast-qual. Every one of those was clean
#            across the tree on 15/09/2026, which is why it is a gate rather
#            than an aspiration. The eight warnings it found were all one
#            pattern — an array length converted to float for a divide — and
#            they are written out now rather than left implicit.
#     NM   symbol lister, default derived from CC.
#     SIZE section sizer, default derived from CC.
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

if [ -z "$SIZE" ]; then
    SIZE=$(printf '%s' "$CC" | sed 's/gcc$/size/')
    command -v "$SIZE" >/dev/null 2>&1 || SIZE=size
fi

STRICT=${STRICT:-0}

if [ "$STRICT" = "1" ]; then
    WARNFLAGS="-Wall -Wextra -Wconversion -Wsign-conversion -Wshadow -Wdouble-promotion -Wcast-qual"
else
    WARNFLAGS="-Wall -Wextra"
fi

outdir=$(mktemp -d 2>/dev/null || echo /tmp/esclib_check.$$)
mkdir -p "$outdir/objs"

failed=0

echo "using CC=$CC NM=$NM SIZE=$SIZE"
echo "warning flags: $WARNFLAGS"
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

    if $CC -c $WARNFLAGS -I"$inc" -Idrv "$f" -o "$obj" 2>"$outdir/cc.err"; then
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

if $CC -c $WARNFLAGS $idirs "$outdir/allhdr.c" -o "$outdir/allhdr.o" 2>"$outdir/hdr.err" \
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

# ---------------------------------------------------------------------------
# 4. Static storage
# ---------------------------------------------------------------------------
echo "== storage =="

# "The caller owns all storage. The module never allocates and never holds
# static state of its own." That rule is stated in rules.md and until now
# nothing checked it. A writable static shows up as .data when it has an
# initializer and .bss when it does not, so both being empty is the whole
# test. A read only table is .rodata and counts as code, which is why crc16
# may carry one and still pass here.
statefull=0
objcount=0

for o in "$outdir"/objs/*.o; do
    [ -f "$o" ] || continue

    objcount=$((objcount + 1))

    line=$($SIZE "$o" 2>/dev/null | tail -1)
    dataSize=$(printf '%s' "$line" | awk '{print $2}')
    bssSize=$(printf '%s' "$line" | awk '{print $3}')

    [ -n "$dataSize" ] || dataSize=0
    [ -n "$bssSize" ] || bssSize=0

    if [ "$dataSize" -ne 0 ] || [ "$bssSize" -ne 0 ]; then
        echo "STATIC STATE  $(basename "$o" .o): data $dataSize, bss $bssSize"
        statefull=$((statefull + 1))
    fi
done

if [ "$statefull" -eq 0 ]; then
    echo "OK  $objcount modules, none holds static state"
else
    echo "FAIL  $statefull of $objcount modules hold static state"
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
