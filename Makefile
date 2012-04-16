#
#
#

PROJECT  = skel

CSRC     = $(wildcard *.c)
CXXSRC   = $(wildcard *.cpp)
ASRC     = $(wildcard *.S)

OSRC     = lpc17xx_clkpwr.o lpc17xx_pinsel.o

INC      = inc

LIBRARIES = 

OUTDIR   = build

#DEPDIR   = .deps
#df       = $(DEPDIR)/$(*F)

CHIP     = lpc1758
MCU      = cortex-m3

ARCH     = arm-none-eabi
PREFIX   = $(ARCH)-

CC       = $(PREFIX)gcc
CXX      = $(PREFIX)g++
OBJCOPY  = $(PREFIX)objcopy

MKDIR    = mkdir

OPTIMIZE = s

CDEFS    = __BUILD_WITH_EXAMPLE__

FLAGS    = -O$(OPTIMIZE) -mcpu=$(MCU) -mthumb -mthumb-interwork -mlong-calls -ffunction-sections -fdata-sections -Wall
FLAGS   += $(patsubst %,-I%,$(INC))
FLAGS   += $(patsubst %,-D%,$(CDEFS))
CFLAGS   = $(FLAGS) -std=gnu99
ASFLAGS  = $(FLAGS)
CXXFLAGS = $(FLAGS) -fno-rtti -fno-exceptions

LDFLAGS  = $(CXXFLAGS) -Wl,--gc-sections -Wl,-e,__cs3_reset_cortex_m -Wl,-T,lpc1758.ld
LDFLAGS += $(patsubst %,-L%,$(LIBRARIES)) -lc -lstdc++

OBJ      = $(patsubst %,$(OUTDIR)/%,$(CSRC:.c=.o) $(CXXSRC:.cpp=.o) $(ASRC:.S=.o)) $(OSRC)

VPATH    = . $(INC) drivers/Drivers/source

.PHONY: all clean program

.PRECIOUS: $(OBJ)

all: $(OUTDIR)/$(PROJECT).elf $(OUTDIR)/$(PROJECT).bin $(OUTDIR)/$(PROJECT).hex

clean:
	$(RM) -f $(OBJ) $(OUTDIR)/$(PROJECT).bin $(OUTDIR)/$(PROJECT).hex $(OUTDIR)/$(PROJECT).elf

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

$(OUTDIR)/%.elf: $(OBJ)
	@echo "  LINK  " $@
	@$(CXX) $^ -o $@ $(LDFLAGS)

$(OUTDIR)/%.o: %.c $(OUTDIR)
	@echo "  CC    " $@
	@#$(CC) $(CFLAGS) -MM -MF $(df).d $<
	@$(CC) $(CFLAGS) -c -o $@ $<

$(OUTDIR)/%.o: %.cpp $(OUTDIR)
	@echo "  CXX   " $@
	@#$(CXX) $(CFLAGS) -MM -MF $(df).d $<
	@$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OUTDIR)/%.o: %.S $(OUTDIR)
	@echo "  AS    " $@
	@#$(CC) $(CFLAGS) -MM -MF $(df).d $<
	@$(CC) $(ASFLAGS) -c -o $@ $<

#-include $(SRCS:%.c=$(DEPDIR)/%.P)
