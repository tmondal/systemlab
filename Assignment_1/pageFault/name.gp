set title "Major PageFault"
set xlabel "File Size"
set ylabel "No of pagefaults"
plot 'output.txt' using 1:2 with lines