#!/bin/sh
#
# Stack usage of every esclib module, and the two rules it has to keep.
#
# Run from the repository root:
#     sh scripts/stack.sh
#
# The caller owns all storage here and scripts/check.sh proves that for the
# static half: no module object has a byte of .data or .bss. The stack is the
# other half of the same promise and nothing measured it. A library aimed at
# parts with a few kilobytes of RAM, much of it called from interrupts, has to
# be able to say how deep it goes — and two of its claims are about exactly
# that: sortHeap "stays in place without recursing", and nothing allocates.
#
# So this is a gate with two rules, and a report beside it:
#
#   - Nothing recurses. A call graph with a cycle in it has no bound that can
#     be read off the source, and on a part with no MMU an overflow is silent
#     corruption rather than a fault. The graph is the compiler's own, from
#     -fcallgraph-info, so it is the code that was emitted rather than the code
#     that was written: a recursion the optimizer turned into a loop is not
#     reported, because it is no longer there.
#   - No frame is dynamic. -fstack-usage marks a function whose frame depends
#     on its arguments, which in C means a variable length array or alloca.
#     That is an allocation in all but name, and the rule is no allocation.
#
# The report is the deepest path through each module: a function's own frame
# plus the deepest of the module functions it calls. Two things are outside
# it and the flags say where. Compiler runtime helpers (__aeabi_fmul and the
# rest, scripts/runtime.sh lists them) belong to the toolchain, and a
# callback installed at Init belongs to the caller; neither can be measured
# from here, and a caller adding them up for a whole program has to add those
# two in.
#
# The gate is tested on every run before it is trusted. A probe with a
# recursion and a variable length array in it is built first and both rules
# have to fire on it; if they do not, the compiler has stopped reporting what
# this script reads and the result for the tree would be meaningless.
#
# Environment:
#     CC       compiler, default arm-none-eabi-gcc. It needs -fcallgraph-info,
#              which gcc has had since version 10.
#     CFLAGS   optimization and target flags, default -Os for a Cortex-M0, the
#              smallest part this library is aimed at.
#
# Exit status is the number of rules broken plus the number of modules that
# did not build, and 1 more if the probe did not trip the gate.

CC=${CC:-arm-none-eabi-gcc}
CFLAGS=${CFLAGS:--Os -mcpu=cortex-m0 -mthumb}

if [ ! -d inc ] || [ ! -d src ]; then
    echo "run from the repository root (no inc/ or src/ here)" >&2
    exit 126
fi

if ! command -v "$CC" >/dev/null 2>&1; then
    echo "$CC not found. Set CC to a compiler you have." >&2
    exit 127
fi

outdir=$(mktemp -d 2>/dev/null || echo /tmp/esclib_stack.$$)
mkdir -p "$outdir"

echo "using CC=$CC CFLAGS=$CFLAGS"
echo

# Reads one module's .su and .ci and prints one line:
#     ROW <module> <deepest function> <path bytes> <largest frame> <helpers> <callbacks>
# followed by a FAIL line for every rule broken. Function names are taken
# after the last colon, because a static function appears in both files
# qualified by its source path and C names cannot contain one.
analyse ( ) {
    awk -v module="$1" '
        function bare( s ) { sub ( /^.*:/, "", s ); return s }

        FILENAME ~ /\.su$/ {
            split ( $0, field, "\t" )
            name = bare( field[ 1 ] )
            frame[ name ] = field[ 2 ] + 0
            kind[ name ] = field[ 3 ]
            order[ ++count ] = name
            next
        }

        /^edge:/ {
            s = $0; sub ( /.*sourcename: "/, "", s ); sub ( /".*/, "", s )
            t = $0; sub ( /.*targetname: "/, "", t ); sub ( /".*/, "", t )
            s = bare( s ); t = bare( t )
            ++edges[ s ]
            target[ s, edges[ s ] ] = t
        }

        # Deepest path from f in bytes. state 1 is on the current path, so
        # meeting it again is a cycle; state 2 is finished and memoised.
        function depth( f,    i, t, d, best ) {
            if ( state[ f ] == 2 ) { return deep[ f ] }
            if ( state[ f ] == 1 ) { cyclic[ f ] = 1; return 0 }
            state[ f ] = 1
            best = 0
            for ( i = 1; i <= edges[ f ]; ++i ) {
                t = target[ f, i ]
                if ( t in frame ) {
                    if ( state[ t ] == 1 ) { cyclic[ t ] = 1; loop[ t ] = f }
                    d = depth( t )
                    if ( helper[ t ] ) { helper[ f ] = 1 }
                    if ( callback[ t ] ) { callback[ f ] = 1 }
                    if ( d > best ) { best = d }
                } else if ( t == "__indirect_call" ) {
                    callback[ f ] = 1
                } else {
                    helper[ f ] = 1
                }
            }
            state[ f ] = 2
            deep[ f ] = frame[ f ] + best
            return deep[ f ]
        }

        END {
            top = ""; topDepth = -1; topFrame = 0; anyHelper = 0; anyCallback = 0
            for ( i = 1; i <= count; ++i ) {
                f = order[ i ]
                d = depth( f )
                if ( d > topDepth ) { topDepth = d; top = f }
                if ( frame[ f ] > topFrame ) { topFrame = frame[ f ] }
                if ( helper[ f ] ) { anyHelper = 1 }
                if ( callback[ f ] ) { anyCallback = 1 }
            }
            printf "ROW %s %s %d %d %s %s\n", module, top, topDepth, topFrame, \
                ( anyHelper ? "yes" : "-" ), ( anyCallback ? "yes" : "-" )
            for ( i = 1; i <= count; ++i ) {
                f = order[ i ]
                if ( cyclic[ f ] ) {
                    printf "FAIL %s %s recurses, through %s\n", module, f, loop[ f ]
                }
                if ( kind[ f ] != "static" ) {
                    printf "FAIL %s %s has a %s frame\n", module, f, kind[ f ]
                }
            }
        }
    ' "$2" "$3"
}

# The probe. Two calls in the recursion, because one of them could be turned
# into a loop and a probe the optimizer can dissolve proves nothing.
cat > "$outdir/probe.c" <<'EOF'
int probeRecursive ( int n )
{
    return ( n < 2 ) ? n : ( probeRecursive ( n - 1 ) + probeRecursive ( n - 2 ) );
}

void probeDynamic ( int n )
{
    volatile char scratch[ n ];
    scratch[ 0 ] = 0;
}
EOF

probeBites=0

if ( cd "$outdir" && $CC -c $CFLAGS -fstack-usage -fcallgraph-info=su probe.c -o probe.o ) \
        2>"$outdir/cc.err"; then
    analyse probe "$outdir/probe.su" "$outdir/probe.ci" > "$outdir/probe.out"
    if grep -q '^FAIL probe probeRecursive recurses' "$outdir/probe.out" \
            && grep -q '^FAIL probe probeDynamic has a dynamic' "$outdir/probe.out"; then
        probeBites=1
    fi
else
    sed 's/^/    /' "$outdir/cc.err"
fi

if [ "$probeBites" -eq 1 ]; then
    echo "OK    the probe trips both rules"
else
    echo "FAIL  the probe did not trip both rules, so nothing below can be trusted"
fi
echo

failures=0

for f in src/*/*.c drv/*.c; do
    [ -f "$f" ] || continue

    m=$(basename "$(dirname "$f")")
    inc="inc/$m"
    [ -d "$inc" ] || inc="drv"

    name=$(basename "$f" .c)

    # -fstack-usage and -fcallgraph-info name their output after the object,
    # so each module is built into a directory of its own.
    mkdir -p "$outdir/$name"

    if ! $CC -c $CFLAGS -fstack-usage -fcallgraph-info=su -I"$inc" -Idrv "$f" \
            -o "$outdir/$name/$name.o" 2>"$outdir/cc.err"; then
        echo "BUILD FAIL  $f"
        sed 's/^/    /' "$outdir/cc.err"
        failures=$((failures + 1))
        continue
    fi

    # A module that built and was not analysed would read as one that broke
    # no rule, so a missing row is a failure in its own right.
    analyse "$name" "$outdir/$name/$name.su" "$outdir/$name/$name.ci" > "$outdir/one"
    if grep -q "^ROW $name " "$outdir/one"; then
        cat "$outdir/one" >> "$outdir/rows"
    else
        echo "NO REPORT  $f"
        failures=$((failures + 1))
    fi
done

if [ ! -f "$outdir/rows" ]; then
    echo "nothing built" >&2
    exit 1
fi

printf '%-16s %-26s %5s %5s %7s %9s\n' module deepest path frame helpers callbacks
printf '%-16s %-26s %5s %5s %7s %9s\n' ---------------- -------------------------- \
    ----- ----- ------- ---------

grep '^ROW ' "$outdir/rows" | sort -k4 -rn | \
    awk '{ printf "%-16s %-26s %5d %5d %7s %9s\n", $2, $3, $4, $5, $6, $7 }'

echo
grep '^ROW ' "$outdir/rows" | sort -k4 -rn | head -1 | \
    awk '{ printf "deepest in the tree: %s, %d bytes through %s\n", $2, $4, $3 }'

echo
echo "path is a function's frame plus the deepest module function it calls."
echo "helpers means compiler runtime calls, and callbacks means functions"
echo "installed at Init; neither is in the path, and a whole program adds both."
echo

broken=$(grep -c '^FAIL ' "$outdir/rows")

if [ "$broken" -eq 0 ]; then
    echo "OK    nothing recurses and no frame is dynamic"
else
    grep '^FAIL ' "$outdir/rows" | sed 's/^FAIL /FAIL  /'
fi

failures=$((failures + broken))

if [ "$probeBites" -ne 1 ]; then
    failures=$((failures + 1))
fi

echo
echo "objects in $outdir"

exit "$failures"
