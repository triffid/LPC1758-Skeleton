#
#
#

PROJECT  = skel

CSRC     = $(wildcard *.c)
CXXSRC   = $(wildcard *.cpp)
ASRC     = $(wildcard *.S)

INC      = inc

LIBRARIES = 

OUTDIR   = build

OSRC     = 

NXPSRC   = $(wildcard drivers/Drivers/source/lpc17xx_*.c)
NXPO     = $(patsubst drivers/Drivers/source/%.c,$(OUTDIR)/%.o,$(NXPSRC)) $(OUTDIR)/system_LPC17xx.o

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

CDEFS    =

FLAGS    = -O$(OPTIMIZE) -mcpu=$(MCU) -mthumb -mthumb-interwork -mlong-calls -ffunction-sections -fdata-sections -Wall
FLAGS   += $(patsubst %,-I%,$(INC))
FLAGS   += $(patsubst %,-D%,$(CDEFS))
CFLAGS   = $(FLAGS) -std=gnu99
ASFLAGS  = $(FLAGS)
CXXFLAGS = $(FLAGS) -fno-rtti -fno-exceptions

LDFLAGS  = $(CXXFLAGS) -Wl,--gc-sections -Wl,-e,__cs3_reset_cortex_m -Wl,-T,lpc1758.ld
LDFLAGS += $(patsubst %,-L%,$(LIBRARIES)) -lc -lstdc++

OBJ      = $(patsubst %,$(OUTDIR)/%,$(CSRC:.c=.o) $(CXXSRC:.cpp=.o) $(ASRC:.S=.o))

VPATH    = . $(INC) drivers/Drivers/source drivers/Core/CM3/DeviceSupport/NXP/LPC17xx

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

$(OUTDIR)/%.elf: $(OBJ) $(OUTDIR)/nxp.ar
	@echo "  LINK  " $@
	@$(CXX) $^ $(OSRC) -o $@ $(LDFLAGS)

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
	

#-include $(SRCS:%.c=$(DEPDIR)/%.P)
