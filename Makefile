all: test.exe

test.exe: main.o sender.o
	g++ -L$(SYSTEMC_HOME)/lib-linux64 main.o sender.o -lsystemc -o test.exe

main.o: main.cpp Config.h sender.h
	g++ -c -I$(SYSTEMC_HOME)/include main.cpp

sender.o : sender.cpp sender.h Config.h RRAM_MNIST.h
	g++ -c -I$(SYSTEMC_HOME)/include sender.cpp

run:
	./test.exe

clean:
	rm ./*.o ./test.exe ./*.vcd ./*.log*
