set term png size 1200,800
set xdata time
set timefmt "%Y-%m-%dT%H:%M:%SZ"
set xrange ["#datetime_start":"#datetime_end"]
set key autotitle columnhead
set grid
set yrange [40:100]
set ylabel 'Degrees F / % Relative Humidity'
set label 1 "#datetime_start thru #datetime_end" at graph 0.025, graph 0.975
plot 'out.txt' using 1:(column("Tsoil")-0) with lines, \
     '' using 1:(column("Tambient")-0) with lines, \
     '' using 1:(column("Hambient")-0) with lines, \
     '' using 1:(column("Tambient2")-0) with lines, \
     '' using 1:(column("Hambient2")-0) with lines, \
     '' using 1:(41+1*(column("soilHeat"))) with lines, \
     '' using 1:(43+1*(column("roomHeat"))) with lines, \
     '' using 1:(45+1*(column("light"))) with lines, \
     '' using 1:(47+1*(column("fan"))) with lines, \
     '' using 1:(49+1*(column("smoke"))) with lines
unset label 1
