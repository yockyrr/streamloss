client:		client.o encdec.o utils.o wavein.o
		cc -g -Og -o client.exe client.o encdec.o wavein.o utils.o opus/.libs/libopus.a -lwinmm

client.o: 	client.c encdec.o utils.o wavein.o
		cc -g -Og -c client.c

wavein.o:	wavein.c utils.o
		cc -g -Og -c wavein.c

encdec.o:	encdec.c utils.o
		cc -g -Og -c encdec.c

utils.o:	utils.c
		cc -g -Og -c utils.c

	
