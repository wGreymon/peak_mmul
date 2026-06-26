help:
	@echo "Usage:"
	@echo "  make aie       - Compile AIE graph to build/libadf.a"
	@echo "  make kernels   - Compile HLS mm2s/s2mm kernels"
	@echo "  make xsa       - Link AIE + PL kernels into standalone XSA"
	@echo "  make xclbin    - Package runtime xclbin from standalone XSA"
	@echo "  make host      - Build aarch64 host executable"
	@echo "  make sd_card   - Package standalone SD card image"
	@echo "  make all       - Build AIE, PL kernels, XSA, xclbin, and host"
	@echo "  make img       - Build all artifacts and package standalone SD card image"
	@echo ""
	@echo "Standalone artifacts:"
	@echo "  build/package_standalone/sd_card.img"
	@echo "  build/package_standalone/sd_card.img.zip"
	@echo "  make clean     - Remove generated files"

MK_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
PROJ_DIR := $(abspath $(dir $(MK_PATH)))
BUILD_DIR := $(PROJ_DIR)/build

PLATFORM ?= /tools/Xilinx/2025.2/Vitis/base_platforms/xilinx_vek280_base_202520_1/xilinx_vek280_base_202520_1.xpfm
TARGET ?= hw
AIETARGET ?= hw

CXX_ARM ?= $(XILINX_VITIS)/gnu/aarch64/lin/aarch64-linux/bin/aarch64-linux-gnu-g++
SYSROOT ?= /opt/xilinx-versal-common-v2025.2/sysroot/sysroots/cortexa72-cortexa53-amd-linux
COMMON_IMAGE_DIR ?= /opt/xilinx-versal-common-v2025.2
ROOTFS ?= $(COMMON_IMAGE_DIR)/rootfs.ext4
IMAGE ?= $(COMMON_IMAGE_DIR)/Image

LIBADF := $(BUILD_DIR)/libadf.a
MM2S_XO := $(BUILD_DIR)/mm2s.xo
S2MM_XO := $(BUILD_DIR)/s2mm.xo
XSA := $(BUILD_DIR)/peak_bf16_single_tile_standalone.xsa
RUNTIME_XCLBIN := $(BUILD_DIR)/peak_bf16_single_tile_standalone.xclbin
LINK_XCLBIN := $(BUILD_DIR)/_x/link/int/peak_bf16_single_tile_standalone.xclbin
HOST_EXE := $(BUILD_DIR)/peak_bf16_single_tile_host
RUN_SCRIPT := $(BUILD_DIR)/run_standalone.sh
PACKAGE_DIR := $(BUILD_DIR)/package_standalone
SD_CARD_IMG := $(PACKAGE_DIR)/sd_card.img
SD_CARD_ZIP := $(PACKAGE_DIR)/sd_card.img.zip

AIE_SRC := $(PROJ_DIR)/aie/graph.cpp
AIE_DEPS := $(PROJ_DIR)/aie/graph.h \
            $(PROJ_DIR)/aie/bf16_local_buffer_kernel.cc \
            $(PROJ_DIR)/include/peak_bf16_config.h \
            $(PROJ_DIR)/include/kernels.h

AIE_FLAGS := --target=$(AIETARGET)
AIE_FLAGS += --platform=$(PLATFORM)
AIE_FLAGS += --aie.pl-freq=312.5
AIE_FLAGS += --aie.workdir=$(BUILD_DIR)/Work
AIE_FLAGS += --aie.output-archive=$(LIBADF)
AIE_FLAGS += -I $(PROJ_DIR)/include
AIE_FLAGS += -I $(PROJ_DIR)/aie

HLS_FLAGS := -c --mode hls --platform $(PLATFORM)

LINK_FLAGS := -g -l
LINK_FLAGS += --platform $(PLATFORM)
LINK_FLAGS += -t $(TARGET)
LINK_FLAGS += --save-temps
LINK_FLAGS += --verbose
LINK_FLAGS += --config $(PROJ_DIR)/config/system.cfg

HOST_CXX_FLAGS := -std=c++17 -O2 -Wall --sysroot=$(SYSROOT)
HOST_CXX_FLAGS += -I $(SYSROOT)/usr/include
HOST_CXX_FLAGS += -I $(XILINX_XRT)/include
HOST_CXX_FLAGS += -I $(PROJ_DIR)/include
HOST_LD_FLAGS := --sysroot=$(SYSROOT)
HOST_LD_FLAGS += -L$(SYSROOT)/usr/lib
HOST_LD_FLAGS += -Wl,-rpath-link,$(SYSROOT)/usr/lib
HOST_LD_FLAGS += -lxrt_coreutil -lpthread -lrt -ldl -lcrypt

$(BUILD_DIR):
	mkdir -p $@

$(LIBADF): $(AIE_SRC) $(AIE_DEPS) | $(BUILD_DIR)
	@echo "=== Compiling AIE graph ==="
	cd $(PROJ_DIR) && v++ -c --mode aie $(AIE_FLAGS) aie/graph.cpp

$(MM2S_XO): $(PROJ_DIR)/pl_kernels/mm2s.cpp $(PROJ_DIR)/pl_kernels/mm2s.cfg | $(BUILD_DIR)
	@echo "=== Compiling HLS mm2s ==="
	cd $(PROJ_DIR)/pl_kernels && v++ $(HLS_FLAGS) --config mm2s.cfg
	cp $(PROJ_DIR)/pl_kernels/mm2s.xo $@

$(S2MM_XO): $(PROJ_DIR)/pl_kernels/s2mm.cpp $(PROJ_DIR)/pl_kernels/s2mm.cfg | $(BUILD_DIR)
	@echo "=== Compiling HLS s2mm ==="
	cd $(PROJ_DIR)/pl_kernels && v++ $(HLS_FLAGS) --config s2mm.cfg
	cp $(PROJ_DIR)/pl_kernels/s2mm.xo $@

$(XSA): $(LIBADF) $(MM2S_XO) $(S2MM_XO) $(PROJ_DIR)/config/system.cfg | $(BUILD_DIR)
	@echo "=== Linking PLIO hardware system XSA ==="
	cd $(BUILD_DIR) && v++ $(LINK_FLAGS) $(MM2S_XO) $(S2MM_XO) $(LIBADF) -o $@

$(RUNTIME_XCLBIN): $(XSA) | $(BUILD_DIR)
	@echo "=== Copying standalone full hardware xclbin ==="
	test -f $(LINK_XCLBIN)
	cp $(LINK_XCLBIN) $@

$(HOST_EXE): $(PROJ_DIR)/app/main.cc $(PROJ_DIR)/include/peak_bf16_config.h | $(BUILD_DIR)
	@echo "=== Building host for aarch64 ==="
	$(CXX_ARM) -o $@ $(PROJ_DIR)/app/main.cc $(HOST_CXX_FLAGS) $(HOST_LD_FLAGS)

$(RUN_SCRIPT): | $(BUILD_DIR)
	@echo "=== Generating standalone run script ==="
	printf '%s\n' \
		'#!/bin/sh' \
		'set -eu' \
		'cd "$$(dirname "$$0")"' \
		'export LD_LIBRARY_PATH=/opt/xilinx/xrt/lib:$${LD_LIBRARY_PATH:-}' \
		'./peak_bf16_single_tile_host peak_bf16_single_tile_standalone.xclbin' \
		> $@
	chmod +x $@

$(SD_CARD_IMG): $(XSA) $(LIBADF) $(RUNTIME_XCLBIN) $(HOST_EXE) $(RUN_SCRIPT) $(ROOTFS) $(IMAGE) | $(BUILD_DIR)
	@echo "=== Packaging standalone SD card image ==="
	mkdir -p $(PACKAGE_DIR)
	cd $(BUILD_DIR) && v++ -p -t $(TARGET) \
		--platform $(PLATFORM) \
		--package.boot_mode=sd \
		--package.image_format=ext4 \
		--package.rootfs $(ROOTFS) \
		--package.kernel_image $(IMAGE) \
		--package.sd_file $(HOST_EXE) \
		--package.sd_file $(RUNTIME_XCLBIN) \
		--package.sd_file $(RUN_SCRIPT) \
		--package.defer_aie_run \
		--save-temps \
		--temp_dir $(BUILD_DIR)/_x_sd \
		--package.out_dir $(PACKAGE_DIR) \
		$(XSA) $(LIBADF)

$(SD_CARD_ZIP): $(SD_CARD_IMG)
	cd $(PACKAGE_DIR) && zip -j sd_card.img.zip sd_card.img

aie: $(LIBADF)
kernels: $(MM2S_XO) $(S2MM_XO)
xsa: $(XSA)
xclbin: $(RUNTIME_XCLBIN)
host: $(HOST_EXE)
sd_card: $(SD_CARD_ZIP)
all: $(LIBADF) $(MM2S_XO) $(S2MM_XO) $(XSA) $(RUNTIME_XCLBIN) $(HOST_EXE)
img: sd_card

clean:
	rm -rf $(BUILD_DIR) .Xil AIECompiler.log AIESimulator.log Work* \
		xcd.log Map_Report.csv aiesimulator_output diag_report.log \
		pl_sample_counts sol.db xsc_report.log v++* *.log
	rm -rf $(PROJ_DIR)/pl_kernels/hls $(PROJ_DIR)/pl_kernels/mm2s \
		$(PROJ_DIR)/pl_kernels/s2mm $(PROJ_DIR)/pl_kernels/*.xo \
		$(PROJ_DIR)/pl_kernels/*.log $(PROJ_DIR)/pl_kernels/*.jou \
		$(PROJ_DIR)/pl_kernels/.Xil

.PHONY: help aie kernels xsa xclbin host sd_card all img clean
