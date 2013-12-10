PROG = game
MPICC = mpic++
CC = g++
DFLAG = -DDEBUG #include this for debugging output
OMPFLAG = -openmp
LFLAGS = -o $(PROG)
MPILFLAGS = -o para-$(PROG)
XWIN = -DX_DISPLAY -L/usr/X11R6/lib -lX11

$(PROG): $(PROG).cpp
	$(CC) $(LFLAGS) $(PROG).cpp

$(PROG)_omp: $(PROG)_omp.cpp
	$(CC) $(OMPFLAG) $(LFLAGS)_omp $(PROG)_omp.cpp

$para-(PROG): para-$(PROG).cpp
	$(MPICC) $(MPILFLAGS) para-$(PROG).cpp
	
$(PROG)_mpi: $(PROG)_mpi.cpp
	$(MPICC) $(LFLAGS)_mpi $(PROG)_mpi.cpp

all:
	$(CC) $(LFLAGS) $(PROG).cpp
	$(CC) $(OMPFLAG) $(LFLAGS)_omp $(PROG)_omp.cpp
	$(MPICC) $(MPILFLAGS) para-$(PROG).cpp
	$(MPICC) $(LFLAGS)_mpi $(PROG)_mpi.cpp

clean:
	rm -f $(PROG) a.out *~ *# *.o

