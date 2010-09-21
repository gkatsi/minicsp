MTL       = ../mtl
CORE      = ../core
CHDRS     = $(wildcard *.hpp) $(wildcard $(MTL)/*.h)
CFLAGS    = -I$(MTL) -I$(CORE) -Wall -ffloat-store
LFLAGS    = -lz

COREOBJS  = $(CORE)/solver.o $(CORE)/cons.o
CSRCS     = $(wildcard *.cpp)
COBJS     = $(COREOBJS)

EXECS     = $(basename $(CSRCS))
EXECSp    = $(addsuffix _profile, $(basename $(CSRCS)))
EXECSd    = $(addsuffix _debug, $(basename $(CSRCS)))
EXECSr    = $(addsuffix _release, $(basename $(CSRCS)))
EXECSrs   = $(addsuffix _static, $(basename $(CSRCS)))
EXECSc    = $(addsuffix _cov, $(basename $(CSRCS)))

PCOBJS     = $(addsuffix p,  $(COBJS))
DCOBJS     = $(addsuffix d,  $(COBJS))
RCOBJS     = $(addsuffix r,  $(COBJS))
CCOBJS     = $(addsuffix c,  $(COBJS))

CXX       ?= g++
CFLAGS    ?= -Wall
LFLAGS    ?= -Wall

COPTIMIZE ?= -O3

GOOGLE_PROFILER ?= -lprofiler

.PHONY : s p d r rs c clean 

s:         $(EXECS)
p:         $(EXECSp)
d:         $(EXECSd)
r:         $(EXECSr)
rs:        $(EXECSrs)
c:         $(EXECSc)

## Compile options
%.o:			CFLAGS +=$(COPTIMIZE) -ggdb -D DEBUG
%.op:			CFLAGS +=$(COPTIMIZE) -pg -ggdb -D NDEBUG
%.od:			CFLAGS +=-O0 -ggdb -D DEBUG -D INVARIANTS
%.or:			CFLAGS +=$(COPTIMIZE) -D NDEBUG
%.oc:                   CFLAGS +=-O0 -fprofile-arcs -ftest-coverage -ggdb -D DEBUG

## Link options
$(EXECS):		LFLAGS := -ggdb $(LFLAGS) $(GOOGLE_PROFILER)
$(EXECSp):	        LFLAGS := -ggdb -pg $(LFLAGS)
$(EXECSd):		LFLAGS := -ggdb $(LFLAGS)
$(EXECSr):	        LFLAGS := $(LFLAGS)
$(EXECSrs):		LFLAGS := --static $(LFLAGS)
$(EXECSc):              LFLAGS := -ggdb -fprofile-arcs -ftest-coverage $(LFLAGS)

# I know nothing about makefiles, i have no idea why these are needed
$(EXECS): $(COBJS)
$(EXECSp): $(PCOBJS)
$(EXECSd): $(DCOBJS)
$(EXECSr): $(RCOBJS)
$(EXECSrs): $(RCOBJS)
$(EXECSc): $(CCOBJS)

## Dependencies
%:              %.o $(COBJS)
	@echo Linking: "$@ ( $^ )"
	@$(CXX) $^ $(LFLAGS) -o $@

%_profile:	%.op $(PCOBJS)
	@echo Linking: "$@ ( $^ )"
	@$(CXX) $^ $(LFLAGS) -o $@

%_debug:	%.od $(DCOBJS)
	@echo Linking: "$@ ( $^ )"
	@$(CXX) $^ $(LFLAGS) -o $@

%_release:	%.or $(RCOBJS)
	@echo Linking: "$@ ( $^ )"
	@$(CXX) $^ $(LFLAGS) -o $@

%_static:	%.or $(RCOBJS)
	@echo Linking: "$@ ( $^ )"
	@$(CXX) $^ $(LFLAGS) -o $@

%_cov:          %.oc $(CCOBJS)
	@echo Linking: "$@ ( $^ )"
	@$(CXX) $^ $(LFLAGS) -o $@

## Build rule
%.o %.op %.od %.or %.oc:	%.cpp
	@echo Compiling: "$@ ( $< )"
	@$(CXX) $(CFLAGS) -c -o $@ $<


clean:
	@rm -f $(EXECS) $(EXECSp) $(EXECSd) $(EXECSr) $(EXECSrs) $(EXECSc) \
	$(COBJS) $(PCOBJS) $(DCOBJS) $(RCOBJS) $(CCOBJS) \
	*gcno *.gcda *.gcov

## Make dependencies
depend.mk: $(CSRCS)
	@echo Making dependencies ...
	@$(CXX) $(CFLAGS) -MM $(CSRCS) > depend.mk
	@cp depend.mk /tmp/depend.mk.tmp
	@sed "s/o:/op:/" /tmp/depend.mk.tmp >> depend.mk
	@sed "s/o:/od:/" /tmp/depend.mk.tmp >> depend.mk
	@sed "s/o:/or:/" /tmp/depend.mk.tmp >> depend.mk
	@rm /tmp/depend.mk.tmp

-include depend.mk
