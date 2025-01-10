#!/bin/bash

echo "Content-type: text/html"
echo ''
cd /home/udooer/SolarMonitor/plot
./plot -growroom > /dev/null
gnuplot < out.plt
