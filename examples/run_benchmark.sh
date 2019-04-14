#!/bin/sh

usage()
{
    echo "$0 program threads iter w_iter" >&2
    exit 1
}

### Entry point ##############################################################

[ $# = 4 ] || usage

program="$1"
threads="$2"
iter="$3"
w_iter="$4"
out="${program##*/}.out"

for t in $threads; do
    for w in $w_iter; do
        echo "Running threads=$t iter=$iter w_iter=$w"
        time=`$program $t $iter $w | sed -En 's/time=(.*)/\1/p'`
        echo "$t $iter $w $time" >> $out
    done
done
