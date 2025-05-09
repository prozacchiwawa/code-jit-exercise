OBJS=cpu.o instructions.o memory.o translation.o tx.o util.o
OUTPUT=tx
%.o: %.cpp
	g++ -g -c -o $@ $<

all: $(OUTPUT)

clean:
	rm -f $(OUTPUT) $(OBJS)

$(OUTPUT): $(OBJS)
	g++ -g -o $@ $^
