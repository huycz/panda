#read variables from command line and environment variables
ifeq (X$(showcommand), Xy)
	SILENCE=
else
	SILENCE=@
endif


rebuild : clean all


all : check prepare lib src


check :
ifeq (X$(PANDA_BUILD_TOP), X)
	@echo "please source build/setupenv firstly"
	@exit 1
endif


prepare : 
	$(SILENCE)mkdir -p $(PANDA_BUILD_TOP)/target
	$(SILENCE)mkdir -p $(PANDA_BUILD_TOP)/target/lib
	$(SILENCE)mkdir -p $(PANDA_BUILD_TOP)/target/bin


.PHONY : lib src
lib :
	$(SILENCE)make -C $(PANDA_BUILD_TOP)/lib all showcommand=$(showcommand)


src :
	$(SILENCE)make -C $(PANDA_BUILD_TOP)/src all showcommand=$(showcommand)


clean :
	$(SILENCE)rm -rf target
	$(SILENCE)cd ${PANDA_BUILD_TOP}/inc && rm -rf `ls | grep -v "panda"` && cd -
	$(SILENCE)make -C $(PANDA_BUILD_TOP)/lib clean showcommand=$(showcommand)
	$(SILENCE)make -C $(PANDA_BUILD_TOP)/src clean showcommand=$(showcommand)


help :
	@echo "Help    :"
	@echo "check   :  check building environment"
	@echo "lib     :  build all libraries"
	@echo "all     :  build all target and release to target"
	@echo "clean   :  clean all target"
	@echo "rebuild :  clean all and rebuild all"
	@echo "options :  showcommand=y : show building details"
