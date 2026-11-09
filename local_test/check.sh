#!/bin/bash
# Check a lab kernel on the Mac before sending it to the server.
#
# Usage:
#   ./check.sh <path to kernel .cpp> [SIZE]
#
# Example:
#   ./check.sh ../lab3/Lab3/src/pl_v3.cpp
#   ./check.sh ../lab3/Lab3/src/pl_v4.cpp 256
#
# Step 1: does it compile?
# Step 2: does it give the right answer? (compares with a plain CPU matrix multiply)

KERNEL="$1"
SIZE="${2:-512}"
HERE="$(cd "$(dirname "$0")" && pwd)"
OUT="$HERE/build"

if [ -z "$KERNEL" ] || [ ! -f "$KERNEL" ]; then
    echo "Usage: ./check.sh <kernel.cpp> [SIZE]"
    exit 1
fi

mkdir -p "$OUT"
FLAGS="-std=c++17 -O2 -Wall -Wno-unknown-pragmas -Wno-unused-variable -Wno-unused-label -I$HERE/include"

echo "=== Step 1: compile $KERNEL ==="
if ! clang++ $FLAGS -c "$KERNEL" -o "$OUT/kernel.o"; then
    echo ""
    echo "COMPILE FAILED. Fix the errors above before uploading."
    exit 1
fi
echo "Compile OK."
echo ""

echo "=== Step 2: run the kernel and check the answer ==="
clang++ $FLAGS -DSIZE="$SIZE" "$HERE/tb_mm.cpp" "$OUT/kernel.o" -o "$OUT/tb" || exit 1
"$OUT/tb"
STATUS=$?
echo ""
if [ $STATUS -eq 0 ]; then
    echo "ALL LOCAL CHECKS PASSED. Safe to upload and build on the server."
else
    echo "LOCAL CHECK FAILED (exit code $STATUS). Do not upload yet."
fi
exit $STATUS
