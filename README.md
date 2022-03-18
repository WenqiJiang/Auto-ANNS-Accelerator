# Baseline

This 

## Design principle


We fix the max nprobe to 128 (stage 3).

We allow the first half (stage 1, 2, 3, 4) which mainly stands for compute, and the second half (stage 5, 6) consumes more or less the same amount of resources. Stage 1 is relatively cheap, so we keep stage 2, 3, 4 using roughly the same amount of resources (LUT). The PE number of stage 5 and 6 are correlated, thus we just make sure their sum is more or less the same as the first half.

## How to build ANNS accelerator and run it?

First, source the Vitis tool (directory depends on installation). We use 2020.2 here, but 2020.1 should also be fine.

```
source /opt/Xilinx/Vitis/2020.2/settings64.sh 
```

Second, enter one project, e.g., 

```
cd generated_projects/baseline_K_1_60perc
```

Then, build the bitstream. Note that it takes around 10 hours to build it on a power server. Make sure the server is turned on. Consider to use the 'screen' command if you are building bitstream on a remote machine to prevent potantial build termination due to unstable network connections.

```
time make check TARGET=hw DEVICE=xilinx_u280_xdma_201920_3 VER=host_cpp > hw 2>&1
```

If you make any change on the host code, use the following command to recompile the host:

```
g++ -I$XILINX_XRT/include/ -I$XILINX_VIVADO/include/ -Wall -O0 -g -std=c++11 ./src/host.cpp  -o 'host'  -L$XILINX_XRT/lib/ -lOpenCL -lpthread -lrt -lstdc++
```

The generated bitstream is stored as build_dir.hw.xilinx_u280_xdma_201920_3/vadd.xclbin, and the host binary is named 'host'. If you want to enable profiling dur
ing the execution, include xrt.ini in your execution folder (here we already have them under each accelerator folder).

To execute the bitstream, make sure you are on a machine equipping Xilinx Alveo U280 FPGA. Then run the following commmand.

```
./host vadd.xclbin {nlist} {nprobe} {OPQ_enable} {data_dir} {ground_truth_dir}
```

For example,

```
./host vadd.xclbin 8192 17 0 /mnt/scratch/wenqi/saved_npy_data/FPGA_data_SIFT100M_IVF8192,PQ16_12_banks /mnt/scratch/wenqi/saved_npy_data/gnd 
```

To automatically test a bunch of algorithm settings on a bitstream, take a look at auto_perf_test/auto_perf_test.py 
