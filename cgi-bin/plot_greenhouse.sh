#!/bin/bash

echo "Content-type: text/html"
echo ''
cd /home/udooer/plot
./plot -greenhouse > /dev/null
gnuplot < out.plt
