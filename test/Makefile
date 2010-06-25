MTL       = ../mtl
CORE      = ../core
CHDRS     = $(wildcard *.h) $(wildcard $(MTL)/*.h)
EXEC      = test
CFLAGS    = -I$(MTL) -I$(CORE) -Wall -ffloat-store
LFLAGS    = -lz

COREOBJS  = $(CORE)/Solver.o $(CORE)/Cons.o
CSRCS     = $(wildcard *.C)
COBJS     = $(addsuffix .o, $(basename $(CSRCS))) $(COREOBJS)

include ../mtl/template.mk
