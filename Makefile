#
#
#

PROJECT  = skel

CSRC     = $(wildcard *.c)
CXXSRC   = $(wildcard *.cpp)
ASRC     = $(wildcard *.S)

OSRC     = 

INC      = inc

LIBRARIES = 

OUTDIR   = build

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

OBJ      = $(patsubst %,$(OUTDIR)/%,$(CSRC:.c=.o) $(CXXSRC:.cpp=.o) $(ASRC:.S=.o))

VPATH    = . $(INC)

.PHONY: all bin hex clean program

.PRECIOUS: $(OBJ)

all: $(OUTDIR)/$(PROJECT).elf bin hex

clean:
	$(RM) -f $(OBJ) $(OUTDIR)/$(PROJECT).bin $(OUTDIR)/$(PROJECT).hex $(OUTDIR)/$(PROJECT).elf

bin: $(OUTDIR)/$(PROJECT).bin

hex: $(OUTDIR)/$(PROJECT).hex

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

$(OUTDIR)/%.elf: $(OBJ) $(OSRC)
	@echo "  LINK  " $@
	@$(CXX) $^ $(OSRC) -o $@ $(LDFLAGS)

$(OUTDIR)/%.o: %.c $(OUTDIR)
	@echo "  CC    " $@
	@$(CC) $(CFLAGS) -c -o $@ $<

$(OUTDIR)/%.o: %.cpp $(OUTDIR)
	@echo "  CXX   " $@
	@$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OUTDIR)/%.o: %.S $(OUTDIR)
	@echo "  AS    " $@
	@$(CC) $(ASFLAGS) -c -o $@ $<
