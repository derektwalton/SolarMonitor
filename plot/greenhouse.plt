set term png size 1200,800
set xdata time
set timefmt "%Y-%m-%dT%H:%M:%SZ"
set xrange ["#datetime_start":"#datetime_end"]
set key autotitle columnhead
set grid
set yrange [-20:120]
set ylabel 'Degrees F / % Relative Humidity'
set label 1 "#datetime_start thru #datetime_end" at graph 0.025, graph 0.975
plot 'out.txt' using 1:(column("GH_Tambient")-0) with lines, \
     '' using 1:(column("GH_Hambient")-0) with lines, \
     '' using 1:(column("GH_gnd13_T")-0) with lines, \
     '' using 1:(column("GH_gnd3_T")-0) with lines, \
     '' using 1:(column("GH_pool_T")-0) with lines, \
     '' using 1:(column("GH_panel_T")-0) with lines, \
     '' using 1:(column("GH_inner_T")-0) with lines, \
     '' using 1:(column("Tout")-0) with lines, \
     '' using 1:(21+2*(column("GH_pump"))) with lines, \
     '' using 1:(29+2*(column("GH_fan"))) with lines
unset label 1
