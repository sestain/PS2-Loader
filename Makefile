EE_BIN = loader.elf
EE_BIN_PKD = loader-packed.elf
EE_OBJS = main.o
EE_INCS += -I$(GSKIT)/include
EE_LDFLAGS = -L$(GSKIT)/lib -s
EE_LIBS = -lgskit -ldmakit -lpad -lelf-loader

all: $(EE_BIN)
	$(EE_STRIP) --strip-all $(EE_BIN)
	ps2-packer $(EE_BIN) $(EE_BIN_PKD) > /dev/null

clean:
	rm -f *.elf *.o *.a *.s *.i *.map

rebuild:clean all

release:rebuild
	rm -f *.elf *.o *.a *.s *.i *.map

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal