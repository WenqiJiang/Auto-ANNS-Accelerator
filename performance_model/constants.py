from common import *

""" Performannce related constants """
FREQ = 140 * 1e6

""" Algorithm related constants """
TOTAL_VECTORS = 1e8

D = 128
topK = 10
K = 256 # 256 row per LUT


""" A number of constant consumption components that should be added to PEs"""

# FIFO depth=512, width=32 bit
resource_FIFO_d512_w32 = Resource()
resource_FIFO_d512_w32.LUT = 54
resource_FIFO_d512_w32.FF = 95
resource_FIFO_d512_w32.BRAM_18K = 2 * 0.5
resource_FIFO_d512_w32.URAM = 0
resource_FIFO_d512_w32.DSP48E = 0
resource_FIFO_d512_w32.HBM_bank = 0

# depth 512, width = 512 = 16 * FIFO_d512_w32
resource_FIFO_d512_w512 = Resource()
resource_FIFO_d512_w512.LUT = 16 * 54
resource_FIFO_d512_w512.FF = 16 * 95
resource_FIFO_d512_w512.BRAM_18K = 16 * 2 * 0.5
resource_FIFO_d512_w512.URAM = 16 * 0
resource_FIFO_d512_w512.DSP48E = 16 * 0
resource_FIFO_d512_w512.HBM_bank = 16 * 0

# FIFO depth=2, width=8 bit
resource_FIFO_d2_w8 = Resource()
resource_FIFO_d2_w8.LUT = 20
resource_FIFO_d2_w8.FF = 6
resource_FIFO_d2_w8.BRAM_18K = 2 * 0
resource_FIFO_d2_w8.URAM = 0
resource_FIFO_d2_w8.DSP48E = 0
resource_FIFO_d2_w8.HBM_bank = 0

# FIFO depth=2, width=32 bit
resource_FIFO_d2_w32 = Resource()
resource_FIFO_d2_w32.LUT = 30
resource_FIFO_d2_w32.FF = 6
resource_FIFO_d2_w32.BRAM_18K = 2 * 0
resource_FIFO_d2_w32.URAM = 0
resource_FIFO_d2_w32.DSP48E = 0
resource_FIFO_d2_w32.HBM_bank = 0

# FIFO depth=2, width=512 bit
resource_FIFO_d2_w512 = Resource()
resource_FIFO_d2_w512.LUT = 484
resource_FIFO_d2_w512.FF = 964
resource_FIFO_d2_w512.BRAM_18K = 2 * 0
resource_FIFO_d2_w512.URAM = 0
resource_FIFO_d2_w512.DSP48E = 0
resource_FIFO_d2_w512.HBM_bank = 0

#####     Shell     #####
resource_network_kernel = Resource()
resource_network_kernel.LUT = 126540
resource_network_kernel.FF = 197124
resource_network_kernel.BRAM_18K = 2 * 430
resource_network_kernel.URAM = 9
resource_network_kernel.DSP48E = 0
resource_network_kernel.HBM_bank = 0

resource_network_user_kernel_functions = Resource()
resource_network_user_kernel_functions.LUT = 11242
resource_network_user_kernel_functions.FF = 5124
resource_network_user_kernel_functions.BRAM_18K = 2 * 0.5
resource_network_user_kernel_functions.URAM = 0
resource_network_user_kernel_functions.DSP48E = 0
resource_network_user_kernel_functions.HBM_bank = 0
resource_network_user_kernel_functions.add_resource(resource_FIFO_d2_w32, num=21)
resource_network_user_kernel_functions.add_resource(resource_FIFO_d512_w32, num=32)

resource_cmac_kernel = Resource()
resource_cmac_kernel.LUT = 17256
resource_cmac_kernel.FF = 58280
resource_cmac_kernel.BRAM_18K = 2 * 18
resource_cmac_kernel.URAM = 9
resource_cmac_kernel.DSP48E = 0
resource_cmac_kernel.HBM_bank = 0

resource_hmss = Resource()
resource_hmss.LUT = 55643 
resource_hmss.FF = 103037
resource_hmss.BRAM_18K = 2 * 4
resource_hmss.URAM = 0
resource_hmss.DSP48E = 0
resource_hmss.HBM_bank = 0

resource_System_DPA = Resource()
resource_System_DPA.LUT = 35738
resource_System_DPA.FF = 76789
resource_System_DPA.BRAM_18K = 2 * 16
resource_System_DPA.URAM = 0
resource_System_DPA.DSP48E = 0
resource_System_DPA.HBM_bank = 0

resource_xdma = Resource()
resource_xdma.LUT = 9100
resource_xdma.FF = 15572
resource_xdma.BRAM_18K = 2 * 0
resource_xdma.URAM = 0
resource_xdma.DSP48E = 0
resource_xdma.HBM_bank = 0

resourece_static_region = Resource()
resourece_static_region.LUT = 93280
resourece_static_region.FF = 128746
resourece_static_region.BRAM_18K = 2 * 200
resourece_static_region.URAM = 0
resourece_static_region.DSP48E = 4
resourece_static_region.HBM_bank = 0

component_list_shell = [resource_network_kernel, resource_network_user_kernel_functions,
    resource_cmac_kernel, resource_hmss, resource_System_DPA, resource_xdma, resourece_static_region]

shell_consumption = sum_resource(component_list_shell)

""" Resource related constants """
MAX_UTIL_PERC = 0.8

TOTAL_BRAM_18K = 4032 
TOTAL_DSP48E = 9024
TOTAL_FF = 2607360 
TOTAL_LUT = 1303680
TOTAL_URAM = 960

MAX_HBM_bank = 32 - 2 - 2 - 1 # reserve 30, 31 unused due to their overlap with PCIe; 2 for Network; 1 for value init
MAX_BRAM_18K = TOTAL_BRAM_18K * MAX_UTIL_PERC
MAX_DSP48E = TOTAL_DSP48E * MAX_UTIL_PERC
MAX_FF = TOTAL_FF * MAX_UTIL_PERC
MAX_LUT = TOTAL_LUT * MAX_UTIL_PERC
MAX_URAM = TOTAL_URAM * MAX_UTIL_PERC

# 1 Bank = 256 MB = 4194304 512-bit = 4194304 * 3 = 12582912 vectors
# 100M / 12582912 = 7.94 (without considering padding)
MIN_HBM_bank = 9 # at least 9 banks to hold PQ16 version


"""
An example of expected scanned ratio of a single index

  e.g., suppose the query vectors has the same distribution as the trained vectors, 
    then the larger a Voronoi cell, the more likely they will be searched
  e.g., searching 32 cells over 8192 in 100 M dataset will not scan 32 / 8192 * 1e8 entries on average,
    we need to scan more
"""

scan_ratio_with_OPQ = {
    1024: 1.102495894347366,
    2048: 1.12463916710666,
    4096: 1.12302396550103,
    8192: 1.135891773928242,
    16384: 1.1527141392580655,
    32768: 1.1441353378627621,
    65536: 1.1411144965226643,
    131072: 1.1476783059960072,
    262144: 1.1543383003102523
}


scan_ratio_without_OPQ = {
    1024: 1.1023307648983034,
    2048: 1.1245342465011723,
    4096: 1.1230564521721877,
    8192: 1.135866022841546, 
    16384: 1.1523836603564073, 
    32768: 1.1440334275739672,
    65536: 1.1410689577844846,
    131072: 1.1476378583040157,
    262144: 1.1543274466049378
}
