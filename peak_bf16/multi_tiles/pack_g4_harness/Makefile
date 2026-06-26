help:
	@echo "Makefile Usage:"
	@echo "  make all"
	@echo "      Build the Test Harness AIE package."
	@echo "  make run GRAPH_ITERATIONS=1"
	@echo "      Run through the board-side Test Harness server."
	@echo "  make cleanall"
	@echo "      Remove generated files."

MK_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
KERNEL_DIR := $(abspath $(dir $(MK_PATH)))
export TEST_HARNESS_REPO_PATH ?= $(abspath $(KERNEL_DIR)/../../../../AI-Engine-Test-Harness)

DEVICE ?= vek280
TARGET ?= hw
DEBUG ?= 0
GRAPH_ITERATIONS ?= 1
NUM_REPETITIONS ?= 1
CHANNEL_DELAY ?= 0
RUN_PERF_MODE ?= 0
PERF_REPETITIONS ?= 1

include ${TEST_HARNESS_REPO_PATH}/test_harness/common.mk

BUILD_DIR := $(KERNEL_DIR)/build
TEMP_DIR := $(BUILD_DIR)/_x_temp.$(TARGET).${PLATFORM_NAME}.xpfm
PKG_DIR := $(BUILD_DIR)/pkg.$(TARGET).${PLATFORM_NAME}.xpfm
VPP_TEMP_DIR := $(BUILD_DIR)/_x

TEST_HARNESS_XSA := ${TEST_HARNESS_REPO_PATH}/bin/vek280_test_harness.xsa
COMMON_OPS_HPP := ${TEST_HARNESS_REPO_PATH}/stx_kernels/common/include/stx_ops.hpp
COMMON_HOST_UTILS_HPP := ${TEST_HARNESS_REPO_PATH}/stx_kernels/common/include/stx_host_utils.hpp

AIE_CXX_FLAGS := -I $(XILINX_VITIS)/aietools/include
AIE_CXX_FLAGS += -I $(KERNEL_DIR)/aie
AIE_CXX_FLAGS += -I $(KERNEL_DIR)/include
AIE_CXX_FLAGS += -I ${TEST_HARNESS_REPO_PATH}/stx_kernels/common/include
AIE_CXX_FLAGS += -I ${TEST_HARNESS_REPO_PATH}/include/aie
AIE_CXX_FLAGS += -I ${TEST_HARNESS_REPO_PATH}/src/aie
AIE_CXX_FLAGS += --target=$(AIETARGET)
AIE_CXX_FLAGS += --aie.pl-freq=312.5
AIE_CXX_FLAGS += --platform=${TEST_HARNESS_XSA}
AIE_CXX_FLAGS += --aie.workdir=$(TEMP_DIR)/Work --aie.output-archive=$(TEMP_DIR)/libadf.a

HOST_CXX_FLAGS := ${HOST_CONFIG_FLAGS}
HOST_CXX_FLAGS += -I $(KERNEL_DIR)
HOST_CXX_FLAGS += -I $(KERNEL_DIR)/include
HOST_CXX_FLAGS += -I $(KERNEL_DIR)/host
HOST_CXX_FLAGS += -I ${TEST_HARNESS_REPO_PATH}/stx_kernels/common/include
HOST_CXX_FLAGS += -I ${TEST_HARNESS_REPO_PATH}/include/ps
HOST_LD_FLAGS := -lpthread

ifeq ($(DEBUG), 1)
	HOST_CXX_FLAGS += -g -DDEBUG
endif

CLIENT_EXE := $(TEMP_DIR)/client_exe
XCLBIN := $(TEMP_DIR)/vek280_test_harness.xclbin
AIE_ADF := $(TEMP_DIR)/libadf.a
RUN_DEPS := $(CLIENT_EXE) ${XCLBIN} run_script.sh

${BUILD_DIR}:
	mkdir -p $@

${PKG_DIR}:
	mkdir -p $@

$(TEST_HARNESS_XSA):
	cd ${TEST_HARNESS_REPO_PATH}/bin && ${TEST_HARNESS_REPO_PATH}/test_harness/scripts/download.sh

$(CLIENT_EXE): $(KERNEL_DIR)/client/tb.cpp \
               $(KERNEL_DIR)/host/peak_bf16_harness.cc \
               $(KERNEL_DIR)/include/peak_bf16_harness.hpp \
               $(KERNEL_DIR)/include/peak_bf16_config.h \
               $(COMMON_OPS_HPP) \
               $(COMMON_HOST_UTILS_HPP) | ${BUILD_DIR}
	mkdir -p $(dir $@)
	${CXX} -o $@ $(KERNEL_DIR)/client/tb.cpp \
		$(KERNEL_DIR)/host/peak_bf16_harness.cc \
		$(HOST_CXX_FLAGS) ${HOST_LD_FLAGS}

download_xsa: $(TEST_HARNESS_XSA)

$(AIE_ADF): $(KERNEL_DIR)/aie/graph.cpp \
            $(KERNEL_DIR)/aie/graph.h \
            $(KERNEL_DIR)/aie/bf16_pack_first.cc \
            $(KERNEL_DIR)/aie/bf16_pack_middle.cc \
            $(KERNEL_DIR)/aie/bf16_pack_last.cc \
            $(KERNEL_DIR)/include/kernels.h \
            $(KERNEL_DIR)/include/peak_bf16_config.h \
            ${TEST_HARNESS_XSA} | ${BUILD_DIR}
	mkdir -p $(TEMP_DIR) $(VPP_TEMP_DIR)
	@echo "Compiling: $(AIE_ADF)"
	cd $(BUILD_DIR) && v++ -c --mode aie $(AIE_CXX_FLAGS) $(KERNEL_DIR)/aie/graph.cpp

build_client_exe: $(CLIENT_EXE)

${XCLBIN}: ${AIE_ADF}
ifeq ($(TARGET), hw)
	mkdir -p $(TEMP_DIR) $(VPP_TEMP_DIR)
	cd $(BUILD_DIR) && v++ -p -t ${TARGET} -f ${TEST_HARNESS_PLATFORM} -o $@ $^ \
		--save-temps \
		--temp_dir $(VPP_TEMP_DIR) \
		--config ${TEST_HARNESS_REPO_PATH}/cfg/package_aie_only.cfg \
		--package.defer_aie_run \
		--advanced.param package.enableAiePartitionDrc=0 \
		--package.out_dir ${TEMP_DIR}
endif

package: $(RUN_DEPS) ${PKG_DIR}
	cp -r $(RUN_DEPS) ${PKG_DIR}
	chmod +x ${PKG_DIR}/run_script.sh ${PKG_DIR}/client_exe

all: check_vitis download_xsa package

run: all
ifeq ($(TARGET), hw)
ifneq ($(CPU), arm)
	cd ${PKG_DIR} && LD_LIBRARY_PATH=${LD_LIBRARY_PATH} \
		GRAPH_ITERATIONS=${GRAPH_ITERATIONS} \
		NUM_REPETITIONS=${NUM_REPETITIONS} \
		CHANNEL_DELAY=${CHANNEL_DELAY} \
		RUN_PERF_MODE=${RUN_PERF_MODE} \
		PERF_REPETITIONS=${PERF_REPETITIONS} \
		./run_script.sh
endif
endif

cleanall:
	rm -rf $(BUILD_DIR) AIECompiler.log Work* x86simulator_output xcd.log .Xil AIESimulator.log Map_Report.csv \
		aiesimulator_output/ diag_report.log pl_sample_counts sol.db xsc_report.log v++* *libadf.a pkg* _x* \
		package_hw .AIE_SIM_CMD_LINE_OPTIONS *.log

.PHONY: help download_xsa build_client_exe package all run cleanall
