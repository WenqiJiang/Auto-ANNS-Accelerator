# Auto-ANNS-Accelerator

## Folder Structure

### PE_optimization

The PEs used in this project.

### performance_model

Function 1: Given a database, an FPGA device, a resource constraint, and a frequency setting, search the parameter space (nlist, nprobe, OPQ_enable) and find the best hardware solution.

Function 2: Input a set of hardware settings (config.yaml), predict the performance and resource consumption by the performance model.

### code_generation

Given an input FPGA setting, automatically generate the project.

### generated_projects

The FPGA projects generated by code generator. They are grouped by categories.

To build them, first source Vitis HLS 2020.2 (depending on the install directory):

```
source /opt/Xilinx/Vitis/2020.2/settings64.sh 
```

Then, build the bitstream:

```
time make check TARGET=hw DEVICE=xilinx_u280_xdma_201920_3 VER=host_cpp > hw 2>&1
```

If you make any change on the host code, use the following command to recompile the host:

```
g++ -I$XILINX_XRT/include/ -I$XILINX_VIVADO/include/ -Wall -O0 -g -std=c++11 ./src/host.cpp  -o 'host'  -L$XILINX_XRT/lib/ -lOpenCL -lpthread -lrt -lstdc++
```

To run the bitstream and get the performance, use the test script within each folder:

```
python perf_test.py <--config_dir ... --data_parent_dir ... --gt_dir ...>
```

Or if you just want to execute the bitstream without automatically computing the throughput (which requires loading the profile_summary.csv):

```
./host <XCLBIN File> <data directory> <ground truth dir>
# e.g., ./host vadd.xclbin /mnt/scratch/wenqi/saved_npy_data/FPGA_data_SIFT100M_OPQ16,IVF8192,PQ16_16_banks /mnt/scratch/wenqi/saved_npy_data/gnd
```

### reference_code

Unused. The reference human-coded project. Used as a reference when writing the codegen module.

### auto_perf_test

Seems unused in this project. Needed for the baseline branch.


