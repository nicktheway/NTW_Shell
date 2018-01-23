CC=gcc
CFLAGS=-O3

ntw_%.o: %.c
	$(CC) -c -o $@ $^ $(CFLAGS)

fe_%.o: %.c
	$(CC) -c -o $@ $^ $(CFLAGS) -D __FORCE_EXIT_ON_ERROR

ntw_shell: shell.o
	$(CC) -o $@ $^ $(CFLAGS)

fe_shell: fe_shell.o
	$(CC) -o $@ $^ $(CFLAGS) -D __FORCE_EXIT_ON_ERROR

all: shell.o fe_shell.o
	$(CC) -o ntw_shell $< $(CFLAGS)
	$(CC) -o fe_shell $(filter-out $<,$^) $(CFLAGS) -D __FORCE_EXIT_ON_ERROR

clean:
	rm *.o

clean2: 
	rm *shell