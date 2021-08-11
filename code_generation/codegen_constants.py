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
template_dir = os.path.join(args.input_dir, "constants.hpp")
template_str = None
with open(template_dir) as f:
    template_str = f.read()

# Fill template
template_fill_dict = config
template_fill_dict["PE_NUM_CENTER_DIST_COMP_EVEN"] = template_fill_dict["PE_NUM_CENTER_DIST_COMP"] - 1
template_fill_dict["CENTROIDS_PER_PARTITION_EVEN"] = int(np.ceil(
    template_fill_dict["NLIST"] / template_fill_dict["PE_NUM_CENTER_DIST_COMP"]))
template_fill_dict["CENTROIDS_PER_PARTITION_LAST_PE"] = \
    template_fill_dict["NLIST"] - \
    template_fill_dict["PE_NUM_CENTER_DIST_COMP_EVEN"] * template_fill_dict["CENTROIDS_PER_PARTITION_EVEN"]

template_fill_dict["PE_NUM_TABLE_CONSTRUCTION_LARGER"] = template_fill_dict["PE_NUM_TABLE_CONSTRUCTION"] - 1
template_fill_dict["NPROBE_PER_TABLE_CONSTRUCTION_PE_LARGER"] = int(np.ceil(
    template_fill_dict["NPROBE"] / template_fill_dict["PE_NUM_TABLE_CONSTRUCTION"]))
template_fill_dict["PE_NUM_TABLE_CONSTRUCTION_SMALLER"] = 1
template_fill_dict["NPROBE_PER_TABLE_CONSTRUCTION_PE_SMALLER"] = template_fill_dict["NPROBE"] - \
    template_fill_dict["PE_NUM_TABLE_CONSTRUCTION_LARGER"] * template_fill_dict["NPROBE_PER_TABLE_CONSTRUCTION_PE_LARGER"]
# check valid
assert template_fill_dict["NPROBE_PER_TABLE_CONSTRUCTION_PE_SMALLER"] > 0
if config["OPQ_ENABLE"]:
    template_fill_dict["OPQ_ENABLE"] = "#define OPQ_ENABLE 1"
else:
    template_fill_dict["OPQ_ENABLE"] = ""
if config["STAGE2_ON_CHIP"]:
    template_fill_dict["STAGE2_ON_CHIP"] = "#define STAGE2_ON_CHIP 1"
else:
    template_fill_dict["STAGE2_ON_CHIP"] = ""

for k in template_fill_dict:
    template_str = template_str.replace("<--{}-->".format(k), str(template_fill_dict[k]))
output_str = template_str

output_str = template_str

# Save generated file
output_dir = os.path.join(args.output_dir, "constants.hpp")
with open(output_dir, "w+") as f:
    f.write(output_str)