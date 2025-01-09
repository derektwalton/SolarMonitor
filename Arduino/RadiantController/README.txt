1/9/25

I've found that this project can be compiled, downloaded, and flashed using the Arduino
IDE, which is good because I am no longer able to get the Atmel tool chain and debugger to
work.  I am currently using v1.8.19 of the Arduino IDE on Windows (download does not work
reliably from Ubuntu).  And, with the cheap Chinese Nanos which use CH341 chip for
USB to serial converter, it is necessary to keep the serial monitor OPEN when attempting
to download / program.


Some additional special considerations:

1.  This project makes use of floating point formats in fprintf/sscanf which are NOT
supported by default avr-libc.  To enable this, you must modified an Arduino platform file
which is located for me at

C:\Users\derek\Documents\ArduinoData\packages\arduino\hardware\avr\1.8.6\platform.txt

to include the following line to adjust linker options:

compiler.ldflags=-lprintf_flt  -Wl,-u,vfprintf  -lm  -Wl,-u,vfscanf  -lscanf_flt


2.  The radiant controller PCB design uses the Arduino nano UART0 to RX data from the
thermal controller and to TX data to the UDOO Solar Monitor via the USB serial converter
on the Nano.  HOWEVER, programming of the Arduino occurs over this same UART and to get
it to work correctly we MUST DISCONNECT the thermal controller connection to the Nano
RX line so that this line can be driver by the USB serial converter from the UDOO.  There
is a jumper which can be used to make / break this connection.


3.  The version of Arduino included in the UDOO image does not successfully download and
support Arduino Nano (it will try but gets the wrong binaries ... wrong architecture).
However, a "sudo apt-get install avrdude" will get a proper architecture version of the
avrdude program.  The bash script ./download uses this to be able to program the Nano
from the UDOO board.

To perform the Nano build, use the Windows Arduino IDE and do "Export compiled binary"
under the sketch menu.
