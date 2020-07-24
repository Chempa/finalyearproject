CC=g++ -Wall
INC=-I.
LIBS=-L./ -pthread  -lr502a -lpigpio  -lrt


all: r502a


r502a: ceSerial.o r502a.o
ifeq ($(OS),Windows_NT)
	$(CC) -shared r502a.o ceSerial.o -o r502a.lib $(INC)
else
	$(CC) -shared r502a.o ceSerial.o -o libr502a.so $(INC)
	sudo ln -f $(PWD)/r502/libr502a.so /usr/lib/libr502a.so
endif

ceSerial.o: ceSerial.cpp
	$(CC) -c ceSerial.cpp -o ceSerial.o

r502a.o: r502a.cpp
	$(CC) -c r502a.cpp -o r502a.o $(INC)

main: main.cpp
	$(CC) main.cpp -o main $(LIBS)  $(INC) 

run:
ifeq ($(OS),Windows_NT)
	./main.exe
else
	sudo ./main
endif

clean:
	rm -rf *.o *.lib *.so main.exe main
	