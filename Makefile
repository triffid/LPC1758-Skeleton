#
#
#

PROJECT  = skel

CSRC     = $(wildcard *.c)
CXXSRC   = $(wildcard *.cpp)
ASRC     = $(wildcard *.S)

SUBDIRS  = Drivers Core

INC      = inc $(patsubst %,%/inc, $(SUBDIRS)) uip uip/webserver

LIBRARIES =

OUTDIR   = build

OSRC     =

NXPSRC   = $(shell find Drivers/ -name '*.c') $(shell find Core/ -name '*.c')
NXPO     = $(patsubst %.c,$(OUTDIR)/%.o,$(notdir $(NXPSRC))) $(OUTDIR)/system_LPC17xx.o

UIPSRC   = $(shell find uip/ -name '*.c')
UIPO     = $(patsubst %.c,$(OUTDIR)/%.o,$(notdir $(UIPSRC)))

#USBSRC   = $(shell find USB/ -name '*.c')
#USBO     = $(patsubst %.c,$(OUTDIR)/%.o,$(notdir $(USBSRC)))

#LWIPSRC  = $(shell find lwip/ -name '*.c')
#LWIPO    = $(patsubst %.c,$(OUTDIR)/%.o,$(notdir $(LWIPSRC)))

#DEPDIR   = .deps
#df       = $(DEPDIR)/$(*F)

CHIP     = lpc1758
MCU      = cortex-m3

ARCH     = arm-none-eabi
PREFIX   = $(ARCH)-

CC       = $(PREFIX)gcc
CXX      = $(PREFIX)g++
OBJCOPY  = $(PREFIX)objcopy
AR       = $(PREFIX)ar

MKDIR    = mkdir
RMDIR    = rmdir
RM       = rm -f

OPTIMIZE = s

CDEFS    = MAX_URI_LENGTH=512 __LPC17XX__ DEBUG_MESSAGES

FLAGS    = -O$(OPTIMIZE) -mcpu=$(MCU) -mthumb -mthumb-interwork -mlong-calls -ffunction-sections -fdata-sections -Wall
FLAGS   += $(patsubst %,-I%,$(INC))
FLAGS   += $(patsubst %,-D%,$(CDEFS))
CFLAGS   = $(FLAGS) -std=gnu99
ASFLAGS  = $(FLAGS)
CXXFLAGS = $(FLAGS) -fno-rtti -fno-exceptions

LDFLAGS  = $(CXXFLAGS) -Wl,--gc-sections -Wl,-e,__cs3_reset_cortex_m -Wl,-T,lpc1758.ld
LDFLAGS += $(patsubst %,-L%,$(LIBRARIES)) -lc -lstdc++

OBJ      = $(patsubst %,$(OUTDIR)/%,$(CSRC:.c=.o) $(CXXSRC:.cpp=.o) $(ASRC:.S=.o))

VPATH    = . $(patsubst %/inc,%/src,$(INC)) $(dir $(NXPSRC)) $(dir $(USBSRC)) $(dir $(UIPSRC)) $(dir $(LWIPSRC))

.PHONY: all clean program

.PRECIOUS: $(OBJ)

all: $(OUTDIR) $(OUTDIR)/$(PROJECT).elf $(OUTDIR)/$(PROJECT).bin $(OUTDIR)/$(PROJECT).hex

clean:
	$(RM) $(OBJ) $(OUTDIR)/$(PROJECT).bin $(OUTDIR)/$(PROJECT).hex $(OUTDIR)/$(PROJECT).elf $(NXPO)
	$(RMDIR) $(OUTDIR)

program: $(OUTDIR)/$(PROJECT).bin
	mount /mnt/r2c2
	cp $< /mnt/r2c2/firmware.bin
	eject /mnt/r2c2

$(OUTDIR):
	$(MKDIR) $(OUTDIR)

$(OUTDIR)/%.bin: $(OUTDIR)/%.elf
	@echo "  COPY  " $@
	@$(OBJCOPY) -O binary $< $@

$(OUTDIR)/%.hex: $(OUTDIR)/%.elf
	@echo "  COPY  " $@
	@$(OBJCOPY) -O ihex $< $@

$(OUTDIR)/%.elf: $(OBJ) $(OUTDIR)/nxp.ar $(OUTDIR)/uip.ar
	@echo "  LINK  " $@
	@$(CXX) $^ $(OSRC) -o $@ $(LDFLAGS)

$(OUTDIR)/%.o: %.c
	@echo "  CC    " $@
	@#$(CC) $(CFLAGS) -MM -MF $(df).d $<
	@$(CC) $(CFLAGS) -c -o $@ $<

$(OUTDIR)/%.o: %.c
	@echo "  CC    " $@
	@#$(CC) $(CFLAGS) -MM -MF $(df).d $<
	@$(CC) $(CFLAGS) -c -o $@ $<

$(OUTDIR)/%.o: %.cpp
	@echo "  CXX   " $@
	@#$(CXX) $(CFLAGS) -MM -MF $(df).d $<
	@$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OUTDIR)/%.o: %.S
	@echo "  AS    " $@
	@#$(CC) $(CFLAGS) -MM -MF $(df).d $<
	@$(CC) $(ASFLAGS) -c -o $@ $<

$(OUTDIR)/nxp.ar: $(NXPO)
	@echo "  AR    " $@
	@$(AR) ru $@ $^

$(OUTDIR)/usb.ar: $(USBO)
	@echo "  AR    " $@
	@$(AR) ru $@ $^

$(OUTDIR)/uip.ar: $(UIPO)
	@echo "  AR    " $@
	@$(AR) ru $@ $^

$(OUTDIR)/lwip.ar: $(LWIPO)
	@echo "  AR    " $@
	@$(AR) ru $@ $^

#-include $(SRCS:%.c=$(DEPDIR)/%.P)
