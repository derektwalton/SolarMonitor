#!/bin/bash

echo "Content-type: text/html"
echo ''
cd /home/udooer/plot
./plot > /dev/null
gnuplot < out.plt
