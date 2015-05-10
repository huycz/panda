#read variables from command line and environment variables
ifeq (X$(showcommand), Xy)
	SILENCE=
else
	SILENCE=@
endif


PANDA_LIBRARY=libpanda.a
PANDA_SERVER=panda_server
PANDA_BALANCER=panda_balancer


INC_DIR=-I$(PANDA_BUILD_TOP)/inc
LIB_DIR=-L$(PANDA_BUILD_TOP)/target/lib
LIBS=-liniparser -lst -ldl
CXXFLAGS= -O2 -Wall -rdynamic -Wformat-security -g
ARFLAGS=-rv


rebuild : clean all


all : dep $(PANDA_LIBRARY) $(PANDA_SERVER) $(PANDA_BALANCER) release


%.o: %.cpp
	$(SILENCE)$(PANDA_CXX) $(CXXFLAGS) -c $< $(INC_DIR) $(LIB_DIR) $(LIBS)

$(PANDA_LIBRARY) : panda_debug.o panda_procedure.o
	$(SILENCE)$(PANDA_AR) $(ARFLAGS) $@ $^

$(PANDA_SERVER) : panda_server.o panda_debug.o panda_procedure.o
	$(SILENCE)$(PANDA_CXX) $(CXXFLAGS) -o $@ $^ $(INC_DIR) $(LIB_DIR) $(LIBS)


$(PANDA_BALANCER) :
	@echo "not ready yet"


clean :
	$(SILENCE)rm -rf *.o
	$(SILENCE)rm -rf $(PANDA_LIBRARY) $(PANDA_SERVER) $(PANDA_BALANCER)
	$(SILENCE)echo "" > Makefile.dep


release :
	$(SILENCE)cp $(PANDA_LIBRARY) $(PANDA_BUILD_TOP)/target/lib
	$(SILENCE)cp $(PANDA_SERVER) $(PANDA_BUILD_TOP)/target/bin

#generate dependence file
#let the compiler do the hard work. XD
include Makefile.dep
.PHONY : dep
dep :
	$(SILENCE)$(PANDA_CXX) -MM *.cpp  $(INC_DIR) > Makefile.dep

