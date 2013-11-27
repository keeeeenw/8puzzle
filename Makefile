PROG = game
MPICC = mpicc
CC = gcc
DFLAG = -DDEBUG #include this for debugging output
OMPFLAG = -openmp
LFLAGS = -o $(PROG)
XWIN = -DX_DISPLAY -L/usr/X11R6/lib -lX11

$(PROG).c-serial: $(PROG).c
	$(CC) $(LFLAGS).c-serial $(PROG).c

clean:
	rm -f $(PROG) a.out *~ *# *.o

