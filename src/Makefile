# -mwindows

EXE=cellscop.exe
OBJS=main.o resource.o

.SUFFIXES: .rc .ico .bmp

$(EXE):$(OBJS)
	gcc -mwindows -g -O3 -o $(EXE) $(OBJS) -lgdi32 -lcomdlg32


clean:
	rm $(EXE) $(OBJS)

strip:
	strip $(EXE)
	rm $(OBJS)

install:$(EXE)
	strip $(EXE)
	mv $(EXE) ..
	rm $(OBJS)	

%.o: %.c
	gcc -g -Wall \
	  -O9 -funroll-loops -fomit-frame-pointer -fexpensive-optimizations \
	  -mno-cygwin  -c $<

%.o: %.rc
	windres.exe $< -o $@