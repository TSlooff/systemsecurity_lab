# Target file name (without extension).
# This is the name of the compiled .hex file.
TARGET = secure-sensor

# List C source files here.
# Header files (.h) are automatically pulled in.
SRC += secure-sensor.c

SS_VER = SS_VER_2_1

#Add simpleserial project to build
include ../simpleserial/Makefile.simpleserial

FIRMWAREPATH = ../.
include $(FIRMWAREPATH)/Makefile.inc

