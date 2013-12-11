PROG = game
MPICC = mpic++
CC = g++
DFLAG = -g -DDEBUG #include this for debugging output
OMPFLAG = -openmp
LFLAGS = -o $(PROG)
XWIN = -DX_DISPLAY -L/usr/X11R6/lib -lX11

$(PROG): $(PROG).cpp
	$(CC) $(LFLAGS) $(PROG).cpp

$(PROG)_mpi: $(PROG)_mpi.cpp
	$(MPICC) $(LFLAGS)_mpi $(PROG)_mpi.cpp

$(PROG)_mpi_debug: $(PROG)_mpi.cpp
	$(MPICC) $(DDEBUG) $(LFLAGS)_mpi $(PROG)_mpi.cpp

all:
	$(CC) $(LFLAGS) $(PROG).cpp
	$(MPICC) $(LFLAGS)_mpi $(PROG)_mpi.cpp
	$(MPICC) $(DFLAG) $(LFLAGS)_mpi_debug $(PROG)_mpi.cpp

clean:
	rm -f $(PROG) a.out *~ *# *.o

