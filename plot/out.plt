set term png size 1200,800
set xdata time
set timefmt "%Y-%m-%dT%H:%M:%SZ"
set xrange ["2025-01-04T00:00:00Z":"2025-01-06T00:00:00Z"]
set key autotitle columnhead
set grid
set multiplot layout 2,2 columnsfirst scale 1,1
set yrange [0:100]
set ylabel 'Degrees F'
set label 1 "2025-01-04T00:00:00Z thru 2025-01-06T00:00:00Z" at graph 0.025, graph 0.975
set label 2 "Total propane gallons  0.0 (avg  0.0 per day)" at graph 0.025, graph 0.94
plot 'out.txt' using 1:(column("T1st")-3) with lines, \
     '' using 1:(column("T2nd")-3) with lines, \
     '' using 1:(column("Troot")-3) with lines, \
     '' using 1:'Tbas' with lines, \
     '' using 1:'Tout' with lines, \
     '' using 1:'Tcondo' with lines, \
     '' using 1:(3+3*(column("H1st"))) with lines, \
     '' using 1:(9+3*(column("H2nd"))) with lines, \
     '' using 1:(15+3*(column("Hcondo"))) with lines
unset label 2
set yrange [0:250]
set ylabel 'Degrees F'
plot 'out.txt' using 1:'Tcoll' with lines, \
     '' using 1:'Tstor' with lines, \
     '' using 1:'TXchI' with lines, \
     '' using 1:'TXchM' with lines, \
     '' using 1:'TXchO' with lines, \
     '' using 1:(5+5*(column("circSolar"))) with lines, \
     '' using 1:(15+5*(column("finnedDump"))) with lines
set yrange [0:200]
set ylabel 'Degrees F'
plot 'out.txt' using 1:'Tstor' with lines, \
     '' using 1:'THotO' with lines, \
     '' using 1:'TRadR' with lines, \
     '' using 1:'THtbR' with lines, \
     '' using 1:(4+4*(column("circRadiant"))) with lines, \
     '' using 1:(12+4*(column("circXchg"))) with lines     
set yrange [0:5]
set ylabel 'KWatt'
set label 2 "Total  0.0 KWhour (avg  0.0 per day)" at graph 0.025, graph 0.94
plot 'out.txt' using 1:(column("PVacPower"))/1000 with lines
unset label 1
unset label 2
unset multiplot
