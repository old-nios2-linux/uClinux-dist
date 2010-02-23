set terminal png large size 800,600
set output "dhrystone.png"


set multiplot

set xdata time          # The x axis data is time
set timefmt "%Y_%b_%d"  # The dates in the file look like 2010_Jan_31
set format x "%b %d"    # On the x-axis, we want tics like Jan 31
set key box
set xtics rotate
set grid
set key out


set size 1,0.5
set origin 0,0.5
set ylabel "Microseconds"
set title "Microseconds for one run through Dhrystone"
plot "dhrystone_data" using 1:2 with linespoints pointtype 4 title "Microseconds"


set size 1,0.5
set origin 0,0
set ylabel "Dhrystones"
set title "Dhrystones per Second"
plot "dhrystone_data" using 1:3 with linespoints pointtype 5 title "Dhrystones"

unset multiplot
