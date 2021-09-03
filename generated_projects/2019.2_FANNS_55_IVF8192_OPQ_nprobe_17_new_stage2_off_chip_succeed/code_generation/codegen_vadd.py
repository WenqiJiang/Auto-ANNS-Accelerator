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
template_dir = os.path.join(args.input_dir, "vadd.cpp")
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

if config["OPQ_ENABLE"]:
    template_fill_dict["vadd_arg_OPQ_matrix"] = """
    // HBM25: OPQ Matrix
    float* HBM_OPQ_matrix,"""
    template_fill_dict["vadd_m_axi_HBM_OPQ_matrix"] = "#pragma HLS INTERFACE m_axi port=HBM_OPQ_matrix  offset=slave bundle=gmemE"
    template_fill_dict["vadd_s_axilite_HBM_OPQ_matrix"] = "#pragma HLS INTERFACE s_axilite port=HBM_OPQ_matrix  bundle=control"
    template_fill_dict["stage_1_OPQ_preprocessing"] = """
    hls::stream<float> s_OPQ_init;
#pragma HLS stream variable=s_OPQ_init depth=512
// #pragma HLS resource variable=s_OPQ_init core=FIFO_BRAM

    load_OPQ_matrix(HBM_OPQ_matrix, s_OPQ_init);

    hls::stream<float> s_preprocessed_query_vectors;
#pragma HLS stream variable=s_preprocessed_query_vectors depth=512
// #pragma HLS resource variable=s_preprocessed_query_vectors core=FIFO_BRAM

    OPQ_preprocessing<QUERY_NUM>(
        s_OPQ_init,
        s_query_vectors,
        s_preprocessed_query_vectors);

    hls::stream<float> s_preprocessed_query_vectors_lookup_PE;
#pragma HLS stream variable=s_preprocessed_query_vectors_lookup_PE depth=512
// #pragma HLS resource variable=s_preprocessed_query_vectors_lookup_PE core=FIFO_BRAM

    hls::stream<float> s_preprocessed_query_vectors_distance_computation_PE;
#pragma HLS stream variable=s_preprocessed_query_vectors_distance_computation_PE depth=512
// #pragma HLS resource variable=s_preprocessed_query_vectors_distance_computation_PE core=FIFO_BRAM

    broadcast_preprocessed_query_vectors<QUERY_NUM>(
        s_preprocessed_query_vectors,
        s_preprocessed_query_vectors_distance_computation_PE,
        s_preprocessed_query_vectors_lookup_PE);"""
else:
    template_fill_dict["vadd_arg_OPQ_matrix"] = ""
    template_fill_dict["vadd_m_axi_HBM_OPQ_matrix"] = ""
    template_fill_dict["vadd_s_axilite_HBM_OPQ_matrix"] = ""
    template_fill_dict["stage_1_OPQ_preprocessing"] = """
    hls::stream<float> s_preprocessed_query_vectors_lookup_PE;
#pragma HLS stream variable=s_preprocessed_query_vectors_lookup_PE depth=512
// #pragma HLS resource variable=s_preprocessed_query_vectors_lookup_PE core=FIFO_BRAM

    hls::stream<float> s_preprocessed_query_vectors_distance_computation_PE;
#pragma HLS stream variable=s_preprocessed_query_vectors_distance_computation_PE depth=512
// #pragma HLS resource variable=s_preprocessed_query_vectors_distance_computation_PE core=FIFO_BRAM

    broadcast_preprocessed_query_vectors<QUERY_NUM>(
        s_query_vectors,
        s_preprocessed_query_vectors_distance_computation_PE,
        s_preprocessed_query_vectors_lookup_PE);"""

# Stage 2 on-chip / off-chip replacement
template_fill_dict["stage2_vadd_arg"] = ""
template_fill_dict["stage2_m_axi"] = ""
template_fill_dict["stage2_s_axilite"] = ""
template_fill_dict["stage_2_IVF_center_distance_computation"] = """
    compute_cell_distance_wrapper<QUERY_NUM>(
        s_center_vectors_init_distance_computation_PE, 
        s_preprocessed_query_vectors_distance_computation_PE, 
        s_merged_cell_distance);"""
if config["STAGE2_ON_CHIP"] == False:
    func_call_str = ""
    for i in range(config["PE_NUM_CENTER_DIST_COMP"]):
        template_fill_dict["stage2_vadd_arg"] += \
            "    const ap_uint512_t* HBM_centroid_vectors_{i},\n".format(i=i)
        template_fill_dict["stage2_m_axi"] += \
            "#pragma HLS INTERFACE m_axi port=HBM_centroid_vectors_{i}  offset=slave bundle=gmemC{i}\n".format(i=i)
        template_fill_dict["stage2_s_axilite"] += \
            "#pragma HLS INTERFACE s_axilite port=HBM_centroid_vectors_{i}  bundle=control\n".format(i=i)
        func_call_str += "        HBM_centroid_vectors_{i},\n".format(i=i)
    template_fill_dict["stage_2_IVF_center_distance_computation"] = """
    compute_cell_distance_wrapper<QUERY_NUM>(
{func_call_str}
        s_preprocessed_query_vectors_distance_computation_PE,
        s_merged_cell_distance);""".format(func_call_str=func_call_str)

for k in template_fill_dict:
    template_str = template_str.replace("<--{}-->".format(k), str(template_fill_dict[k]))
output_str = template_str

# Save generated file
output_dir = os.path.join(args.output_dir, "vadd.cpp")
with open(output_dir, "w+") as f:
    f.write(output_str)