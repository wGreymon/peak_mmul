MK_PATH  := $(abspath $(lastword $(MAKEFILE_LIST)))
PROJ_DIR := $(abspath $(dir $(MK_PATH)))
BUILD_DIR := $(PROJ_DIR)/build
TOP_DIR   := $(PROJ_DIR)  # This file lives at peak_mmul/

.PHONY: help bf16 int8 all clean

help:
	@echo "VEK280 AIE-ML Peak Performance Benchmark"
	@echo ""
	@echo "Usage:"
	@echo "  make bf16      - Build BF16 peak benchmark"
	@echo "  make int8      - Build INT8 peak benchmark"
	@echo "  make all       - Build both"
	@echo "  make clean     - Remove all build artifacts"
	@echo ""
	@echo "  cd peak_bf16  && make all  - Build & run BF16"
	@echo "  cd peak_int8  && make all  - Build & run INT8"

bf16:
	$(MAKE) -C $(PROJ_DIR)/peak_bf16 $(MAKEFLAGS)

int8:
	$(MAKE) -C $(PROJ_DIR)/peak_int8 $(MAKEFLAGS)

all: bf16 int8

clean:
	rm -rf $(PROJ_DIR)/peak_bf16/build $(PROJ_DIR)/peak_int8/build
	rm -rf $(PROJ_DIR)/peak_bf16/.Xil $(PROJ_DIR)/peak_int8/.Xil
	rm -f  $(PROJ_DIR)/peak_bf16/AIECompiler.log $(PROJ_DIR)/peak_int8/AIECompiler.log
	rm -f  $(PROJ_DIR)/peak_bf16/AIESimulator.log $(PROJ_DIR)/peak_int8/AIESimulator.log
