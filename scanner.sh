#!/bin/bash

workingdir="$( cd "$(dirname "$0")" ; pwd -P )"
bindir=${workingdir}/bin


scanner=${bindir}/scanner

command -v ${scanner} >/dev/null 2>&1 || { echo "Ich vermisse " ${scanner} >&2; exit 1; }


${scanner} p 18051 -u 17051 -f 404000000 -s 1500 -v -o sdrcfg-rtl0.txt -b blacklist.txt -q 55 -n 5 &
