#read variables from command line and environment variables
ifeq (X$(showcommand), Xy)
	SILENCE=
else
	SILENCE=@
endif


PANDA_LOG_TEST=panda_log_test
PANDA_MODULE_TEST=panda_module_test.so


INC_DIR=-I$(PANDA_BUILD_TOP)/inc
LIB_DIR=-L$(PANDA_BUILD_TOP)/target/lib
LIBS=-liniparser -lst
CXXFLAGS= -fPIC -O2 -Wall -rdynamic -Wformat-security


rebuild : clean all


all : dep $(PANDA_LOG_TEST) $(PANDA_MODULE_TEST)


%.o: %.cpp
	$(SILENCE)$(PANDA_CXX) $(CXXFLAGS) -c $< $(INC_DIR) $(LIB_DIR) $(LIBS)


$(PANDA_LOG_TEST) : panda_log_test.o
	$(SILENCE)$(PANDA_CXX) $(CXXFLAGS) -o $@ $^ $(INC_DIR) $(LIB_DIR) $(LIBS)


$(PANDA_MODULE_TEST) : panda_module_test.o
	$(SILENCE)$(PANDA_CXX) $(CXXFLAGS) -shared -o $@ $^ $(INC_DIR) $(LIB_DIR) $(LIBS)


clean :
	$(SILENCE)rm -rf *.o
	$(SILENCE)rm -rf $(PANDA_LOG_TEST)
	$(SILENCE)rm -rf $(PANDA_MODULE_TEST)
	$(SILENCE)rm -rf *log
	$(SILENCE)echo "" > Makefile.dep


release :
	$(SILENCE)cp $(PANDA_LOG_TEST) $(PANDA_BUILD_TOP)/target/bin
	$(SILENCE)cp $(PANDA_MODULE_TEST) $(PANDA_BUILD_TOP)/target/lib


#generate dependence file
#let the compiler do the hard work. XD
include Makefile.dep
.PHONY : dep
dep :
	$(SILENCE)$(PANDA_CXX) -MM *.cpp  $(INC_DIR) > Makefile.dep

