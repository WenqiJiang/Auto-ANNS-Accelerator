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
template_dir = os.path.join(args.input_dir, "HBM_interconnections.hpp")
template_str = None
with open(template_dir) as f:
    template_str = f.read()

# Fill template
template_fill_dict = dict()

template_fill_dict["load_and_split_PQ_codes_wrapper_arguments"] = ""
template_fill_dict["load_and_split_PQ_codes_wrapper_func_body"] = ""
for i in range(config["HBM_CHANNEL_NUM"]):
	template_fill_dict["load_and_split_PQ_codes_wrapper_arguments"] += \
        "    const ap_uint512_t* HBM_in{},\n".format(i)
	template_fill_dict["load_and_split_PQ_codes_wrapper_func_body"] += \
    """
    load_and_split_PQ_codes<query_num, nprobe>(
        HBM_in{i}, s_start_addr_every_cell_replicated[{i}], 
        s_scanned_entries_every_cell_Load_unit_replicated[{i}], 
        s_scanned_entries_every_cell_Split_unit_replicated[{i}],
        s_single_PQ[{i} * 3 + 0], s_single_PQ[{i} * 3 + 1], s_single_PQ[{i} * 3 + 2]);""".format(i=i)

for k in template_fill_dict:
    template_str = template_str.replace("<--{}-->".format(k), str(template_fill_dict[k]))
output_str = template_str

# Save generated file
output_dir = os.path.join(args.output_dir, "HBM_interconnections.hpp")
with open(output_dir, "w+") as f:
    f.write(output_str)