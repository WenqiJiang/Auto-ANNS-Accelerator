import argparse 
import os
import yaml
import numpy as np

parser = argparse.ArgumentParser()
parser.add_argument('--input_dir', type=str, default="./template_files", help="template input directory")
parser.add_argument('--output_dir', type=str, default="./output_files", help="output directory")
args = parser.parse_args()

# Load YAML configurations
config_file = open("config.yaml", "r")
config = yaml.load(config_file)

# Load template
template_dir = os.path.join(args.input_dir, "design.cfg")
template_str = None
with open(template_dir) as f:
    template_str = f.read()

# Fill template
template_fill_dict = dict()

if config["DEVICE"] == "U280":
    template_fill_dict["DEVICE_NAME"] = "xilinx_u280_xdma_201920_3"
elif config["DEVICE"]== "U250":
    # here we use the xdma shell, there are several alternative versions including the QMDA shell
    template_fill_dict["DEVICE_NAME"] = "xilinx_u250_xdma_201830_2"
elif config["DEVICE"]== "U50":
    template_fill_dict["DEVICE_NAME"] = "xilinx_u50_gen3x16_xdma_201920_3"
else:
    print("Unsupported device! Supported models: Xilinx Alveo U280/U250/U50")
    raise ValueError

template_fill_dict["FREQ"] = str(config["FREQ"])

# U280 / U50 has HBM
if config["DEVICE"] == "U280" or config["DEVICE"] == "U50":

    template_fill_dict["sp_memory_channel"] = ""
    for i in range(config["HBM_CHANNEL_NUM"]):
        template_fill_dict["sp_memory_channel"] += \
            "sp=vadd_1.HBM_in{i}:HBM[{i}]\n".format(i=i)

    if config["STAGE2_ON_CHIP"]:
        template_fill_dict["stage2_memory_channel"] = ""
    else:
        connectivity_str = "" 
        for i in range(config["PE_NUM_CENTER_DIST_COMP"]):
            connectivity_str += "sp=vadd_1.HBM_centroid_vectors_{i}:HBM[{c}]\n".format(
                i=i, c=int(i + config["STAGE2_OFF_CHIP_START_CHANNEL"]))
        template_fill_dict["stage2_memory_channel"] = connectivity_str

    # Store meta info in some HBM channels
    template_fill_dict["meta_info_memory_channel"] = """
sp=vadd_1.HBM_addr_info:HBM[21]
sp=vadd_1.HBM_query_vectors:HBM[22]
sp=vadd_1.HBM_vector_quantizer:HBM[23]
sp=vadd_1.HBM_product_quantizer:HBM[24]
    """
    if config["OPQ_ENABLE"]:
        template_fill_dict["connectivity_HBM_OPQ_matrix"] = "sp=vadd_1.HBM_OPQ_matrix:HBM[25]"
    else:
        template_fill_dict["connectivity_HBM_OPQ_matrix"] = ""

    template_fill_dict["output_memory_channel"] = "sp=vadd_1.HBM_out:HBM[26]"
elif config["DEVICE"]== "U250":
    assert config["HBM_CHANNEL_NUM"] <= 4, "U250 only has 4 DDR channels"
    if not config["STAGE2_ON_CHIP"]:
        assert config["HBM_CHANNEL_NUM"] + config["PE_NUM_CENTER_DIST_COMP"] <= 4, \
            "U250 only has 4 DDR channels"

    template_fill_dict["sp_memory_channel"] = ""
    for i in range(config["HBM_CHANNEL_NUM"]):
        template_fill_dict["sp_memory_channel"] += \
            "sp=vadd_1.HBM_in{i}:DDR[{i}]\n".format(i=i)

    if config["STAGE2_ON_CHIP"]:
        template_fill_dict["stage2_memory_channel"] = ""
    else:
        connectivity_str = "" 
        for i in range(config["PE_NUM_CENTER_DIST_COMP"]):
            connectivity_str += "sp=vadd_1.HBM_centroid_vectors_{i}:DDR[{c}]\n".format(
                i=i, c=int(i + config["STAGE2_OFF_CHIP_START_CHANNEL"]))
        template_fill_dict["stage2_memory_channel"] = connectivity_str

    # Store meta info in some HBM channels
    template_fill_dict["meta_info_memory_channel"] = """
sp=vadd_1.HBM_addr_info:DDR[0]
sp=vadd_1.HBM_query_vectors:DDR[1]
sp=vadd_1.HBM_vector_quantizer:DDR[2]
sp=vadd_1.HBM_product_quantizer:DDR[3]
    """
    if config["OPQ_ENABLE"]:
        template_fill_dict["connectivity_HBM_OPQ_matrix"] = "sp=vadd_1.HBM_OPQ_matrix:DDR[0]"
    else:
        template_fill_dict["connectivity_HBM_OPQ_matrix"] = ""

    template_fill_dict["output_memory_channel"] = "sp=vadd_1.HBM_out:DDR[2]"
else:
    print("Unsupported device! Supported models: Xilinx Alveo U280/U250/U50")
    raise ValueError

for k in template_fill_dict:
    template_str = template_str.replace("<--{}-->".format(k), str(template_fill_dict[k]))
output_str = template_str

# Save generated file
output_dir = os.path.join(args.output_dir, "design.cfg")
with open(output_dir, "w+") as f:
    f.write(output_str)