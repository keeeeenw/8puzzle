PROG = game
MPICC = mpic++
CC = g++
DFLAG = -DDEBUG #include this for debugging output
OMPFLAG = -openmp
LFLAGS = -o $(PROG)
XWIN = -DX_DISPLAY -L/usr/X11R6/lib -lX11

$(PROG): $(PROG).cpp
	$(CC) $(LFLAGS) $(PROG).cpp

clean:
	rm -f $(PROG) a.out *~ *# *.o

