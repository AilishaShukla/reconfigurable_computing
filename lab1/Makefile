# Copyright 2021 Xilinx Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

ECHO=@echo

.PHONY: help

help::
	$(ECHO) "Makefile Usage:"
	$(ECHO) "  make build [TARGET=hw_emu]"
	$(ECHO) ""
	$(ECHO) "  make host"
	$(ECHO) ""
	$(ECHO) "  make clean"
	$(ECHO) ""


ENABLE_AIE := 0
TARGET := hw
PLATFORM := xilinx_vck5000_gen4x8_qdma_2_202220_1
CXXFLAGS := -std=c++17 -Wno-deprecated-declarations
CXXFLAGS += -I$(XILINX_XRT)/include -I$(XILINX_HLS)/include
LDFLAGS := -L$(XILINX_XRT)/lib
LDFLAGS += $(LDFLAGS) -lxrt_coreutil 
XOCCFLAGS := --platform $(PLATFORM) -t $(TARGET)  -s -g
KERNEL_FLAGS := --kernel pl -I. --hls.clock 300000000:pl
XOCCLFLAGS := --kernel_frequency 200 --platform $(PLATFORM) -t $(TARGET)  -s -g
TEMP_DIR := ./_x.$(TARGET).xilinx_vck5000_gen4x8_qdma_2_202220_1
BUILD_DIR := ./build_dir.$(TARGET).xilinx_vck5000_gen4x8_qdma_2_202220_1
KERNEL_XO :=$(TEMP_DIR)/pl.xo
RM = rm -f
RMDIR = rm -rf
AIE_OBJ := ./libadf.a
XSA_OBJ := pl_$(TARGET).xsa
XCLBIN  := pl_$(TARGET).xclbin
EXECUTABLE := host.exe
HOST_SRCS := ./src/host.cpp


all: build host

host: $(EXECUTABLE)

$(EXECUTABLE): $(HOST_SRCS)
	$(CXX) -o $(EXECUTABLE) $^ $(CXXFLAGS) $(LDFLAGS)

build: $(XCLBIN)

ifeq ($(ENABLE_AIE),1)
$(XCLBIN): $(XSA_OBJ) $(AIE_OBJ)
	v++ -p -t $(TARGET) -f $(PLATFORM) $^ -o $@ --package.boot_mode=ospi
$(XSA_OBJ): $(KERNEL_XO) $(AIE_OBJ)
	v++ -l $(XOCCLFLAGS) --config xclbin_overlay.cfg -o $@ $^
else
$(XCLBIN): $(XSA_OBJ)
	v++ -p -t $(TARGET) -f $(PLATFORM) $^ -o $@ --package.boot_mode=ospi
	@echo "### ***** $(XCLBIN) compilation done! *****"
$(XSA_OBJ): $(KERNEL_XO)
	mkdir -p $(BUILD_DIR)
	v++ -l $(XOCCLFLAGS) --temp_dir $(BUILD_DIR) -o $@ $^ | tee $(BUILD_DIR)/hpc_xclbin.log
	@echo "### ***** $(XSA_OBJ) compilation done! *****"
endif

hls: $(KERNEL_XO)
$(KERNEL_XO): ./src/pl.cpp
	mkdir -p $(TEMP_DIR)
	v++ $(XOCCFLAGS) $(KERNEL_FLAGS) -c -o $@ $< | tee $(TEMP_DIR)/pl.log
	@echo "### ***** $(KERNEL_XO) compilation done! *****"

.PHONY: clean

clean:
	-$(RMDIR) $(EXECUTABLE) $(XCLBIN)/{*sw_emu*,*hw_emu*} 
	-$(RMDIR) profile_* TempConfig system_estimate.xtxt *.rpt *.csv *.o *.xo *.xpe *.xsa cfg qemu_dts_files emu_qemu_scripts *.db sim Work aie/*.ll _x*
	-$(RMDIR)  *v++* .Xil emconfig.json dltmp* xmltmp* *.log *.jou *.wcfg *.wdb *bin* *summary* *.BIN *.bif *.exe  *.log *.txt _x *.ltx
cleanaie: cleansim
	-$(RMDIR) Work
cleansim:
	-$(RMDIR) aiesimulator_output foo.vcd ISS_RPC_SERVER_PORT
cleantemp:
	-$(RMDIR) ${TEMP_DIR}
cleanhost:
	-$(RMDIR) ${HOST_EXE} ${AIE_OBJ} main.o
cleanall: clean cleantemp cleansim cleanaie cleanhost
	-$(RMDIR) build_dir* sd_card* .ipcache
	-$(RMDIR) package.hw run_app.sh 
	-$(RMDIR) *xclbin.run_summary qemu-memory-_* emulation _vimage start_simulation.sh *.xclbin