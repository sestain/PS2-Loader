EE_BIN = loader.elf
EE_BIN_PKD = loader-packed.elf
EE_OBJS = main.o sio2man_irx.o padman_irx.o
EE_INCS += -I$(GSKIT)/include
EE_LDFLAGS = -L$(GSKIT)/lib
EE_LIBS = -lgskit -ldmakit -lpad -lpatches

ifeq ($(DEBUG), 1)
  $(info --- debugging enabled)
  EE_CFLAGS += -DDEBUG -O0 -g
  EE_LIBS += -lelf-loader
else 
  EE_CFLAGS += -Os
  EE_LDFLAGS += -s
  EE_LIBS += -lelf-loader-nocolour
endif


all: $(EE_BIN)
	$(EE_STRIP) --strip-all $(EE_BIN)
	ps2-packer $(EE_BIN) $(EE_BIN_PKD) > /dev/null

clean:
	rm -f *.elf *.o *.a *.s *.i *.map *_irx.c

rebuild:clean all

sio2man_irx.c: 
	$(PS2SDK)/bin/bin2c $(PS2SDK)/iop/irx/freesio2.irx sio2man_irx.c sio2man_irx

padman_irx.c: 
	$(PS2SDK)/bin/bin2c $(PS2SDK)/iop/irx/freepad.irx padman_irx.c padman_irx

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal_cpp
