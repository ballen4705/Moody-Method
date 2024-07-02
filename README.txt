This directory contains a simple program which computes the height of
a surface plate, starting with angular measurements from an
auto-collimator or tilt meter (clinometer).

The method makes use of eight lines of measurement: two diagonal, four
perimeter line, and two center line.  This is often called the "Union
Jack" method because the lines look like those on the flag.  The
method was described by Hume in his book "Engineering metrology"
around 1950, and then was published in more detail in a paper by
Moody, ``How to calibrate surface plates in the plant'', which was
published in "The Tool Engineer" in October 1955.

Moody's paper contains a number of typographical errors and omissions.
These are documented in a short paper which is included here, called
MoodyCorrections.pdf

To use this code follow steps a-e below.


(a) Compile the code if it has not already been done, with:
    cc -o moody moody.c -lm


(b) Enter the measured angular deviations in arc-seconds into the
eight text files:

NW_SE.txt NE_SW.txt (diagonals)
NE_SE.txt SE_SW.txt NE_NW.txt NW_SW.txt (perimeter)
N_S.txt E_W.txt (center lines)

Put one data value per line, with no blank or empty lines. For example
"-12.3" means -12.3 arcseconds. Lines beginning with "#" are treated
as comments and ignored.

A set of files containing Moody's original data from his paper may be
found in the directory Moody_data/.  If you are using this for your
own data, I suggest that you create another directory and put your eight
data files and your Config.txt file there.  Then run "moody" from within
that directory.


(c) Edit the configuration file Config.txt.  It has one line with
either "I" or "M" as the first character (to indicate inch or metric
units) followed by a blank space and then the foot spacing in inches
or mm respectively.  Lines beginning with "#" are treated as comments
and ignored. The Config.txt file included by default is set up to
agree with Moody's paper (a 4-inch foot spacing).


(d) Run the program.  On a Linux system or on a Mac terminal:
./moody

is sufficient.  The main output (Moody's eight tables) will be printed
on the terminal.  There is also some commentary printed before these
tables that may be of interest.


(e) Two text files are created, that can be used to plot the output:
"gnuplot.dat" and "gnuplot.cmd".  If gnuplot is installed on your
computer, the command "gnuplot gnuplot.cmd" should create the plot.
You may have to edit the first line of "gnuplot.cmd" to set the
correct terminal type for your computer.  If unsure, comment out the
first non-comment line in gnuplot.cmd by adding a "#" character at the
start of the line.


This code and documentation are Copyright Bruce Allen 2018-2024.

They may be freely distributed under the GNU General Public License
version 3.  See the file "LICENSE" for details.

