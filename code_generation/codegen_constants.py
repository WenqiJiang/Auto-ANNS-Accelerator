import yaml
import numpy as np

# Load YAML configurations
config_file = open("config.yaml", "r")
config = yaml.load(config_file)

# Load template
template_dir = "./template_files/constants.hpp" 
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

for k in template_fill_dict:
    template_fill_dict[k] = str(template_fill_dict[k])

template_str = template_str.replace(
    "<--NLIST-->", template_fill_dict["NLIST"])
template_str = template_str.replace(
    "<--NPROBE-->", template_fill_dict["NPROBE"])
template_str = template_str.replace(
    "<--D-->", template_fill_dict["D"])
template_str = template_str.replace(
    "<--M-->", template_fill_dict["M"])
template_str = template_str.replace(
    "<--K-->", template_fill_dict["K"])
template_str = template_str.replace(
    "<--PRIORITY_QUEUE_LEN-->", template_fill_dict["PRIORITY_QUEUE_LEN"])
template_str = template_str.replace(
    "<--QUERY_NUM-->", template_fill_dict["QUERY_NUM"])
template_str = template_str.replace(
    "<--LARGE_NUM-->", template_fill_dict["LARGE_NUM"])
template_str = template_str.replace(
    "<--PE_NUM_CENTER_DIST_COMP-->", template_fill_dict["PE_NUM_CENTER_DIST_COMP"])
template_str = template_str.replace(
    "<--PE_NUM_CENTER_DIST_COMP_EVEN-->", template_fill_dict["PE_NUM_CENTER_DIST_COMP_EVEN"])
template_str = template_str.replace(
    "<--CENTROIDS_PER_PARTITION_EVEN-->", template_fill_dict["CENTROIDS_PER_PARTITION_EVEN"])
template_str = template_str.replace(
    "<--CENTROIDS_PER_PARTITION_LAST_PE-->", template_fill_dict["CENTROIDS_PER_PARTITION_LAST_PE"])
template_str = template_str.replace(
    "<--STAGE_3_PRIORITY_QUEUE_LEVEL-->", template_fill_dict["STAGE_3_PRIORITY_QUEUE_LEVEL"])
template_str = template_str.replace(
    "<--STAGE_3_PRIORITY_QUEUE_L1_NUM-->", template_fill_dict["STAGE_3_PRIORITY_QUEUE_L1_NUM"])
template_str = template_str.replace(
    "<--PE_NUM_TABLE_CONSTRUCTION-->", template_fill_dict["PE_NUM_TABLE_CONSTRUCTION"])
template_str = template_str.replace(
    "<--PE_NUM_TABLE_CONSTRUCTION_LARGER-->", template_fill_dict["PE_NUM_TABLE_CONSTRUCTION_LARGER"])
template_str = template_str.replace(
    "<--PE_NUM_TABLE_CONSTRUCTION_SMALLER-->", template_fill_dict["PE_NUM_TABLE_CONSTRUCTION_SMALLER"])
template_str = template_str.replace(
    "<--NPROBE_PER_TABLE_CONSTRUCTION_PE_LARGER-->", template_fill_dict["NPROBE_PER_TABLE_CONSTRUCTION_PE_LARGER"])
template_str = template_str.replace(
    "<--NPROBE_PER_TABLE_CONSTRUCTION_PE_SMALLER-->", template_fill_dict["NPROBE_PER_TABLE_CONSTRUCTION_PE_SMALLER"])
template_str = template_str.replace(
    "<--HBM_CHANNEL_NUM-->", template_fill_dict["HBM_CHANNEL_NUM"])
template_str = template_str.replace(
    "<--SORT_GROUP_NUM-->", template_fill_dict["SORT_GROUP_NUM"])

output_str = template_str

# Save generated file
output_dir = "./output_files/constants.hpp" 
with open(output_dir, "w+") as f:
    f.write(output_str)