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
template_dir = os.path.join(args.input_dir, "host.cpp")
template_str = None
with open(template_dir) as f:
    template_str = f.read()

# Fill template
template_fill_dict = dict()

template_fill_dict["QUERY_NUM"] = str(config["QUERY_NUM"])
template_fill_dict["NLIST"] = str(config["NLIST"])
template_fill_dict["D"] = str(config["D"])
template_fill_dict["M"] = str(config["M"])
template_fill_dict["HBM_CHANNEL_NUM"] = str(config["HBM_CHANNEL_NUM"])

# number of 512-bit chunk per bank
size_per_bank = config["DB_BYTES"] / config["HBM_CHANNEL_NUM"] / 64
assert size_per_bank - int(size_per_bank)  == 0
size_per_bank = int(size_per_bank)

template_fill_dict["HBM_embedding_len"] = ""
template_fill_dict["HBM_embedding_size"] = ""
template_fill_dict["HBM_embedding_allocate"] = ""
template_fill_dict["HBM_embedding_char"] = ""
template_fill_dict["HBM_embedding_fstream"] = ""
template_fill_dict["HBM_embedding0_fstream_read"] = ""
template_fill_dict["HBM_embedding_memcpy"] = ""
template_fill_dict["HBM_embedding_char_free"] = ""
template_fill_dict["HBM_embeddingExt"] = ""
template_fill_dict["HBM_embedding0Ext_set"] = ""
template_fill_dict["buffer_HBM_embedding"] = ""
template_fill_dict["buffer_HBM_embedding_set_krnl_arg"] = ""
template_fill_dict["buffer_HBM_embedding_enqueueMigrateMemObjects"] = ""
for i in range(config["HBM_CHANNEL_NUM"]):
    template_fill_dict["HBM_embedding_len"] += \
        "    size_t HBM_embedding{i}_len = {size};\n".format(i=i, size=size_per_bank)
    template_fill_dict["HBM_embedding_size"] += \
        "    size_t HBM_embedding{i}_size =  HBM_embedding{i}_len * sizeof(ap_uint512_t);\n".format(i=i)
    template_fill_dict["HBM_embedding_allocate"] += \
        "    std::vector<ap_uint512_t, aligned_allocator<ap_uint512_t>> HBM_embedding{i}(HBM_embedding{i}_len, 0);\n".format(i=i)
    template_fill_dict["HBM_embedding_char"] += \
        "    char* HBM_embedding{i}_char = (char*) malloc(HBM_embedding{i}_size);\n".format(i=i)
    template_fill_dict["HBM_embedding_fstream"] += \
        '''    std::ifstream HBM_embedding{i}_fstream(
        "{DATA_DIR}/HBM_bank_{i}_raw", 
        std::ios::in | std::ios::binary);\n'''.format(
            DATA_DIR=config["DATA_DIR"], DB_SCALE=config["DB_SCALE"],
            M=config["M"], NLIST=config["NLIST"], HBM_CHANNEL_NUM=config["HBM_CHANNEL_NUM"], i=i)
    template_fill_dict["HBM_embedding0_fstream_read"] += \
        "    HBM_embedding{i}_fstream.read(HBM_embedding{i}_char, HBM_embedding{i}_size);\n".format(i=i)
    template_fill_dict["HBM_embedding_memcpy"] += \
        "    memcpy(&HBM_embedding{i}[0], HBM_embedding{i}_char, HBM_embedding{i}_size);\n".format(i=i)
    template_fill_dict["HBM_embedding_char_free"] += \
        "    free(HBM_embedding{i}_char);\n".format(i=i)
    template_fill_dict["HBM_embeddingExt"] += \
        "        HBM_embedding{i}Ext,\n".format(i=i)
    template_fill_dict["HBM_embedding0Ext_set"] += \
        '''    HBM_embedding{i}Ext.obj = HBM_embedding{i}.data();
    HBM_embedding{i}Ext.param = 0;
    HBM_embedding{i}Ext.flags = bank[{i}];\n'''.format(i=i)
    template_fill_dict["buffer_HBM_embedding"] += \
        '''    OCL_CHECK(err, cl::Buffer buffer_HBM_embedding{i}(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX, 
            HBM_embedding{i}_size, &HBM_embedding{i}Ext, &err));\n'''.format(i=i)
    template_fill_dict["buffer_HBM_embedding_set_krnl_arg"] += \
        "    OCL_CHECK(err, err = krnl_vector_add.setArg({i}, buffer_HBM_embedding{i}));\n".format(i=i)
    template_fill_dict["buffer_HBM_embedding_enqueueMigrateMemObjects"] += \
        "        buffer_HBM_embedding{i},\n".format(i=i)

    template_fill_dict["HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid_fstream"] = \
        '''    std::ifstream HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid_fstream(
        "{DATA_DIR}/HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid_3_by_{NLIST}_raw", 
        std::ios::in | std::ios::binary);\n'''.format(
                DATA_DIR=config["DATA_DIR"], NLIST=config["NLIST"])
    template_fill_dict["HBM_query_vector_fstream"] = \
        '''    std::ifstream HBM_query_vector_fstream(
        "{DATA_DIR}/query_vectors_float32_{QUERY_NUM}_128_raw", 
        std::ios::in | std::ios::binary);\n'''.format(
                DATA_DIR=config["DATA_DIR"], QUERY_NUM=config["QUERY_NUM"])
    template_fill_dict["HBM_vector_quantizer_fstream"] = \
        '''    std::ifstream HBM_vector_quantizer_fstream(
        "{DATA_DIR}/vector_quantizer_float32_{NLIST}_128_raw", 
        std::ios::in | std::ios::binary);\n'''.format(
                DATA_DIR=config["DATA_DIR"], NLIST=config["NLIST"])
    template_fill_dict["HBM_product_quantizer_fstream"] = \
        '''    std::ifstream HBM_product_quantizer_fstream(
        "{DATA_DIR}/product_quantizer_float32_{M}_{K}_{PARTITION}_raw", 
        std::ios::in | std::ios::binary);\n'''.format(
                DATA_DIR=config["DATA_DIR"], M=config["M"],
                K=config["K"],  PARTITION=int(config["D"]/config["M"]))
    template_fill_dict["HBM_OPQ_matrix_fstream"] = \
        '''    std::ifstream HBM_OPQ_matrix_fstream(
        "{DATA_DIR}/OPQ_matrix_float32_{D}_{D}_raw", 
        std::ios::in | std::ios::binary);\n'''.format(
                DATA_DIR=config["DATA_DIR"], D=config["D"])
    template_fill_dict["sw_result_vec_ID_fstream"] = \
        '''    std::ifstream sw_result_vec_ID_fstream(
        "{DATA_DIR}/result_nprobe_{NPROBE}_index_int32_{QUERY_NUM}_10_raw", 
        std::ios::in | std::ios::binary);\n'''.format(
                DATA_DIR=config["DATA_DIR"], NPROBE=config["NPROBE"], QUERY_NUM=config["QUERY_NUM"])
    template_fill_dict["sw_result_dist_fstream"] = \
        '''    std::ifstream sw_result_dist_fstream(
        "{DATA_DIR}/result_nprobe_{NPROBE}_distance_float32_{QUERY_NUM}_10_raw", 
        std::ios::in | std::ios::binary);\n'''.format(
                DATA_DIR=config["DATA_DIR"], NPROBE=config["NPROBE"], QUERY_NUM=config["QUERY_NUM"])


for k in template_fill_dict:
    template_str = template_str.replace("<--{}-->".format(k), str(template_fill_dict[k]))
output_str = template_str

# Save generated file
output_dir = os.path.join(args.output_dir, "host.cpp")
with open(output_dir, "w+") as f:
    f.write(output_str)