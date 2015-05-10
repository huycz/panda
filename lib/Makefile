#read variables from command line and environment variables
ifeq (X$(showcommand), Xy)
	SILENCE=
else
	SILENCE=@
endif

.PHONY : iniparser st
all : iniparser st


iniparser :
	@echo "iniparse unpackage ..."
	$(SILENCE)tar -xzf iniparser.tar.gz
	
	@echo "iniparse build ..."
	$(SILENCE)make -C iniparser default

	@echo "iniparse release ..."
	$(SILENCE)cp iniparser/src/*.h $(PANDA_BUILD_TOP)/inc
	$(SILENCE)cp iniparser/libiniparser.a $(PANDA_BUILD_TOP)/target/lib
	@echo "iniparse done ..."


st :
	@echo "st unpackage ..."
	$(SILENCE)tar -xzf st.tar.gz
	
	@echo "st build ..."
	$(SILENCE)make -C st OS=LINUX

	@echo "st release ..."
	$(SILENCE)cp st/obj/*.h $(PANDA_BUILD_TOP)/inc
	$(SILENCE)cp st/obj/libst.a $(PANDA_BUILD_TOP)/target/lib
	@echo "st done ..."


clean :
	$(SILENCE)rm -rf iniparser
	$(SILENCE)rm -rf st


