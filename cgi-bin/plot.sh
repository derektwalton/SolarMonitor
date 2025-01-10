#!/bin/bash

echo "Content-type: text/html"
echo ''
cd /home/udooer/SolarMonitor/plot
./plot > /dev/null
gnuplot < out.plt
