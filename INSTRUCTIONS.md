/*
INSTRUCTIONS.md

Copyright (C) 2015 Juha Aaltonen

This file is part of standalone gdb stub for Raspberry Pi 2B.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

------------------
# general

In the text I'll use the word 'stub' for this program and the word
'debuggee' for a program being debugged.

This program is a bare metal standalone gdb stub for Raspberry Pi 2B.
That is, the idea is that stub is installed onto the SD card as
'kernel7.img' to be booted. The gdb is then used for the debugging
and loading debuggees onto the Raspberry Pi 2B.

This way there is no need to constant insert/removal of the SD card
to/from Raspberry Pi 2B.

## Limitations:
- Thumb instructions not supported. They can be used, but setting
  breakpoints and single-stepping don't work with thumb instructions.
- Only single core is supported.
- Single stepping partly untested.
- No hardware breakpoints.
- No conditional breakpoints/watchpoints.
- The stub reserves the UART0 to itself for communicating with gdb.
  (See Readme.txt about UART0 interrupt configuration options.)
- The stub runs in addresses 0x1f000000 upwards leaving the
  lower memory for the debuggee.
- All gdb-versions can't be told to show the Neon registers. If that is
  required, a gdb-version that supports xml target definitions must be
  used.

## The stub supports:
- Both hex and binary data transfer.
- Software interrupts.
- Software breakpoints.
- Hardware watchpoints.
- Control-C handling.
- Some cmdline-parameters (See Readme.txt)

# INSTALLATION:

## From sources:

For now, the stub is being developed using the "official" Raspberry
Pi / Raspbian tool chain (gcc-linaro-arm-linux-gnueabihf-raspbian)
and Eclipse (Kepler SR2) with CDT.
The tool chain can be found:
https://github.com/raspberrypi/tools/tree/master/arm-bcm2708
and the eclipse:
https://eclipse.org/downloads/packages/release/Kepler/SR2

The rpi_stub can be compiled using the makefile in the 'Debug' subdirectory

## Using image file:

- Format an SD card as FAT32
- Download the files bootcode.bin, cmdline.txt, config.txt and start.elf
  from https://github.com/raspberrypi/firmware/tree/master/boot .
- Copy the downloaded files onto the SD card.
- Copy the file 'kernel7.img' from the 'Debug' folder onto the SD card.
- To update the program, copy a new version of kernel7.img file
  (from the Debug directory, if you have compiled it, or from the Debug
  directory in the github) on top of the kernel7.img file on the SD card.

# USAGE:

## The typical way to use the stub:

- Connect a serial cable between the Raspberry Pi 2B and your
  development computer. Raspberry Pi GPIO-pins 13, 14 and 15 are
  used for the serial adapter.
  NOTE: 3.3V adapter must be used.
  ( https://www.raspberrypi.org/documentation/usage/gpio/README.md )
- Boot the Raspberry Pi with the SD card containing the image.
- Start the gdb on the debugging host
- In gdb, give the commands:
```
	set architecture arm # if you use gdb-multiarch
	set serial baud 115200
	target remote /dev/ttyUSB0 # or which ever serial device you use
	file my_program/Debug/my_program.elf # select the program to debug
	load my_program/Debug/my_program.elf # load your program
	break main # set breakpoint to the function 'main' in your program
	cont # start your program (not 'run')
	info register # look at the registers when the breakpoint is hit
	info all-register # shows also Neon registers (see Restrictions)
	cont # continue running
	{ Control-C to stop the program }
	info register # look at the registers
	detach # to stop debugging, leave the debuggee running
	quit # to quit gdb
```
- You can also put lines
```
	set architecture arm # if you use gdb-multiarch
	set serial baud 115200
	target remote /dev/ttyUSB0 # or which ever serial device you use
	file my_program/Debug/my_program.elf # select the program to debug
	load my_program/Debug/my_program.elf # load your program
	break main # set breakpoint to the function 'main' in your program
```
  in a command file, say, startup.txt in the directory where the .elf is,
  and after starting gdb, give command
```
source startup.txt
```
  and continue manually from 'cont'
  
# USING DDD:

You can also use ddd:
- create a text file (say, "script.gdb") in the directory where your .elf
  of your program is.
- put the following lines in the file:
```
set architecture arm # if you use gdb-multiarch. It doesn't harm with other gdbs either
set serial baud 115200
target remote /dev/ttyUSB0 # or which ever serial device you use
```
- start ddd in the directory where the .elf and the script file is with command:
```
ddd --debugger "arm-none-eabi-gdb -x script.gdb" # or which ever gdb you have
```

- If ddd refuses to start, delete the ddd-sessions directory:
```
rm -rf ~/.ddd
```

You can also put the loading into the gdb-script by adding:
```
	file my_program.elf # select the program to debug (reads symbols etc.)
	load my_program.elf # load your program onto RPi 2B
```
	


