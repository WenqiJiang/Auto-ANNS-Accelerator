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

if config["DEVICE"] == "U280":
    template_fill_dict["bank_topology"] = """
/* for U280 specifically */
const int bank[40] = {
    /* 0 ~ 31 HBM (256MB per channel) */
    BANK_NAME(0),  BANK_NAME(1),  BANK_NAME(2),  BANK_NAME(3),  BANK_NAME(4),
    BANK_NAME(5),  BANK_NAME(6),  BANK_NAME(7),  BANK_NAME(8),  BANK_NAME(9),
    BANK_NAME(10), BANK_NAME(11), BANK_NAME(12), BANK_NAME(13), BANK_NAME(14),
    BANK_NAME(15), BANK_NAME(16), BANK_NAME(17), BANK_NAME(18), BANK_NAME(19),
    BANK_NAME(20), BANK_NAME(21), BANK_NAME(22), BANK_NAME(23), BANK_NAME(24),
    BANK_NAME(25), BANK_NAME(26), BANK_NAME(27), BANK_NAME(28), BANK_NAME(29),
    BANK_NAME(30), BANK_NAME(31), 
    /* 32, 33 DDR (16GB per channel) */ 
    BANK_NAME(32), BANK_NAME(33), 
    /* 34 ~ 39 PLRAM */ 
    BANK_NAME(34), BANK_NAME(35), BANK_NAME(36), BANK_NAME(37), 
    BANK_NAME(38), BANK_NAME(39)};
    """
elif config["DEVICE"] == "U250":
    template_fill_dict["bank_topology"] = """
/* for U250 specifically */
const int bank[8] = {
    /* 0 ~ 3 DDR (16GB per channel) */
    BANK_NAME(0),  BANK_NAME(1),  BANK_NAME(2),  BANK_NAME(3), 
    /* 4 ~ 7 PLRAM */ 
    BANK_NAME(4), BANK_NAME(5), BANK_NAME(6), BANK_NAME(7)};
    """
elif config["DEVICE"] == "U50":
    template_fill_dict["bank_topology"] = """
/* for U50 specifically */
const int bank[36] = {
    /* 0 ~ 31 HBM (256MB per channel) */
    BANK_NAME(0),  BANK_NAME(1),  BANK_NAME(2),  BANK_NAME(3),  BANK_NAME(4),
    BANK_NAME(5),  BANK_NAME(6),  BANK_NAME(7),  BANK_NAME(8),  BANK_NAME(9),
    BANK_NAME(10), BANK_NAME(11), BANK_NAME(12), BANK_NAME(13), BANK_NAME(14),
    BANK_NAME(15), BANK_NAME(16), BANK_NAME(17), BANK_NAME(18), BANK_NAME(19),
    BANK_NAME(20), BANK_NAME(21), BANK_NAME(22), BANK_NAME(23), BANK_NAME(24),
    BANK_NAME(25), BANK_NAME(26), BANK_NAME(27), BANK_NAME(28), BANK_NAME(29),
    BANK_NAME(30), BANK_NAME(31), 
    /* 32 ~ 35 PLRAM */ 
    BANK_NAME(32), BANK_NAME(33), BANK_NAME(34), BANK_NAME(35)};
    """
else:
    print("Unsupported device! Supported model: U280/U250/U50")
    raise ValueError


template_fill_dict["QUERY_NUM"] = str(config["QUERY_NUM"])
template_fill_dict["NLIST"] = str(config["NLIST"])
template_fill_dict["D"] = str(config["D"])
template_fill_dict["M"] = str(config["M"])
template_fill_dict["HBM_CHANNEL_NUM"] = str(config["HBM_CHANNEL_NUM"])

template_fill_dict["HBM_embedding_len"] = ""
template_fill_dict["HBM_embedding_size"] = ""
template_fill_dict["HBM_embedding_allocate"] = ""
template_fill_dict["HBM_embedding_char"] = ""
template_fill_dict["HBM_embedding_fstream"] = ""
template_fill_dict["HBM_embedding0_fstream_read"] = ""
template_fill_dict["HBM_embedding_memcpy"] = ""
template_fill_dict["HBM_embedding_char_free"] = ""
template_fill_dict["HBM_embeddingExt"] = ""
template_fill_dict["HBM_embeddingExt_set"] = ""
template_fill_dict["buffer_HBM_embedding"] = ""
template_fill_dict["buffer_HBM_embedding_set_krnl_arg"] = ""
template_fill_dict["buffer_HBM_embedding_enqueueMigrateMemObjects"] = ""
for i in range(config["HBM_CHANNEL_NUM"]):
    template_fill_dict["HBM_embedding_size"] += """
    HBM_embedding{i}_fstream.seekg(0, HBM_embedding{i}_fstream.end);
    size_t HBM_embedding{i}_size =  HBM_embedding{i}_fstream.tellg();
    HBM_embedding{i}_fstream.seekg(0, HBM_embedding{i}_fstream.beg);""".format(i=i)
    template_fill_dict["HBM_embedding_len"] += \
        "    size_t HBM_embedding{i}_len = (int) (HBM_embedding{i}_size / sizeof(ap_uint512_t));\n".format(i=i)
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
    template_fill_dict["HBM_embeddingExt_set"] += \
        '''    HBM_embedding{i}Ext.obj = HBM_embedding{i}.data();
    HBM_embedding{i}Ext.param = 0;
    HBM_embedding{i}Ext.flags = bank[{i}];\n'''.format(i=i)
    template_fill_dict["buffer_HBM_embedding"] += \
        '''    OCL_CHECK(err, cl::Buffer buffer_HBM_embedding{i}(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX, 
            HBM_embedding{i}_size, &HBM_embedding{i}Ext, &err));\n'''.format(i=i)
    template_fill_dict["buffer_HBM_embedding_set_krnl_arg"] += \
        "    OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, buffer_HBM_embedding{i}));\n".format(i=i)
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
    template_fill_dict["raw_gt_vec_ID_fstream"] = '''
    std::ifstream raw_gt_vec_ID_fstream(
        "{}", 
        std::ios::in | std::ios::binary);\n'''.format(
            os.path.join(config["GT_DIR"], "idx_{}.ivecs".format(config["DB_SCALE"])))

centroids_per_partition_even = int(np.ceil(
    config["NLIST"] / config["PE_NUM_CENTER_DIST_COMP"]))
bytes_float = 4
bytes_ap512 = 64

template_fill_dict["HBM_centroid_vectors_len"] = ""
template_fill_dict["HBM_centroid_vectors_size"] = ""
template_fill_dict["HBM_centroid_vectors_allocate"] = ""
template_fill_dict["HBM_centroid_vectors_memcpy"] = ""
template_fill_dict["HBM_centroid_vectorsExt"] = ""
template_fill_dict["HBM_centroid_vectorsExt_set"] = ""
template_fill_dict["HBM_metainfoExt_set"] = ""
template_fill_dict["buffer_HBM_centroid_vectors"] = ""
template_fill_dict["buffer_HBM_centroid_vectors_set_krnl_arg"] = ""
template_fill_dict["buffer_HBM_centroid_vectors_enqueueMigrateMemObjects"] = ""
if config["STAGE2_ON_CHIP"] == False:
    for i in range(config["PE_NUM_CENTER_DIST_COMP"]):
        if i != config["PE_NUM_CENTER_DIST_COMP"] - 1:
            template_fill_dict["HBM_centroid_vectors_len"] += \
                "    size_t HBM_centroid_vectors{i}_len = {l};\n".format(
                    i=i, l=int(centroids_per_partition_even * config["D"] * bytes_float / bytes_ap512))
        else:
            centroids_per_partition_last = \
                config["NLIST"] - \
                (config["PE_NUM_CENTER_DIST_COMP"] - 1) * centroids_per_partition_even
            template_fill_dict["HBM_centroid_vectors_len"] += \
                "    size_t HBM_centroid_vectors{i}_len = {l};\n".format(
                    i=i, l=int(centroids_per_partition_last * config["D"] * bytes_float / bytes_ap512))
        template_fill_dict["HBM_centroid_vectors_size"] += \
            "    size_t HBM_centroid_vectors{i}_size =  HBM_centroid_vectors{i}_len * sizeof(ap_uint512_t);\n".format(i=i)
        template_fill_dict["HBM_centroid_vectors_allocate"] += \
            "    std::vector<ap_uint512_t, aligned_allocator<ap_uint512_t>> HBM_centroid_vectors{i}(HBM_centroid_vectors{i}_len, 0);\n".format(i=i)
        template_fill_dict["HBM_centroid_vectors_memcpy"] += \
            "    memcpy(&HBM_centroid_vectors{i}[0], HBM_vector_quantizer_char + {start_addr}, HBM_centroid_vectors{i}_size);\n".format(
                i=i, start_addr=int(i * centroids_per_partition_even * config["D"] * bytes_float)
            )
        template_fill_dict["HBM_centroid_vectorsExt"] += \
            "        HBM_centroid_vectors{i}Ext,\n".format(i=i)
        template_fill_dict["HBM_centroid_vectorsExt_set"] += \
            '''    HBM_centroid_vectors{i}Ext.obj = HBM_centroid_vectors{i}.data();
    HBM_centroid_vectors{i}Ext.param = 0;
    HBM_centroid_vectors{i}Ext.flags = bank[{c}];\n'''.format(i=i, c=i + config["STAGE2_OFF_CHIP_START_CHANNEL"])
        template_fill_dict["buffer_HBM_centroid_vectors"] += \
            '''    OCL_CHECK(err, cl::Buffer buffer_HBM_centroid_vectors{i}(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX, 
            HBM_centroid_vectors{i}_size, &HBM_centroid_vectors{i}Ext, &err));\n'''.format(i=i)
        template_fill_dict["buffer_HBM_centroid_vectors_set_krnl_arg"] += \
            "    OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, buffer_HBM_centroid_vectors{i}));\n".format(i=i)
        template_fill_dict["buffer_HBM_centroid_vectors_enqueueMigrateMemObjects"] += \
            "        buffer_HBM_centroid_vectors{i},\n".format(i=i)

if config["DEVICE"] == "U280" or config["DEVICE"] == "U50":
    template_fill_dict["HBM_metainfoExt_set"] = """
    HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_validExt.obj = 
        HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid.data();
    HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_validExt.param = 0;
    HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_validExt.flags = bank[21];

    HBM_query_vectorExt.obj = HBM_query_vectors.data();
    HBM_query_vectorExt.param = 0;
    HBM_query_vectorExt.flags = bank[22];

    HBM_vector_quantizerExt.obj = HBM_vector_quantizer.data();
    HBM_vector_quantizerExt.param = 0;
    HBM_vector_quantizerExt.flags = bank[23];

    HBM_product_quantizerExt.obj = HBM_product_quantizer.data();
    HBM_product_quantizerExt.param = 0;
    HBM_product_quantizerExt.flags = bank[24];

#ifdef OPQ_ENABLE
    HBM_OPQ_matrixExt.obj = HBM_OPQ_matrix.data();
    HBM_OPQ_matrixExt.param = 0;
    HBM_OPQ_matrixExt.flags = bank[25];
#endif

    HBM_outExt.obj = HBM_out.data();
    HBM_outExt.param = 0;
    HBM_outExt.flags = bank[26];
    """
elif config["DEVICE"] == "U250":
    template_fill_dict["HBM_metainfoExt_set"] = """
    HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_validExt.obj = 
        HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid.data();
    HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_validExt.param = 0;
    HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_validExt.flags = bank[0];

    HBM_query_vectorExt.obj = HBM_query_vectors.data();
    HBM_query_vectorExt.param = 0;
    HBM_query_vectorExt.flags = bank[1];

    HBM_vector_quantizerExt.obj = HBM_vector_quantizer.data();
    HBM_vector_quantizerExt.param = 0;
    HBM_vector_quantizerExt.flags = bank[2];

    HBM_product_quantizerExt.obj = HBM_product_quantizer.data();
    HBM_product_quantizerExt.param = 0;
    HBM_product_quantizerExt.flags = bank[3];

#ifdef OPQ_ENABLE
    HBM_OPQ_matrixExt.obj = HBM_OPQ_matrix.data();
    HBM_OPQ_matrixExt.param = 0;
    HBM_OPQ_matrixExt.flags = bank[0];
#endif

    HBM_outExt.obj = HBM_out.data();
    HBM_outExt.param = 0;
    HBM_outExt.flags = bank[2];
    """

for k in template_fill_dict:
    template_str = template_str.replace("<--{}-->".format(k), str(template_fill_dict[k]))
output_str = template_str

# Save generated file
output_dir = os.path.join(args.output_dir, "host.cpp")
with open(output_dir, "w+") as f:
    f.write(output_str)