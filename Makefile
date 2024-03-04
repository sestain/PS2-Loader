EE_BIN = loader.elf
EE_BIN_PKD = loader-packed.elf
EE_OBJS = main.o sio2man.o padman.o
EE_INCS += -I$(GSKIT)/include
EE_LDFLAGS = -L$(GSKIT)/lib -s
EE_LIBS = -lgskit -ldmakit -lpad -lelf-loader -lpatches

KERNEL_NOPATCH = 1 
NEWLIB_NANO = 1


all: $(EE_BIN)
	$(EE_STRIP) --strip-all $(EE_BIN)
	ps2-packer $(EE_BIN) $(EE_BIN_PKD) > /dev/null

clean:
	rm -f *.elf *.o *.a *.s *.i *.map

rebuild:clean all

sio2man.s: 
	$(PS2SDK)/bin/bin2s $(PS2SDK)/iop/irx/freesio2.irx sio2man.s sio2man_irx

padman.s: 
	$(PS2SDK)/bin/bin2s $(PS2SDK)/iop/irx/freepad.irx padman.s padman_irx

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal_cpp
