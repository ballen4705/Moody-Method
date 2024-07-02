/*
 * Moody Surface Plate Analysis
 * Copyright Bruce Allen, 2018-2024
 * 
 * (1) Implementation of the method documented in "How to calibrate a
 * surface plate in the plant", by J.C. Moody, published in The Tool
 * Engineer, October 1955.
 *
 * (2) See the README.txt file for usage instructions.
 *
 * (3) The code is distributed with a sample set of input files that
 * reproduce Moody's worksheets and results.
 *
 * (4) The output of the code is the completed "worksheets" described
 * in Moody's publication above
 *
 * (5) The code also outputs a gnuplot script and gnuplot data file.
 * These can be used to make a 3d plot of the surface plate surface
 * which can be zoomed, panned, rotated, etc.
 *
 * (6) The latest version of this code can be obtained from GITHUB.
 *
 * (7) This program is written in standard C to make it as portable as
 * possible.  I am aware that using GNU libraries would make it
 * simpler and more capable, but portability and compilation
 * simplicity are more important.  This is also why all headers and
 * code are in a single file.
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see
 * <https://www.gnu.org/licenses/>. 
 */

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

/* maximum number of stations along any of the 8 lines */
#define MAX_STATIONS 128

/* maximum number of charcters on any of the input data files */
#define MAX_LINELEN 1024

/* labels for the 8 different worksheets */
/* the two diagonals */
#define NW_SE 0
#define NE_SW 1

/* the four perimeter lines */
#define NE_NW 2
#define NE_SE 3
#define SE_SW 4
#define NW_SW 5

/* the two center lines */
#define E_W 6
#define N_S 7

/* Input data file names */
const char *filenames[]={"NW_SE.txt", "NE_SW.txt",
			 "NE_NW.txt", "NE_SE.txt",
			 "SE_SW.txt", "NW_SW.txt",
			 "E_W.txt", "N_S.txt"};
/* 
 * Main data structure, contains worksheets as done by Moody.
 * First index is which table (0-7)
 * Second index is which column (0-8)
 * Third index is which station (0-max)
 *
 * For center line tables, Moody column 6a is stored in column 6
 * and Moody column 6 is stored in column 9.
 * Other tables do not use column 9.
 * Note that we index from zero, so column 1 has index 0.
*/
float ws[8][9][MAX_STATIONS];

/* Number of input data entries in each of the 8 worksheets */
int num_dat[8];

/* One arc second in radians */
const float arcsec = 2.0*3.141592/(360.0*60*60);

/* Set to 1 for metric, 0 for imperial (inches) */
int metric=-1;

/* Reflector foot spacing in either inches or mm */
float foot_spacing;

/* Reads and parses configuration file */
void read_config_file(void) {
   FILE* fp;
   char buf[MAX_LINELEN];
   char* head;
   int file_line=0;
   const char fname[]="Config.txt";
   char flag;

   /* open file for reading */
   if ((fp = fopen(fname, "r")) == NULL) {
      fprintf(stderr, "Error: unable to find/open input data file %s\n", fname);
      exit(EXIT_FAILURE);
   }
   
   /* read line from file */
   while (fgets(buf, sizeof(buf), fp) != NULL) {   
      
      /* keep track of which line we are on */
      file_line++;
      
      /* guarantee newline at the end */
      buf[sizeof(buf)-1]='\n';
      
      /* move to first non-white-space character */
      head = buf;
      while (isspace(*head) && *head!='\n') head++;

      /* if comment or end of line, skip line */
      if (*head=='\n' || *head=='#') continue;
      
      /* parse foot spacing */
      {
	 char remaining[MAX_LINELEN];
	 int ret = sscanf(head, "%c %f %s\n", &flag, &foot_spacing, remaining);
	    if (ret !=2 || !(flag == 'M' || flag == 'I')) {
	       fprintf(stderr,
		       "Error: unable to parse line %d of data file %s.\n"
		       "Expected is either \"M x\" or \"I x\",\n"
		       "where \"x\" is the foot spacing in mm or inches respectively.\n"
		       "Line %d reads:\n%s\n",
		       file_line, fname, file_line, buf);
	       fprintf(stderr,"Flag is %c ret is %d\n", flag, ret);
	       exit(EXIT_FAILURE);
	    }
	    else {
	       if (flag=='M') {
		  metric=1;
		  printf("From file %s: using a %.2f mm foot spacing.\n\n",
			 fname, foot_spacing);
	       } else {
		  metric=0;
		  printf("From file %s: using a %.2f inch foot spacing.\n\n",
			 fname, foot_spacing);
	       }
	       fclose(fp);
	       return;
	    }
      }
   }
   fprintf(stderr,
	   "Configuration file %s must specify a foot spacing and units.\n"
	   "Examples:\n"
	   "M 66.0\n"
	   "means 66mm foot spacing, and\n"
	   "I 4.0\n"
	   "means 4 inch foot spacing.\n",
	   fname);
   exit(EXIT_FAILURE);
}

void read_data(int which_file) {
   FILE* fp;
   char buf[MAX_LINELEN];
   char* head;
   const char *fname= filenames[which_file];
   int lines_read=0;
   int file_line=0;

   /* open file for reading */
   if ((fp = fopen(fname, "r")) == NULL) {
      fprintf(stderr, "Error: unable to find/open input data file %s\n", fname);
      exit(EXIT_FAILURE);
   }
   
   /* read line from file */
   while (fgets(buf, sizeof(buf), fp) != NULL) {
	 /* keep track of which line we are on */
	 file_line++;

         /* guarantee newline at the end */
	 buf[sizeof(buf)-1]='\n';

	 /* move to first non-white-space character */
	 head = buf;
	 while (isspace(*head) && *head!='\n') head++;

	 /* if comment or end of line, skip line */
	 if (*head=='\n' || *head=='#') continue;
	 	 
	 /* parse number of arcseconds */
	 {
	    char remaining[MAX_LINELEN];
	    int ret = sscanf(head, "%f%s\n", &ws[which_file][1][lines_read+1], remaining);
	    if (ret !=1) {
	       fprintf(stderr,
		       "Error: unable to parse line %d of data file %s.\n"
		       "Expected is an angle in arcseconds.\nLine %d reads:\n%s\n",
		       file_line, fname, file_line, buf);
	       exit(EXIT_FAILURE);
	    }
	    lines_read++;

	    if (lines_read>=MAX_STATIONS-2) {
	       fprintf(stderr,
		       "Error: code can accept a maximum of MAX_STATIONS-2=%d stations,\n"
		       "but file %s contains more stations than this. Recompile code\n"
		       "with a larger value of MAX_STATIONS, then rerun analysis.\n",
		       MAX_STATIONS-2, fname);
	       exit(EXIT_FAILURE);
	    }
	 }
   }
   if (lines_read<3) {
      fprintf(stderr, "Error: read %d data lines from data file %s.\n"
	      "Need at least 3 valid data lines.\n",
	      lines_read, fname);
      exit(EXIT_FAILURE);
   }
   printf("Read %d data entries from %s\n", lines_read, fname);
   
   /* store number of lines read in the array itself */
   num_dat[which_file] = lines_read;
   fclose(fp);
   return;
}


/* printing assumes fixed character width and avoids tabs */ 
void print_table(int which_file) {
   
   const char h1[]=
      "   1       2       3       4       5       6       7       8   \n"
      "---------------------------------------------------------------\n"
      "Station  Auto-   Angle  Sum of   Cumul   Delta   Delta   Delta \n"
      " Num-    Corr    Displ   Displ   Corr    Datum    Base    Base \n"
      " ber    ArcSec  ArcSec  ArcSec   Factor  ArcSec  ArcSec 10^-5in\n"
      "---------------------------------------------------------------\n";

   const char h2[]=
      "   1       2       3       4       5       6       6a      7       8   \n"
      "-----------------------------------------------------------------------\n"
      "Station  Auto-   Angle  Sum of   Cumul   Delta    Error  Delta   Delta \n"
      " Num-    Corr    Displ   Displ   Corr    Datum    Shift   Base    Base \n"
      " ber    ArcSec  ArcSec  ArcSec   Factor  ArcSec    Out   ArcSec 10^-5in\n"
      "-----------------------------------------------------------------------\n";
   
   const char h3[]=
      "   1       2       3       4       5       6       7       8   \n"
      "---------------------------------------------------------------\n"
      "Station  Auto-   Angle  Sum of   Cumul   Delta   Delta   Delta \n"
      " Num-    Corr    Displ   Displ   Corr    Datum    Base    Base \n"
      " ber    ArcSec  ArcSec  ArcSec   Factor  ArcSec  ArcSec  micron\n"
      "---------------------------------------------------------------\n";

   const char h4[]=
      "   1       2       3       4       5       6       6a      7       8   \n"
      "-----------------------------------------------------------------------\n"
      "Station  Auto-   Angle  Sum of   Cumul   Delta    Error  Delta   Delta \n"
      " Num-    Corr    Displ   Displ   Corr    Datum    Shift   Base    Base \n"
      " ber    ArcSec  ArcSec  ArcSec   Factor  ArcSec    Out   ArcSec  micron\n"
      "-----------------------------------------------------------------------\n";

   const char format1[]="%6d";
   const char format2[]="%8.1f";
   
   int i,j;

   const char *header1, *header2;

   if (metric) {
      header1=h3;
      header2=h4;
   } else {
      header1=h1;
      header2=h2;
   }  
   
   printf("\nTABLE %s\n", filenames[which_file]);
   if (which_file<6)
      printf("%s", header1);
   else
      printf("%s", header2);

   for (j=0; j<=num_dat[which_file]; j++) {
      /* station number, Moody column 1 */
      printf(format1, (int)ws[which_file][0][j]);
      /* Moody columns 2 to 6 */
      for (i=1; i<=5; i++) printf(format2, ws[which_file][i][j]);
      /* Moody column 6a for the two center lines */
      if (which_file>5) printf(format2, ws[which_file][8][j]);
      /* Moody columns 7 and 8 */
      for (i=6; i<8; i++) printf(format2, ws[which_file][i][j]);    

      printf("\n");
   }
   return;
   
}

/* Return the "middle value" from a given column of the specified
 * sheet, meaning: If there are an odd number of rows, return the
 * middle one.  If there are an even number of rows, return average of
 * two middle ones.
 */
float mid_value(int which_sheet, int which_column) {
   int ndat = num_dat[which_sheet];
   if (ndat % 2 == 0)
      return ws[which_sheet][which_column][ndat/2];
   else {
      float a = ws[which_sheet][which_column][(ndat-1)/2];
      float b = ws[which_sheet][which_column][(ndat+1)/2];
      return 0.5*(a+b);
   }
}

/* Carry out the "correction factor" jazz for perimeter and center lines */
void shift_lines(int which_sheet) {
   float correction_factor,should_be_zero;
   int j;
   int i=which_sheet;
   int ndat=num_dat[i];
   
   ws[i][4][ndat] = ws[i][5][ndat]-ws[i][3][ndat];
   correction_factor = (ws[i][4][0]-ws[i][4][ndat])/ndat;
   for (j=ndat-1; j>0; j--) {
      ws[i][4][j]=ws[i][4][j+1]+correction_factor;
      ws[i][5][j]=ws[i][4][j]+ws[i][3][j];
   }

   /* do column 6a for center lines only */
   if (which_sheet==6 || which_sheet==7) {
      should_be_zero = mid_value(which_sheet, 5);
      for (j=0; j<=ndat; j++)
	 ws[which_sheet][8][j]=ws[which_sheet][5][j]-should_be_zero;
   }
   return;
}

/* carry out the "cumulative corrections" to the diagonals */
void diagonal_correction(int which_sheet) {
   int j;
   int ndat=num_dat[which_sheet];      
   float a= -1.0*ws[which_sheet][3][ndat]/ndat;
   float b=  0.5*ws[which_sheet][3][ndat]-mid_value(which_sheet,3);      
   for (j=0;j<=ndat; j++) {
      /* column 5 */
      ws[which_sheet][4][j]=a*j+b;
      /* column 6 */
      ws[which_sheet][5][j] = ws[which_sheet][3][j] + ws[which_sheet][4][j];
   }
   return;
}

/* compute the first four columns of the worksheets */
void first_four_columns(int which_sheet) {
      int j;
      int ndat=num_dat[which_sheet];
      /* label stations, Moody column 1 */
      for (j=0; j<=ndat; j++) ws[which_sheet][0][j]=j+1;
      
      /* angular differences, Moody column 3 */
      for (j=1; j<=ndat; j++) ws[which_sheet][2][j]=
				 ws[which_sheet][1][j]-ws[which_sheet][1][1];
      
      /* sum of angular differences, Moody column 4 */
      ws[which_sheet][3][0]=0.0;
      ws[which_sheet][3][1]=0.0;
      for (j=2; j<=ndat; j++) ws[which_sheet][3][j]=
				 ws[which_sheet][3][j-1]+ws[which_sheet][2][j];
      return;
}

/* search column 6 or 6a of all sheets for the min and max value */
void return_low_and_high_point(float *pmin, float *pmax) {
   int i, j;
   float min=ws[0][5][0];
   float max=ws[0][5][0];
   
   /* loop over all worksheets */
   for (i=0; i<8; i++) {
      /* loop over all rows */
      for (j=0; j<=num_dat[i]; j++) {
	 /* select column six, except for center lines select column 6a */
	 float tmp;
	 if (i==6 || i==7) 
	    tmp = ws[i][8][j];
	 else
	    tmp = ws[i][5][j];

	 /* keep track of the smallest and largest values */
	 if (tmp<min) min=tmp;
	 if (tmp>max) max=tmp;
      }
   }
   /* return min and max values */
   *pmin = min;
   *pmax = max;
   return;
}

void do_consistency_checks(void) {

   int i;
   
   if (num_dat[0] != num_dat[1])
      printf("Warning: the number of stations along the %s and %s diagonals\n"
	     "are expected to be the same, but are not.\n", filenames[0], filenames[1]);
   
   for (i=0; i<2; i++) {
      int stat1=2+i;
      int stat2=4+i;
      int stat3=6+i;

      if (
	  (num_dat[stat1] != num_dat[stat2]) ||
	  (num_dat[stat2] != num_dat[stat3]) ||
	  (num_dat[stat1] != num_dat[stat3])
	  )
            printf("Warning: the number of stations along the three lines\n"
		   "%s, %s and %s are expected to be the same, but are not.\n",
		   filenames[stat1], filenames[stat2], filenames[stat3]);
   }
   printf("\n");

   /* Pythagoras check x^2+y^2=z^2 where x,y,z refer to data sets 2,3,0  and 4,5,1 */
   for (i=0; i<2; i++) {
      int x=num_dat[2*i+2];
      int y=num_dat[2*i+3];
      int z=num_dat[i];
	 
      float diag_len = sqrt((float)x*(float)x+(float)y*(float)y);

      if (fabs(diag_len - z) > 1.5) {
	 printf("Warning: the number of stations along the perimeter lines\n"
		"and diagonal lines appears to deviate significantly from\n"
		"Pythagoras' Theorem x^2 + y^2 = z^2 for\n"
		"x = %d, y = %d and z=%d\n",
		x, y, z);
      }
   }
   printf("\n");
   return;
}

void print_license(void) {
   printf("Moody Surface Plate Analysis\n"
	  "Copyright 2018-2024, Bruce Allen\n"
	  "This program comes with ABSOLUTELY NO WARRANTY.\n"
	  "This is free software, and you are welcome to redistribute it\n"
	  "under the conditions of the included GNU General Public License.\n\n"
	  );
   return;
}

int max(int a, int b, int c) {
   int max=a;
   if (b>max) max=b;
   if (c>max) max=c;
   return max;
}


/* output a data file which can be plotted with gnuplot */
void output_gnuplot(float biggest) {
   int i,j;

   FILE *fp;
   char *fname;
   const char *zlabels[2];   
   int max_x = max(num_dat[2], num_dat[4], num_dat[6]);
   int max_y = max(num_dat[3], num_dat[5], num_dat[7]);
   int max_z = (int)(1.0+biggest);
   zlabels[0]="height\\nin\\ntens of\\nmicroinch";
   zlabels[1]="height\\nin\\nmicrons";

   fname="gnuplot.cmd";
   if (!(fp=fopen(fname, "w"))) {
            fprintf(stderr, "Error: unable to open/write output file %s\n", fname);
	    exit(EXIT_FAILURE);
   }

   fprintf(fp,
	   "# The following command file can be used with gnuplot to produce\n"
	   "# a 3-dimensional plot of the surface plate. The associated data\n"
	   "# file is called \"gnuplot.dat\" and can be found in this directory.\n"
	   "#\n"
	   "# On typical Unix/Linux/Mac systems, invoke gnuplot with:\n"
	   "# gnuplot -c gnuplot.cmd\n"
	   "\n"
	   "set term X11 enhanced\n"
	   "set xyplane at 0\n"
	   "set label \"N\" at %f, %f, %f\n"
	   "set label \"S\" at %f, %f, %f\n"
	   "set label \"E\" at %f, %f, %f\n"
	   "set label \"W\" at %f, %f, %f\n"
	   "set zrange [0:%d]\n"
	   "set zlabel \"%s\"\n"
	   "set key off\n"
	   "splot [0:%d][0:%d][0:%d] \"gnuplot.dat\" using 1:2:3 with lines\n"
	   "pause -1\n",
	   0.5*max_x,  1.1*max_y, 0.0,
	   0.5*max_x, -0.1*max_y, 0.0,
	   1.1*max_x, 0.5*max_y,  0.0,
	   -0.1*max_x, 0.5*max_y,  0.0,
	   max_z,
	   zlabels[metric],
	   max_x, max_y, max_z
	   );
   fclose(fp);

   fname="gnuplot.dat";
   if (!(fp=fopen(fname, "w"))) {
      fprintf(stderr, "Error: unable to open/write output file %s\n", fname);
      exit(EXIT_FAILURE);
   }

   fprintf(fp,
	   "# This is a data file for use with gnuplot.\n"
	   "# The corresponding command file in this directory\n"
	   "# is called \"gnuplot.cmd\". Together these can be\n"
	   "# used to generate a 3-d plot of the surface plate height.\n"
	   "\n\n"
	   );
   
   /* now output data, first for the two diagonals */
   for (i=0; i<2; i++) {
      int max=num_dat[i];
      fprintf(fp,"# %s\n", filenames[i]);
      for (j=0; j<=max; j++) {
	 float x[2];
	 float y = max_y*((float)(max-j))/max;
	 x[0]= max_x*((float)j)/max;
	 x[1]= max_x*((float)(max-j))/max;
	 fprintf(fp, "%f %f %f\n", x[i], y, ws[i][7][j]);
      }
      fprintf(fp,"\n\n");
   }

   /* three East to West lines */
   for (i=2; i<=6; i+=2) {
      float y;
      int max = num_dat[i];
      fprintf(fp,"# %s\n", filenames[i]);
      /* the North/South locations of these three lines */
      if (i==2) y=max_y; else if (i==4) y=0; else y=0.5*max_y;
      for (j=0; j<=max; j++) {
	 float x = max_x*((float)(max-j))/max;
	 fprintf(fp, "%f %f %f\n", x, y, ws[i][7][j]);
      }
      fprintf(fp,"\n\n");
   }

   /* three North to South lines */
   for (i=3; i<=7; i+=2) {
      float x;
      int  max = num_dat[i];
      fprintf(fp,"# %s\n", filenames[i]);
      /* the East/West locations of these three lines */
      if (i==3) x=max_x; else if (i==5) x=0; else x=0.5*max_x;
      for (j=0; j<=max; j++) {
	 float y = max_y*((float)(max-j))/max;
	 fprintf(fp, "%f %f %f\n", x, y, ws[i][7][j]);
      }
      fprintf(fp,"\n\n");
   }
   fclose(fp);
   return;
}

void do_moody_consistency_checks(void) {
   int i;
   int printwarning=0;

   /* A couple of consistency checks */
   printf("================================================================\n"
	  "Measurement errors are estimated from the computed\n"
	  "heights at the middle of the two center lines. Absent any\n"
	  "measurement errors, these computed heights would be zero.\n"
	  );

   for (i=6; i<8; i++) {
      float error = mid_value(i, 5)*arcsec*foot_spacing;
      if (metric) {
	 printf("Computed height at the center of the %s line: %4.2f microns.\n",
		filenames[i],error);
	 if (fabs(error)>2.54) printwarning=1;
      } else {
	 printf("Computed height at the center of the %s line: %4.2f micro-inches.\n",
		filenames[i],10*error);
	 if (fabs(error)>10.0) printwarning=1;
      }
   }
   if (printwarning)
      printf("Warning: measurement errors are larger than Moody considers\n"
	     "acceptable (100 micro-inch = 2.54 microns). The job must be done over!\n");
   else
      printf("According to Moody these errors are acceptable, because their\n"
	     "magnitude is less than 100 micro-inch = 2.54 microns.\n");
   printf("================================================================\n");
   return;
}

/* The main routine is structured to follow Moody's recipe closely */
int main(int argc, char *argv[]) {

   int i,j;
   float lowest, highest;

   /* Print out license information */
   print_license();
   
   /* Read configuration file */
   read_config_file();
   
   /* Read data from input files*/
   for (i=0; i<8; i++) read_data(i);
   printf("\n");

   /* Check for consistency of the input data */
   do_consistency_checks();

   /* Step through all eight worksheets, doing first four columns */
   for (i=0; i<8; i++) first_four_columns(i);
   
   /* Moody columns 5 and 6 for diagonal lines */
   for (i=0; i<2; i++) diagonal_correction(i);

   /* Moody columns 5 and 6 for perimeter lines. */
   /* Copy NE corner into worksheets */
   ws[2][4][0] = ws[2][5][0] = ws[3][4][0] = ws[3][5][0] = ws[1][5][0];
   /* Copy SW corner into worksheets */
   ws[4][5][num_dat[4]] = ws[5][5][num_dat[5]] = ws[1][5][num_dat[1]];
   /* Copy NW corner into worksheets */
   ws[2][5][num_dat[2]]= ws[5][4][0]= ws[5][5][0]= ws[0][5][0];
   /* Copy SE corner into worksheets */
   ws[3][5][num_dat[3]] = ws[4][4][0] = ws[4][5][0] = ws[0][5][num_dat[0]];

   /* Perimeter line correction factors */
   for (i=2; i<6; i++) shift_lines(i);
   
   /* Moody columns 5 and 6 for center lines */
   /* Copy midpoints of perimeter E-W (East end) */
   ws[6][4][0] = ws[6][5][0] = mid_value(3,5);
   /* Copy E-W (West end) */
   ws[6][5][num_dat[6]] = mid_value(5,5);
   /* Copy N-S (North end) */
   ws[7][4][0] = ws[7][5][0] = mid_value(2,5);   
   /* Copy N-S (South end) */
   ws[7][5][num_dat[7]] = mid_value(4,5);

   /* Center line correction factors and column 6a */
   for (i=6; i<8; i++) shift_lines(i);

   /* Compute Moody column 7  */
   
   /* For diagonals and perimeter lines */
   return_low_and_high_point(&lowest, &highest);
   for (i=0; i<6; i++)
      for (j=0; j<=num_dat[i]; j++)
	 ws[i][6][j]=ws[i][5][j]-lowest;
   /* and for the center lines */
   for (i=6; i<8; i++)
      for (j=0; j<=num_dat[i]; j++)
	 ws[i][6][j]=ws[i][8][j]-lowest;

   /* To convert from angle to distance */
   if (metric) {
      /* output in microns */
      foot_spacing *= 1000.0;
   } else {
      /* output in 1/100,000 of an inch */
      foot_spacing *= 100000.0;
   }
   
   /* Now fill in column 8 */
   for (i=0; i<8; i++)
      for (j=0; j<=num_dat[i]; j++)
	    ws[i][7][j] = ws[i][6][j]*arcsec*foot_spacing;

   /* Check if the middle of the center lines falls at zero as it should */
   do_moody_consistency_checks();
   
   /* Print out the completed worksheet */
   for (i=0; i<8; i++) print_table(i);

   /* Maximum height over plate, with units */
   highest = (highest-lowest)*arcsec*foot_spacing;
   
   /* Output a surface plot */
   output_gnuplot(highest);
   
   return 0;
}




