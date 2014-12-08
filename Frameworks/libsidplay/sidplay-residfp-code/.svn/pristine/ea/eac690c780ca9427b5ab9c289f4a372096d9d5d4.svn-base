set terminal png truecolor small size 1024,1024 \
    xffffff x000000 x404040 \
    xff0000 x00ff00 x0000ff \
    xbb00bb xbbbb00 x00bbbb \
    x888888 x880000 x008800 \
    x000088 x660066
set output "curves.png"
set logscale y
set yrange [170:24000]
set xrange [0:2300]
set label "Follin-style" at 160,2000 
set label "Galway-style" at 590,2000 
set label "Average 6581" at 1050,2000 
set label "Strong filter" at 1500,2000 
set label "Extreme filter" at 1800,2000 
plot \
    "Trurl_Ext/6581R3_4885.txt"  with lines title "r3 4885 trurl",      \
    "Trurl_Ext/6581R3_0486S.txt" with lines title "r3 0486S trurl",     \
    "Trurl_Ext/6581R4AR_4486_unreliable.txt" with lines title "r4ar 4486 trurl", \
    "resid/r2.txt"               with lines title "r? resid", \
    "nata/r2.txt"                with lines title "r2 2083 nata", \
    "Trurl_Ext/6581_3384.txt"    with lines title "r2 3384 trurl",      \
    "alankila/6581R2_3984_2.txt" with lines title "r2 3984 alan (2)",   \
    "alankila/6581R2_3984_1.txt" with lines title "r2 3984 alan (1)",   \
    "ZrX-oMs/6581R4AR_2286.txt"  with lines title "r4ar 2286 zrx",      \
    "Trurl_Ext/6581R4AR_3789.txt" with lines title "r4ar 3789 trurl",   \
    "ZrX-oMs/6581R3_3985.txt"    with lines title "r3 3985 zrx",        \
    "ZrX-oMs/6581R2_0384.txt"    with lines title "r2 0384 zrx",        \
    "Trurl_Ext/6581_0784.txt"    with lines title "r2 0784 trurl",      \
    "alankila/6581R4AR_3789.txt" with lines title "r4ar 3789 alan",     \
    "lord_nightmare/r3-4285.txt" with lines title "r3 4285 lordn",      \
    "lord_nightmare/r3-6581r3-4485-redone.txt" with lines title "r3 4485 lordn",       \
    "lord_nightmare/r4-1986s.txt" with lines title "r4 1986S lordn",     \
    "ZrX-oMs/6581R2_1984.txt"    with lines title "r2 1984 zrx",        \
    "ZrX-oMs/6581R2_3684.txt"    with lines title "r2 3684 zrx",        \
"nosuch"
    
