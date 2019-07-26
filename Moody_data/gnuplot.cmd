# The following command file can be used with gnuplot to produce
# a 3-dimensional plot of the surface plate. The associated data
# file is called "gnuplot.dat" and can be found in this directory.

set term X11 enhanced
set xyplane at 0
set label "N" at 8.000000, 11.000000, 0.000000
set label "S" at 8.000000, -1.000000, 0.000000
set label "E" at 17.600000, 5.000000, 0.000000
set label "W" at -1.600000, 5.000000, 0.000000
set zrange [0:18]
set zlabel "height\nin\ntens of\nmicroinch"
set key off
splot [0:16][0:10][0:18] "gnuplot.dat" using 1:2:3 with lines
pause -1
