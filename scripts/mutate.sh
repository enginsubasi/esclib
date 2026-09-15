#!/bin/sh
#
# Checks that the tests still bite.
#
# Run from the repository root:
#     sh scripts/mutate.sh                       every mutation
#     sh scripts/mutate.sh fir-history           only the ones whose name matches
#
# The testing rule in this tree is that when a bug is fixed the test gets a
# check aimed at that specific bug, so the regression fails rather than passing
# quietly. CLAUDE.md keeps the table of which test pins which bug. Nothing
# checked that those pins still bite — and a pin that stops biting is exactly
# the silent failure the pins exist to prevent. Soften an assertion while
# tidying a test and nothing anywhere would notice.
#
# So: each mutation under scripts/mutations/ reintroduces one real defect, and
# names the test that has to fail because of it. A mutation that is applied and
# the test still passes is a hole in the test, reported here as loudly as a
# broken build.
#
# Every mutation is applied to a copy of the working tree state and reverted
# immediately, whatever the outcome. Nothing is left modified.
#
# Adding one costs a file. There is no list to keep in sync.
#
# Environment:
#     CC       host compiler, passed through to run_tests.sh, default gcc
#     CFLAGS   extra flags, passed through
#
# Exit status is the number of mutations that survived or could not be applied.

if [ ! -d inc ] || [ ! -d scripts/mutations ]; then
    echo "run from the repository root (no inc/ or scripts/mutations/ here)" >&2
    exit 126
fi

FILTER=${1:-}

outdir=$(mktemp -d 2>/dev/null || echo /tmp/esclib_mutate.$$)
mkdir -p "$outdir"

# Whatever happens, put the file back. A mutation left behind in the tree is
# worse than no mutation testing at all.
mutatedFile=""
backupFile="$outdir/original"

restore ( ) {
    if [ -n "$mutatedFile" ] && [ -f "$backupFile" ]; then
        cp "$backupFile" "$mutatedFile"
        mutatedFile=""
    fi
}

trap 'restore; exit 130' INT TERM
trap 'restore' EXIT

total=0
caught=0
survived=0
broken=0

for mut in scripts/mutations/*.mut; do
    [ -f "$mut" ] || continue

    name=$(basename "$mut" .mut)

    case "$name" in
        *"$FILTER"*) ;;
        *) continue ;;
    esac

    total=$((total + 1))

    # Pull the four fields out of the mutation file. FIND and REPLACE are
    # literal blocks between their markers, kept exactly as written.
    target=$(awk '$1 == "FILE" { print $2; exit }' "$mut")
    test=$(awk '$1 == "TEST" { print $2; exit }' "$mut")
    what=$(awk '/^# what:/ { sub ( /^# what: */, "" ); print; exit }' "$mut")

    awk 'f == 1 && /^--- REPLACE$/ { f = 2; next }
         f == 1 { print }
         /^--- FIND$/ { f = 1 }' "$mut" > "$outdir/find"

    awk 'f == 1 && /^--- END$/ { f = 2; next }
         f == 1 { print }
         /^--- REPLACE$/ { f = 1 }' "$mut" > "$outdir/replace"

    if [ -z "$target" ] || [ -z "$test" ] || [ ! -f "$target" ]; then
        echo "MALFORMED  $name (FILE or TEST missing, or the file is gone)"
        broken=$((broken + 1))
        continue
    fi

    # Apply, by literal substring replacement rather than by regex or by a
    # patch. A regex would need every metacharacter in a block of C escaped,
    # and a patch would go stale the moment anything above it moved; an exact
    # block either matches or it does not.
    cp "$target" "$backupFile"

    awk -v findFile="$outdir/find" -v replaceFile="$outdir/replace" '
        # A carriage return is stripped on both sides. On Windows the .c
        # files check out with CRLF and these blocks with LF, and a block
        # that differed only in line endings would report as stale. The
        # mutated file is written with LF and then thrown away; the
        # original comes back from a byte for byte copy, so nothing in
        # the tree is reformatted by running this.
        function slurp ( path,    line, acc ) {
            acc = ""
            while ( ( getline line < path ) > 0 )
            {
                sub ( /\r$/, "", line )
                acc = acc line "\n"
            }
            close ( path )
            return acc
        }
        BEGIN {
            # No space before the paren: awk allows one for a built in and
            # forbids it for a user defined function, which is the one place
            # this script cannot follow the library house style.
            find = slurp(findFile)
            replace = slurp(replaceFile)
        }
        { line = $0; sub ( /\r$/, "", line ); body = body line "\n" }
        END {
            count = 0
            rest = body
            while ( ( at = index ( rest, find ) ) > 0 )
            {
                count += 1
                rest = substr ( rest, at + length ( find ) )
            }

            if ( count != 1 )
            {
                printf "%d", count > "/dev/stderr"
                exit 1
            }

            at = index ( body, find )
            printf "%s%s%s", substr ( body, 1, at - 1 ), replace, \
                             substr ( body, at + length ( find ) )
        }
    ' "$target" > "$outdir/mutated" 2> "$outdir/count"

    if [ $? -ne 0 ]; then
        hits=$(cat "$outdir/count" 2>/dev/null)
        echo "STALE      $name — its FIND block matches $hits times in $target"
        echo "           re-record it against the current source"
        broken=$((broken + 1))
        continue
    fi

    mutatedFile="$target"
    cp "$outdir/mutated" "$target"

    sh run_tests.sh "$test" > "$outdir/run" 2>&1
    status=$?

    restore

    if grep -q "BUILD FAIL" "$outdir/run"; then
        echo "BROKEN     $name — the mutation does not compile, so it proves nothing"
        sed -n '/BUILD FAIL/,$p' "$outdir/run" | head -5 | sed 's/^/           /'
        broken=$((broken + 1))
    elif [ "$status" -eq 0 ]; then
        echo "SURVIVED   $name"
        echo "           $what"
        echo "           $test passed with the defect in place. The check that"
        echo "           should have failed has stopped biting."
        survived=$((survived + 1))
    else
        echo "caught     $name  ($test)"
        caught=$((caught + 1))
    fi
done

echo

if [ "$total" -eq 0 ]; then
    echo "no mutations matched${FILTER:+ $FILTER}"
    exit 0
fi

echo "$total mutations: $caught caught, $survived survived, $broken not applied"

failures=$((survived + broken))

if [ "$failures" -eq 0 ]; then
    echo "every mutation was caught"
else
    echo "artifacts in $outdir"
fi

[ "$failures" -gt 125 ] && failures=125
exit "$failures"
