set title "Minor PageFault"
set xlabel "File Size"
set ylabel "No of pagefaults"
plot 'output.txt' using 1:3 with lines