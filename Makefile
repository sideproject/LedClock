GCCFLAGS=-g -Os -Wall -mmcu=atmega168 
LINKFLAGS=-Wl,-u,vfprintf -lprintf_flt -Wl,-u,vfscanf -lscanf_flt -lm
AVRDUDEFLAGS=-c avr109 -p m168 -b 115200 -P COM4
LINKOBJECTS=../libnerdkits/delay.o ../libnerdkits/lcd.o ../libnerdkits/uart.o

all:	oneledclock-upload

oneledclock.hex:	oneledclock.c
	make -C ../libnerdkits
	avr-gcc ${GCCFLAGS} ${LINKFLAGS} -o oneledclock.o oneledclock.c ${LINKOBJECTS}
	avr-objcopy -j .text -O ihex oneledclock.o oneledclock.hex
	
oneledclock.ass:	oneledclock.hex
	avr-objdump -S -d oneledclock.o > oneledclock.ass
	
oneledclock-upload:	oneledclock.hex
	avrdude ${AVRDUDEFLAGS} -U flash:w:oneledclock.hex:a

clean:
	rm *.o
	rm *.hex
