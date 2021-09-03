import argparse 
import os
import yaml
import numpy as np

parser = argparse.ArgumentParser()
parser.add_argument('--input_dir', type=str, default="./template_files/stage2_cluster_distance_computation", help="template input directory")
parser.add_argument('--output_dir', type=str, default="./output_files", help="output directory")
args = parser.parse_args()

# Load YAML configurations
config_file = open("config.yaml", "r")
config = yaml.load(config_file)

assert config["PE_NUM_CENTER_DIST_COMP"] <= 8, \
    "ERROR: Stage 2 PE number should be less than 8 (might be supported in the future), otherwise otherwise the computation will be faster than result forwarding. "
# select stage 2 on-chip / off-chip
if config["STAGE2_ON_CHIP"]:
    os.system("cp {i} {o}".format(
        i=os.path.join(args.input_dir, "on_chip_cluster_distance_computation.hpp"),
        o=os.path.join(args.output_dir, "cluster_distance_computation.hpp")))
else:
    os.system("cp {i} {o}".format(
        i=os.path.join(args.input_dir, "off_chip_cluster_distance_computation.hpp"),
        o=os.path.join(args.output_dir, "cluster_distance_computation.hpp")))
