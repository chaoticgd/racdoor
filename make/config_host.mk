# Toolchain
CC = gcc
CXX = g++
AR = ar

# Racdoor tools
RACDOOR =
TBLGEN =

INCLUDES += -I$(PROJECTDIR)/include

COMPILEFLAGS += -std=c99 -O2
