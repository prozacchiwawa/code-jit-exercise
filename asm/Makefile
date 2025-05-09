all: instrs.o

instr.o: instrs.s
	gcc -c -o $@ $<

clean:
	rm -f instr.o
