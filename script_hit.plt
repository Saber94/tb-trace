set term png truecolor
set style fill transparent solid 0.5 noborder
set grid
unset key
set autoscale
set title "cache hit ratio"
set xlabel "flush times"
set ylabel "hit ratio"
set yrange [0:1]
set xrange [0:50]
set output "~/Documents/tb-trace/hit_out.png"
plot '~/Documents/tb-trace/hit_ratio.dat' with boxes

