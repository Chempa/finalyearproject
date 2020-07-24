CC=g++ -std=c++11
#INC=-I.
LIBS= -pthread  -lpigpio -lr502a  -lrt 

all: main.o
	make --directory ./nfc
	make --directory ./r502
	$(CC) $(LIBS) nfc.o MFRC522.o main.o -o main -lbcm2835

main.o: main.cpp
	$(CC) -c main.cpp

main:
	$(CC) $(LIBS) nfc.o MFRC522.o main.o -o main -lbcm2835

# dae:daemon.cpp nfc/nfc.cpp nfc/MFRC522.cpp
	# $(CC) $(LIBS) -o daemon nfc/nfc.cpp nfc/MFRC522.cpp -I./nfc daemon.cpp -lbcm2835

daemon: daemon.o nfc.o sql.o
	$(CC) $(LIBS) sql.o nfc.o MFRC522.o daemon.o -o daemon -lmysqlcppconn -lbcm2835
nfc.o:
	make --directory ./nfc

sql.o:
	make --directory ./sql

daemon.o: daemon.cpp
	$(CC) -c daemon.cpp

clean:
	rm *.o