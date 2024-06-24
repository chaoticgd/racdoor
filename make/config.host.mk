# Toolchain
CC = gcc
CXX = g++
AR = ar
OBJDUMP = objdump

INCLUDES += -I$(PROJECTDIR)/include

COMPILEFLAGS += -std=c99 -O2 -D_HOST
