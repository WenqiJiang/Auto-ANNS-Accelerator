import yaml

# Load YAML configurations
config_file = open("config.yaml", "r")
config = yaml.load(config_file)

# Load template
template_dir = "./template_files/vadd.cpp" 
template_str = None
with open(template_dir) as f:
    template_str = f.read()

# Fill template
template_fill_dict = dict()

template_fill_dict["HBM_in_vadd_arg"] = ""
template_fill_dict["HBM_in_m_axi"] = ""
template_fill_dict["HBM_in_s_axilite"] = ""
template_fill_dict["load_and_split_PQ_codes_wrapper_arg"] = ""
for i in range(config["HBM_CHANNEL_NUM"]):
	template_fill_dict["HBM_in_vadd_arg"] += \
        "    const ap_uint512_t* HBM_in{i},\n".format(i=i)
	template_fill_dict["HBM_in_m_axi"] += \
        "#pragma HLS INTERFACE m_axi port=HBM_in{i} offset=slave bundle=gmem{i}\n".format(i=i)
	template_fill_dict["HBM_in_s_axilite"] += \
        "#pragma HLS INTERFACE s_axilite port=HBM_in{i}  bundle=control\n".format(i=i)
	template_fill_dict["load_and_split_PQ_codes_wrapper_arg"] += \
        "        HBM_in{i},\n".format(i=i)

for k in template_fill_dict:
    template_str = template_str.replace("<--{}-->".format(k), template_fill_dict[k])
output_str = template_str

# Save generated file
output_dir = "./output_files/vadd.cpp" 
with open(output_dir, "w+") as f:
    f.write(output_str)