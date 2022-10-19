This software is licenced under GNU.

CellScop is simple program which simulate cellular automat, a bit similar
to caleidoscope. The cells are simulated on 1/8 of squre. The remaining 7/8
are copied. The cells has binary state 0 or 1. State of cell in next step is
determined on sum of 1 in 3x3 array of cells. The sum can be 0..9. The rule
for automat says, which sums will be 1 and which will be zeros. Small number
of options can be set. For example foreground and bacground color.
It is posible to perform single step, run steps with displaying, without
displaying (much faster), invert all cells, invert center cell. And perform
duality (mirror cells around diagonal). It is possible to export state of
automat into bmp file. Size can be set only from command line.
In the directory src are sources compilable with mingw.

cellscop [-s size] [-r rule] [-0 color0] [-1 color1]
  -s size  : set half of window (for example: -s 127)
  -r rule  : rule, sequence of digits for one in next step (-r123789)
  -m       : mirror mode (-r 4567)
  -0 color : set color 0, #rrggbb or #rgb or decimal or 0xrrggbb (-0 0)
  -1 color : set color 1 (-1 #44ccff)
  -i       : swap colors 0 and 1
  -2 color : set fill color (-2 #ff0, -3 set 2nd gradient color)
  -p sleep : sleep in milliseconds
  -f       : fullscreen

Good rules : -r123459 -r1357 -r1234567 -r123 -r1235 -r9870

Left Click : start automat / disable or enable window update  (Enter)
Right Click: stop automat / one step  (Space or + on numeric)
R          : change rule
0-9        : switch rule (reset and set with Ctrl, when rulebar hidden)
M          : mirror mode (-r 4567)
I          : invert all cells (or / on numeric, -r 1)
E          : invert central cell
X          : randomize (or - on numeric, -r4567 -r36789 -r56789)
D          : duality
W          : write state image bmp
Ctrl+C     : copy state image to clipboard
C,V,B      : change fill/gradient,foreground,background color
Ctrl+I     : swap foreground and background color (-r123456)
N          : swap gradient colors (Ctrl invert them)
F          : screen gradient flood fill (Shift for all octets,Ctrl white only)
Caps Lock  : fill also diagonal (all 8 directions)
K          : screen boundary flood fill (Shift for all octets,Ctrl white only)
O          : screen boundary flood fill color replace
L          : screen white flood fill (Ctrl for black,Shift for all octets)
G          : screen gradient color replace (Shift - replace white,Ctrl - Black)
J          : screen white color replace (Shift - replace white,Ctrl - Black)
P          : expand black (Shift - just boundary from black,Ctrl - 8 directions)
T          : screen invert (Shift|Ctrl - fullscren border)
U          : screen rotate colors (Shift - back,Ctrl - rgb to cmy)
F11        : fullscreen (or Ctrl|Alt+Enter)
<F1>,H     : this help
it is all

   Popelka Pavel, http://popelkapavel.github.io/cellscop/cellscop.htm

