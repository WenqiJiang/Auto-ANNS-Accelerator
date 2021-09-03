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

template_fill_dict["FREQ"] = str(config["FREQ"])

template_fill_dict["sp_memory_channel"] = ""
for i in range(config["HBM_CHANNEL_NUM"]):
	template_fill_dict["sp_memory_channel"] += \
        "sp=vadd_1.HBM_in{i}:HBM[{i}]\n".format(i=i)

if config["OPQ_ENABLE"]:
    template_fill_dict["connectivity_HBM_OPQ_matrix"] = "sp=vadd_1.HBM_OPQ_matrix:HBM[25]"
else:
    template_fill_dict["connectivity_HBM_OPQ_matrix"] = ""

for k in template_fill_dict:
    template_str = template_str.replace("<--{}-->".format(k), str(template_fill_dict[k]))
output_str = template_str

# Save generated file
output_dir = os.path.join(args.output_dir, "design.cfg")
with open(output_dir, "w+") as f:
    f.write(output_str)