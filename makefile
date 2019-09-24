CFLAGS  = -W -Wall -lrt

#------------------------------------------------------------------------------
.PHONY : all clean

#------------------------------------------------------------------------------
all : disco 

disco : queue.o ppos_core.o ppos_disk.o hard_disk.o pingpong-disco.o 
	$(CC) -o $@ $^ $(CFLAGS)

extra : queue.o ppos_core.o pingpong-disco.o ppos_disk.o hard_disk.o
	$(CC) -Wextra -o $@ $^ $(CFLAGS) 

GDB : queue.o ppos_core.o pingpong-disco.o ppos_disk.o hard_disk.o
	$(CC) -Wextra -o $@ $^ -g $(CFLAGS)

#------------------------------------------------------------------------------
clean :
	$(RM) GDB disco extra *.o *.out
