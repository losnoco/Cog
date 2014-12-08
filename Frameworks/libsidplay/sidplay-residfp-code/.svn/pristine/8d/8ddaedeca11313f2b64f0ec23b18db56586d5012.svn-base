#!/bin/bash
set -e

if [[ $# != 3 ]]; then
    echo "Usage: $0 lowpassfile highpassfile outputfile"
    exit 1
fi

LP=$1
HP=$2
OUT=$3
SCRIPTDIR=$(dirname $0)
TMPDIR=$(mktemp -d)

# split to pieces... need 256 pieces output
echo "Analysing $LP (should be lowpass file)"
$SCRIPTDIR/split-by-silence.py "$LP" $TMPDIR/lp
echo "Analysing $HP (should be highpass file)"
$SCRIPTDIR/split-by-silence.py "$HP" $TMPDIR/hp

# make FFT analysis
echo "Building FFTs. (This will take a long time.)"
for i in $TMPDIR/*.wav; do
    echo "$i -> $i.fft"
    $SCRIPTDIR/fft.py "$i" 2048 > "$i.fft"
done

# run analysis for the whole damn thing
echo "Analysing FFT curve crossing points..."
$SCRIPTDIR/find-center-freq.py "$TMPDIR" > "$OUT"

echo "Analysis complete, output written to $OUT"
