.PHONY: help

help::
	$(ECHO) "Makefile Usage:"
	$(ECHO) "  make all TARGET=<sw_emu/hw_emu/hw> DEVICE=<FPGA platform> VER=<host_cpp/host_c>"
	$(ECHO) "      Command to pick the specific files and generates the design for specified Target and Device."
	$(ECHO) ""
	$(ECHO) "  make clean "
	$(ECHO) "      Command to remove the generated non-hardware files."
	$(ECHO) ""
	$(ECHO) "  make cleanall"
	$(ECHO) "      Command to remove all the generated files."
	$(ECHO) ""
	$(ECHO) "  make check TARGET=<sw_emu/hw_emu/hw> DEVICE=<FPGA platform>"
	$(ECHO) "      Command to run application in emulation."
	$(ECHO) ""

TARGETS := hw
TARGET := $(TARGETS)
DEVICE := $(DEVICES)
XCLBIN := ./xclbin
KERNEL_NAME := vadd
VER ?= host_cpp

DSA := $(call device2sandsa, $(DEVICE))
BUILD_DIR := ./_x.$(TARGET)

BUILD_DIR_vadd = $(BUILD_DIR)/$(KERNEL_NAME)

# The C++ Compiler to use: g++
CXX := g++

# The kernel Compiler to use : V++
VPP := $(XILINX_VITIS)/bin/v++

# The below are compile flags are passed to the C++ Compiler
opencl_CXXFLAGS += -g -I./ -I$(XILINX_XRT)/include -I$(XILINX_VIVADO)/include -Wall -O0 -g -std=c++11
# The below are linking flags for C++ Comnpiler
opencl_LDFLAGS += -L$(XILINX_XRT)/lib -lOpenCL -lpthread

CXXFLAGS += $(opencl_CXXFLAGS)
LDFLAGS += $(opencl_LDFLAGS)

# Host CPP FILE
HOST_C_SRCS += src/host.c
# Host Header FILE
HOST_C_HDRS += src/host.h
#Host CPP FILE
HOST_CPP_SRCS += src/host.cpp
# Host Header FILE
HOST_CPP_HDRS += src/host.hpp
# HOST_CPP_HDRS += include/CL/cl2.hpp


# Enable Profiling
REPORT := yes
PROFILE:= yes


#timeline_trace is always enabled



# Host compiler global settings
CXXFLAGS += -fmessage-length=0
LDFLAGS += -lrt -lstdc++

# Kernel compiler global settings
CLFLAGS += -t $(TARGET) --config design.cfg --save-temps


EXECUTABLE = host
EMCONFIG_DIR = $(XCLBIN)


BINARY_CONTAINERS += $(XCLBIN)/$(KERNEL_NAME).$(TARGET).xclbin
BINARY_CONTAINER_vadd_OBJS += $(XCLBIN)/$(KERNEL_NAME).$(TARGET).xo

CP = cp -rf

host_change: $(VER)

.PHONY: all clean cleanall docs emconfig
all: check-devices $(host_change) $(EXECUTABLE) $(BINARY_CONTAINERS) emconfig Makefile

.PHONY: exe
exe: $(EXECUTABLE)

# Building kernel
$(XCLBIN)/$(KERNEL_NAME).$(TARGET).xo: src/vadd.cpp
# $(XCLBIN)/$(KERNEL_NAME).$(TARGET).xo: src/vadd.cpp src/first_half.cpp src/second_half.cpp
	mkdir -p $(XCLBIN)
	$(VPP) $(CLFLAGS) --temp_dir $(BUILD_DIR_vadd) -c -k $(KERNEL_NAME) -I'$(<D)' -o'$@' '$<'  --jobs 32 #--from_step vpl.impl.phys_opt_design
$(XCLBIN)/$(KERNEL_NAME).$(TARGET).xclbin: $(BINARY_CONTAINER_vadd_OBJS)
	mkdir -p $(XCLBIN)
	$(VPP) $(CLFLAGS) --temp_dir $(BUILD_DIR_vadd) -l $(LDCLFLAGS) -o'$@' $(+) --jobs 32 #--from_step vpl.impl.phys_opt_design


ifeq ($(VER),host_cpp)
# Building Host
$(EXECUTABLE): $(HOST_CPP_SRCS) $(HOST_CPP_HDRS)
	mkdir -p $(XCLBIN)
	$(CXX) $(CXXFLAGS) $(HOST_CPP_SRCS) $(HOST_CPP_HDRS) -o '$@' $(LDFLAGS)
else
$(EXECUTABLE): $(HOST_C_SRCS) $(HOST_C_HDRS)
	mkdir -p $(XCLBIN)
	$(CXX) $(CXXFLAGS) $(HOST_C_SRCS) $(HOST_C_HDRS) -o '$@' $(LDFLAGS)
endif

emconfig:$(EMCONFIG_DIR)/emconfig.json
$(EMCONFIG_DIR)/emconfig.json:
	emconfigutil --platform $(DEVICE) --od $(EMCONFIG_DIR)

check: all
# e.g. make check TARGET=sw_emu DEVICE=xilinx_u280_xdma_201920_1 VER=host_cpp
ifeq ($(VER),host_cpp)
# sw / hw emulation
ifeq ($(TARGET),$(filter $(TARGET),sw_emu hw_emu))
	$(CP) $(EMCONFIG_DIR)/emconfig.json .
	XCL_EMULATION_MODE=$(TARGET) ./$(EXECUTABLE) $(XCLBIN)/$(KERNEL_NAME).$(TARGET).xclbin
#$(DEVICE)
# hw -> FPGA
else
	 ./$(EXECUTABLE) $(XCLBIN)/$(KERNEL_NAME).$(TARGET).xclbin
#$(DEVICE)
endif
# else, VER!=host_cpp
else
ifeq ($(TARGET),$(filter $(TARGET),sw_emu hw_emu))
	$(CP) $(EMCONFIG_DIR)/emconfig.json .
	XCL_EMULATION_MODE=$(TARGET) ./$(EXECUTABLE) $(XCLBIN)/$(KERNEL_NAME).$(TARGET).xclbin $(DEVICE)
else
	 ./$(EXECUTABLE) $(XCLBIN)/$(KERNEL_NAME).$(TARGET).xclbin $(DEVICE)
endif
endif



# Cleaning stuff
RMDIR = rm -rf

clean:
	-$(RMDIR) $(EXECUTABLE) $(XCLBIN)/{*sw_emu*,*hw_emu*}
	-$(RMDIR) TempConfig system_estimate.xtxt *.rpt
	-$(RMDIR) *.protoinst _v++_* .Xil emconfig.json dltmp* xmltmp* *.log *.jou

cleanall: clean
	-$(RMDIR) $(XCLBIN)
	-$(RMDIR) _x.*


check-devices:
ifndef DEVICE
	$(error DEVICE not set. Please set the DEVICE properly and rerun. Run "make help" for more details.)
endif

check_xrt:
ifndef XILINX_XRT
	$(error XILINX_XRT variable is not set, please set correctly and rerun)
endif


ECHO := @echo

#'estimate' for estimate report generation
#'system' for system report generation
ifneq ($(REPORT), no)
CLFLAGS += --report estimate
CLLDFLAGS += --report system
endif

#Generates profile summary report
ifeq ($(PROFILE), yes)
LDCLFLAGS += --profile_kernel data:all:all:all:all
LDCFLAGS += --profile_kernel  stall:all:all:all:all
LDCFALGS += --profile_kernel exec:all:all:all:all
endif
