***Portable C implementation of Moody's method (Union Jack) for surface
plate calibration***  

This directory contains a simple program which computes the height of
a surface plate, starting with angular measurements from an
auto-collimator or tilt meter (clinometer).  
    
The method makes use of eight lines of measurement: two diagonals,
four perimeter lines, and two center lines. This is often called the
"Union Jack" method because the lines look like those on the UK flag.
The method was described by Hume in his book "Engineering Metrology"
around 1950, and then was published in more detail in a paper by
Moody, "How to calibrate surface plates in the plant'', which was
published in "The Tool Engineer" in October 1955.  

- **README.txt** has instructions about how to compile and run this code. These are reproduced below.

- Moody's 1955 paper contains a number of typographical errors and omissions.
These are documented in a short paper which is included here, called
**MoodyCorrections.pdf** .

To use this code follow the steps below.  

1.  Compile the code if it has not already been done, with:  
  **cc -o moody moody.c -lm**  
  This creates an executable called **moody** . 

3.  Enter the measured angular deviations in arc-seconds into the eight
    text files:  
    **NE_SE.txt SE_SW.txt NE_NW.txt NW_SW.txt**    
      **NW_SE.txt NE_SW.txt**   
      **N_S.txt E_W.txt**    
      Here N=North, S=South, E=East and W=West.  The first four files are perimeters, the next two files are diagonals, and the last two files are center lines.

4.  In these text files, put one data value per line. Blank or empty lines are ignored. For
    example: "-12.3" means -12.3 arcseconds. Lines beginning with "#"
    are treated as comments and ignored. See **MoodyCorrections.pdf** for
    information about the sign and direction conventions. 

5.  A set of files containing Moody's original data from his paper may
    be found in the subdirectory **Moody_data/** .   To see how the program
    works, go into that directory and then run the moody executable
    within that directory. 

6.  To use this program for your own data:

    - Create another subdirectory. Put your eight data files there, and
      copy the **Config.txt** configuration file there.

    - You should modify the **Config.txt** to put in the correct foot spacing for your measurements. **Config.txt** has one line with either "I"
      or "M" (to indicate inch or metric units) followed by a blank
      space and then the foot spacing in inches or mm respectively.
      White space and lines beginning with "#" are treated as comments
      and ignored.

    - The included **Config.txt** file is set up to agree with Moody's paper
      (a 4-inch foot spacing).

    - Run the program. On a Linux system or on a Mac terminal:  
**../moody**

    - The main output (Moody's eight tables) will be printed on the
      terminal. Some commentary of interest is printed before the
      tables.

    - Two text files are created, that can be used to plot the
      output: **gnuplot.dat** and **gnuplot.cmd** .

    - If **gnuplot** is installed on your computer, the command:  
**gnuplot gnuplot.cmd**  
 should create a surface plot of your plate, like that shown below. You may have to edit the first
      line of **gnuplot.cmd** to set the correct terminal type for your
      computer. If unsure, comment out the first non-comment line in
      **gnuplot.cmd** by adding a "#" character at the start of the line.  


        ![Plot showing deviations in surface plate, produced by moody method/code.](/Moody_data/gnuplot.jpg)
        
