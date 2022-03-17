/*
Usage: 
    ./host <XCLBIN File> <nlist> <nprobe> <OPQ_enable> <data directory> <ground truth dir>
Example
    ./host vadd.xclbin 8192 17 1 /mnt/scratch/wenqi/saved_npy_data/FPGA_data_SIFT100M_OPQ16,IVF8192,PQ16_16_banks /mnt/scratch/wenqi/saved_npy_data/gnd
*/

/*
Variable to be replaced (<--variable_name-->):
    multiple lines (depends on HBM channel num):
        HBM_embedding_len    # number of 512-bit chunk in each bank
        HBM_embedding_size
        HBM_embedding_allocate
        HBM_embedding_char
        HBM_embedding_fstream
        HBM_embedding_memcpy
        HBM_embedding_char_free
        HBM_embeddingExt
        HBM_embeddingExt_set
        buffer_HBM_embedding

    multiple lines (depends on stage 2 PE num / on or off-chip):
        HBM_centroid_vectors_stage2_len
        HBM_centroid_vectors_stage2_size
        HBM_centroid_vectors_stage2_allocate
        HBM_centroid_vectors_stage2_memcpy
        HBM_centroid_vectorsExt
        HBM_centroid_vectorsExt_set
        HBM_metainfoExt_set
        buffer_HBM_centroid_vectors
        buffer_HBM_centroid_vectors_stage2_set_krnl_arg
        buffer_HBM_centroid_vectors_stage2_enqueueMigrateMemObjects

    single line:
        bank_topology
        HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid_fstream
        HBM_query_vector_fstream
        HBM_vector_quantizer_fstream
        HBM_product_quantizer_fstream
        HBM_OPQ_matrix_fstream
        sw_result_vec_ID_fstream
        sw_result_dist_fstream

    basic constants:
        QUERY_NUM
        NLIST
        D
        M
*/

#include "constants.hpp"
#include "types.hpp"

#include <algorithm>
#include <vector>
#include <unistd.h>
#include <limits>
#include <iostream>
#include <fstream>

#include "xcl2.hpp"

#define BANK_NAME(n) n | XCL_MEM_TOPOLOGY
// memory topology:  https://www.xilinx.com/html_docs/xilinx2021_1/vitis_doc/optimizingperformance.html#utc1504034308941
// <id> | XCL_MEM_TOPOLOGY
// The <id> is determined by looking at the Memory Configuration section in the xxx.xclbin.info file generated next to the xxx.xclbin file. 
// In the xxx.xclbin.info file, the global memory (DDR, HBM, PLRAM, etc.) is listed with an index representing the <id>.

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
    


std::vector<cl::Device> get_devices(const std::string& vendor_name) {

    size_t i;
    cl_int err;
    std::vector<cl::Platform> platforms;
    OCL_CHECK(err, err = cl::Platform::get(&platforms));
    cl::Platform platform;
    for (i  = 0 ; i < platforms.size(); i++){
        platform = platforms[i];
        OCL_CHECK(err, std::string platformName = platform.getInfo<CL_PLATFORM_NAME>(&err));
        if (platformName == vendor_name){
            std::cout << "Found Platform" << std::endl;
            std::cout << "Platform Name: " << platformName.c_str() << std::endl;
            break;
        }
    }
    if (i == platforms.size()) {
        std::cout << "Error: Failed to find Xilinx platform" << std::endl;
        exit(EXIT_FAILURE);
    }
   
    //Getting ACCELERATOR Devices and selecting 1st such device 
    std::vector<cl::Device> devices;
    OCL_CHECK(err, err = platform.getDevices(CL_DEVICE_TYPE_ACCELERATOR, &devices));
    return devices;
}
   
char* read_binary_file(const std::string &xclbin_file_name, unsigned &nb) 
{
    std::cout << "INFO: Reading " << xclbin_file_name << std::endl;

	if(access(xclbin_file_name.c_str(), R_OK) != 0) {
		printf("ERROR: %s xclbin not available please build\n", xclbin_file_name.c_str());
		exit(EXIT_FAILURE);
	}
    //Loading XCL Bin into char buffer 
    std::cout << "Loading: '" << xclbin_file_name.c_str() << "'\n";
    std::ifstream bin_file(xclbin_file_name.c_str(), std::ifstream::binary);
    bin_file.seekg (0, bin_file.end);
    nb = bin_file.tellg();
    bin_file.seekg (0, bin_file.beg);
    char *buf = new char [nb];
    bin_file.read(buf, nb);
    return buf;
}

// boost::filesystem does not compile well, so implement this myself
std::string dir_concat(std::string dir1, std::string dir2) {
    if (dir1.back() != '/') {
        dir1 += '/';
    }
    return dir1 + dir2;
}

int main(int argc, char** argv)
{
    if (argc != 7) {
        std::cout << "Usage: " << argv[0] << " <XCLBIN File> <nlist> <nprobe> <OPQ_enable> <data directory> <ground truth dir>" << std::endl;
		return EXIT_FAILURE;
	}

    std::string binaryFile = argv[1];

    int nlist = std::stoi(argv[2]);
    int nprobe = std::stoi(argv[3]);
    bool OPQ_enable = (bool) std::stoi(argv[4]);

    std::string data_dir_prefix = argv[5];
    std::string gnd_dir = argv[6];

    std::cout << "nlist: " << nlist << std::endl <<
        "nprobe: " << nprobe << std::endl <<
        "OPQ enable: " << OPQ_enable << std::endl <<
        "data directory" << data_dir_prefix << std::endl <<
        "ground truth directory" << gnd_dir << std::endl;

    // inferred parameters giving input parameters
    int centroids_per_partition_even = ceil(float(nlist) / float(PE_NUM_CENTER_DIST_COMP));
    int centroids_per_partition_last_PE = nlist - centroids_per_partition_even * (PE_NUM_CENTER_DIST_COMP - 1);

    int nprobe_stage4 = nprobe;
    int nprobe_per_table_construction_pe_larger = -1;
    int nprobe_per_table_construction_pe_smaller = -1;
    while (nprobe_per_table_construction_pe_smaller < 1) {
        nprobe_per_table_construction_pe_larger = ceil(float(nprobe_stage4) / float(PE_NUM_TABLE_CONSTRUCTION));
        nprobe_per_table_construction_pe_smaller = 
            nprobe_stage4 - PE_NUM_TABLE_CONSTRUCTION_LARGER * nprobe_per_table_construction_pe_larger;
        if (nprobe_per_table_construction_pe_smaller < 1) {
            nprobe_stage4++;
            std::cout << "Increasing nprobe_stage4 due to stage 4 hardware compatibility reason," <<
                "current nprobe_stage4: " << nprobe_stage4 << std::endl;
        }
    }
    if (PE_NUM_TABLE_CONSTRUCTION == 1) {
        nprobe_per_table_construction_pe_smaller = nprobe_per_table_construction_pe_larger;
    }

    std::cout << "Inferred parameters:" << std::endl <<
         "centroids_per_partition_even: " << centroids_per_partition_even << std::endl <<
         "centroids_per_partition_last_PE: " << centroids_per_partition_last_PE << std::endl <<
         "nprobe_per_table_construction_pe_larger: " << nprobe_per_table_construction_pe_larger << std::endl <<
         "nprobe_per_table_construction_pe_smaller: " << nprobe_per_table_construction_pe_smaller << std::endl;

//////////////////////////////   TEMPLATE START  //////////////////////////////
    
    
    std::string HBM_embedding0_dir_suffix("HBM_bank_0_raw");
    std::string HBM_embedding0_dir = dir_concat(data_dir_prefix, HBM_embedding0_dir_suffix);
    std::ifstream HBM_embedding0_fstream(
        HBM_embedding0_dir, 
        std::ios::in | std::ios::binary);
    
    std::string HBM_embedding1_dir_suffix("HBM_bank_1_raw");
    std::string HBM_embedding1_dir = dir_concat(data_dir_prefix, HBM_embedding1_dir_suffix);
    std::ifstream HBM_embedding1_fstream(
        HBM_embedding1_dir, 
        std::ios::in | std::ios::binary);
    
    std::string HBM_embedding2_dir_suffix("HBM_bank_2_raw");
    std::string HBM_embedding2_dir = dir_concat(data_dir_prefix, HBM_embedding2_dir_suffix);
    std::ifstream HBM_embedding2_fstream(
        HBM_embedding2_dir, 
        std::ios::in | std::ios::binary);
    
    std::string HBM_embedding3_dir_suffix("HBM_bank_3_raw");
    std::string HBM_embedding3_dir = dir_concat(data_dir_prefix, HBM_embedding3_dir_suffix);
    std::ifstream HBM_embedding3_fstream(
        HBM_embedding3_dir, 
        std::ios::in | std::ios::binary);
    
    std::string HBM_embedding4_dir_suffix("HBM_bank_4_raw");
    std::string HBM_embedding4_dir = dir_concat(data_dir_prefix, HBM_embedding4_dir_suffix);
    std::ifstream HBM_embedding4_fstream(
        HBM_embedding4_dir, 
        std::ios::in | std::ios::binary);
    
    std::string HBM_embedding5_dir_suffix("HBM_bank_5_raw");
    std::string HBM_embedding5_dir = dir_concat(data_dir_prefix, HBM_embedding5_dir_suffix);
    std::ifstream HBM_embedding5_fstream(
        HBM_embedding5_dir, 
        std::ios::in | std::ios::binary);
    
    std::string HBM_embedding6_dir_suffix("HBM_bank_6_raw");
    std::string HBM_embedding6_dir = dir_concat(data_dir_prefix, HBM_embedding6_dir_suffix);
    std::ifstream HBM_embedding6_fstream(
        HBM_embedding6_dir, 
        std::ios::in | std::ios::binary);
    
    std::string HBM_embedding7_dir_suffix("HBM_bank_7_raw");
    std::string HBM_embedding7_dir = dir_concat(data_dir_prefix, HBM_embedding7_dir_suffix);
    std::ifstream HBM_embedding7_fstream(
        HBM_embedding7_dir, 
        std::ios::in | std::ios::binary);
    
    std::string HBM_embedding8_dir_suffix("HBM_bank_8_raw");
    std::string HBM_embedding8_dir = dir_concat(data_dir_prefix, HBM_embedding8_dir_suffix);
    std::ifstream HBM_embedding8_fstream(
        HBM_embedding8_dir, 
        std::ios::in | std::ios::binary);
    
    std::string HBM_embedding9_dir_suffix("HBM_bank_9_raw");
    std::string HBM_embedding9_dir = dir_concat(data_dir_prefix, HBM_embedding9_dir_suffix);
    std::ifstream HBM_embedding9_fstream(
        HBM_embedding9_dir, 
        std::ios::in | std::ios::binary);
    
    std::string HBM_embedding10_dir_suffix("HBM_bank_10_raw");
    std::string HBM_embedding10_dir = dir_concat(data_dir_prefix, HBM_embedding10_dir_suffix);
    std::ifstream HBM_embedding10_fstream(
        HBM_embedding10_dir, 
        std::ios::in | std::ios::binary);
    
    std::string HBM_embedding11_dir_suffix("HBM_bank_11_raw");
    std::string HBM_embedding11_dir = dir_concat(data_dir_prefix, HBM_embedding11_dir_suffix);
    std::ifstream HBM_embedding11_fstream(
        HBM_embedding11_dir, 
        std::ios::in | std::ios::binary);


    HBM_embedding0_fstream.seekg(0, HBM_embedding0_fstream.end);
    size_t HBM_embedding0_size =  HBM_embedding0_fstream.tellg();
    if (!HBM_embedding0_size) std::cout << "HBM_embedding0_size is 0!";
    HBM_embedding0_fstream.seekg(0, HBM_embedding0_fstream.beg);
    HBM_embedding1_fstream.seekg(0, HBM_embedding1_fstream.end);
    size_t HBM_embedding1_size =  HBM_embedding1_fstream.tellg();
    if (!HBM_embedding1_size) std::cout << "HBM_embedding1_size is 0!";
    HBM_embedding1_fstream.seekg(0, HBM_embedding1_fstream.beg);
    HBM_embedding2_fstream.seekg(0, HBM_embedding2_fstream.end);
    size_t HBM_embedding2_size =  HBM_embedding2_fstream.tellg();
    if (!HBM_embedding2_size) std::cout << "HBM_embedding2_size is 0!";
    HBM_embedding2_fstream.seekg(0, HBM_embedding2_fstream.beg);
    HBM_embedding3_fstream.seekg(0, HBM_embedding3_fstream.end);
    size_t HBM_embedding3_size =  HBM_embedding3_fstream.tellg();
    if (!HBM_embedding3_size) std::cout << "HBM_embedding3_size is 0!";
    HBM_embedding3_fstream.seekg(0, HBM_embedding3_fstream.beg);
    HBM_embedding4_fstream.seekg(0, HBM_embedding4_fstream.end);
    size_t HBM_embedding4_size =  HBM_embedding4_fstream.tellg();
    if (!HBM_embedding4_size) std::cout << "HBM_embedding4_size is 0!";
    HBM_embedding4_fstream.seekg(0, HBM_embedding4_fstream.beg);
    HBM_embedding5_fstream.seekg(0, HBM_embedding5_fstream.end);
    size_t HBM_embedding5_size =  HBM_embedding5_fstream.tellg();
    if (!HBM_embedding5_size) std::cout << "HBM_embedding5_size is 0!";
    HBM_embedding5_fstream.seekg(0, HBM_embedding5_fstream.beg);
    HBM_embedding6_fstream.seekg(0, HBM_embedding6_fstream.end);
    size_t HBM_embedding6_size =  HBM_embedding6_fstream.tellg();
    if (!HBM_embedding6_size) std::cout << "HBM_embedding6_size is 0!";
    HBM_embedding6_fstream.seekg(0, HBM_embedding6_fstream.beg);
    HBM_embedding7_fstream.seekg(0, HBM_embedding7_fstream.end);
    size_t HBM_embedding7_size =  HBM_embedding7_fstream.tellg();
    if (!HBM_embedding7_size) std::cout << "HBM_embedding7_size is 0!";
    HBM_embedding7_fstream.seekg(0, HBM_embedding7_fstream.beg);
    HBM_embedding8_fstream.seekg(0, HBM_embedding8_fstream.end);
    size_t HBM_embedding8_size =  HBM_embedding8_fstream.tellg();
    if (!HBM_embedding8_size) std::cout << "HBM_embedding8_size is 0!";
    HBM_embedding8_fstream.seekg(0, HBM_embedding8_fstream.beg);
    HBM_embedding9_fstream.seekg(0, HBM_embedding9_fstream.end);
    size_t HBM_embedding9_size =  HBM_embedding9_fstream.tellg();
    if (!HBM_embedding9_size) std::cout << "HBM_embedding9_size is 0!";
    HBM_embedding9_fstream.seekg(0, HBM_embedding9_fstream.beg);
    HBM_embedding10_fstream.seekg(0, HBM_embedding10_fstream.end);
    size_t HBM_embedding10_size =  HBM_embedding10_fstream.tellg();
    if (!HBM_embedding10_size) std::cout << "HBM_embedding10_size is 0!";
    HBM_embedding10_fstream.seekg(0, HBM_embedding10_fstream.beg);
    HBM_embedding11_fstream.seekg(0, HBM_embedding11_fstream.end);
    size_t HBM_embedding11_size =  HBM_embedding11_fstream.tellg();
    if (!HBM_embedding11_size) std::cout << "HBM_embedding11_size is 0!";
    HBM_embedding11_fstream.seekg(0, HBM_embedding11_fstream.beg);

    size_t HBM_embedding0_len = (int) (HBM_embedding0_size / sizeof(ap_uint512_t));
    size_t HBM_embedding1_len = (int) (HBM_embedding1_size / sizeof(ap_uint512_t));
    size_t HBM_embedding2_len = (int) (HBM_embedding2_size / sizeof(ap_uint512_t));
    size_t HBM_embedding3_len = (int) (HBM_embedding3_size / sizeof(ap_uint512_t));
    size_t HBM_embedding4_len = (int) (HBM_embedding4_size / sizeof(ap_uint512_t));
    size_t HBM_embedding5_len = (int) (HBM_embedding5_size / sizeof(ap_uint512_t));
    size_t HBM_embedding6_len = (int) (HBM_embedding6_size / sizeof(ap_uint512_t));
    size_t HBM_embedding7_len = (int) (HBM_embedding7_size / sizeof(ap_uint512_t));
    size_t HBM_embedding8_len = (int) (HBM_embedding8_size / sizeof(ap_uint512_t));
    size_t HBM_embedding9_len = (int) (HBM_embedding9_size / sizeof(ap_uint512_t));
    size_t HBM_embedding10_len = (int) (HBM_embedding10_size / sizeof(ap_uint512_t));
    size_t HBM_embedding11_len = (int) (HBM_embedding11_size / sizeof(ap_uint512_t));

    size_t HBM_centroid_vectors0_len = 2 * centroids_per_partition_even * D * sizeof(float) / sizeof(ap_uint512_t);
    size_t HBM_centroid_vectors1_len = 2 * centroids_per_partition_even * D * sizeof(float) / sizeof(ap_uint512_t);
    size_t HBM_centroid_vectors2_len = 2 * centroids_per_partition_even * D * sizeof(float) / sizeof(ap_uint512_t);
    size_t HBM_centroid_vectors3_len = 2 * centroids_per_partition_even * D * sizeof(float) / sizeof(ap_uint512_t);
    size_t HBM_centroid_vectors4_len = (centroids_per_partition_even + centroids_per_partition_last_PE) * D * sizeof(float) / sizeof(ap_uint512_t);


    int query_num = 10000;
    size_t HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid_len = nlist * 3;
    size_t HBM_query_vector_len = query_num * 128 < 10000 * 128? query_num * 128: 10000 * 128;
    size_t HBM_vector_quantizer_len = nlist * 128;
    size_t HBM_product_quantizer_len = 16 * 256 * (128 / 16);
    size_t HBM_OPQ_matrix_len = 128 * 128;
    size_t HBM_out_len = TOPK * query_num; 

    // the storage format of the meta info:
    //   (1) HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid: size = 3 * nlist
    //   (2) HBM_product_quantizer: size = K * D
    //   (3) (optional) s_OPQ_init: D * D, if OPQ_enable = False, send nothing
    //   (4) HBM_query_vectors: size = query_num * D (send last, because the accelerator needs to send queries continuously)
    size_t HBM_meta_info_len;
    if (OPQ_enable) {
        HBM_meta_info_len = HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid_len + 
            HBM_product_quantizer_len + HBM_OPQ_matrix_len + HBM_query_vector_len;
    } else {
        HBM_meta_info_len = HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid_len + 
            HBM_product_quantizer_len + HBM_query_vector_len;
    }

    // the raw ground truth size is the same for idx_1M.ivecs, idx_10M.ivecs, idx_100M.ivecs
    size_t raw_gt_vec_ID_len = 10000 * 1001; 
    // recall counts the very first nearest neighbor only
    size_t gt_vec_ID_len = 10000;

    size_t HBM_centroid_vectors0_size =  HBM_centroid_vectors0_len * sizeof(ap_uint512_t);
    size_t HBM_centroid_vectors1_size =  HBM_centroid_vectors1_len * sizeof(ap_uint512_t);
    size_t HBM_centroid_vectors2_size =  HBM_centroid_vectors2_len * sizeof(ap_uint512_t);
    size_t HBM_centroid_vectors3_size =  HBM_centroid_vectors3_len * sizeof(ap_uint512_t);
    size_t HBM_centroid_vectors4_size =  HBM_centroid_vectors4_len * sizeof(ap_uint512_t);


    size_t HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid_size = 
        HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid_len * sizeof(int);
    size_t HBM_query_vector_size = HBM_query_vector_len * sizeof(float);
    size_t HBM_vector_quantizer_size = HBM_vector_quantizer_len * sizeof(float);
    size_t HBM_product_quantizer_size = HBM_product_quantizer_len * sizeof(float);
    size_t HBM_OPQ_matrix_size = HBM_OPQ_matrix_len * sizeof(float);
    size_t HBM_out_size = HBM_out_len * sizeof(ap_uint64_t); 
    size_t HBM_meta_info_size = HBM_meta_info_len * sizeof(float);

    size_t raw_gt_vec_ID_size = raw_gt_vec_ID_len * sizeof(int);
    size_t gt_vec_ID_size = gt_vec_ID_len * sizeof(int);

//////////////////////////////   TEMPLATE END  //////////////////////////////

    cl_int err;
    unsigned fileBufSize;

    // allocate aligned 2D vectors
//////////////////////////////   TEMPLATE START  //////////////////////////////
    std::vector<ap_uint512_t, aligned_allocator<ap_uint512_t>> HBM_embedding0(HBM_embedding0_len, 0);
    std::vector<ap_uint512_t, aligned_allocator<ap_uint512_t>> HBM_embedding1(HBM_embedding1_len, 0);
    std::vector<ap_uint512_t, aligned_allocator<ap_uint512_t>> HBM_embedding2(HBM_embedding2_len, 0);
    std::vector<ap_uint512_t, aligned_allocator<ap_uint512_t>> HBM_embedding3(HBM_embedding3_len, 0);
    std::vector<ap_uint512_t, aligned_allocator<ap_uint512_t>> HBM_embedding4(HBM_embedding4_len, 0);
    std::vector<ap_uint512_t, aligned_allocator<ap_uint512_t>> HBM_embedding5(HBM_embedding5_len, 0);
    std::vector<ap_uint512_t, aligned_allocator<ap_uint512_t>> HBM_embedding6(HBM_embedding6_len, 0);
    std::vector<ap_uint512_t, aligned_allocator<ap_uint512_t>> HBM_embedding7(HBM_embedding7_len, 0);
    std::vector<ap_uint512_t, aligned_allocator<ap_uint512_t>> HBM_embedding8(HBM_embedding8_len, 0);
    std::vector<ap_uint512_t, aligned_allocator<ap_uint512_t>> HBM_embedding9(HBM_embedding9_len, 0);
    std::vector<ap_uint512_t, aligned_allocator<ap_uint512_t>> HBM_embedding10(HBM_embedding10_len, 0);
    std::vector<ap_uint512_t, aligned_allocator<ap_uint512_t>> HBM_embedding11(HBM_embedding11_len, 0);

    std::vector<ap_uint512_t, aligned_allocator<ap_uint512_t>> HBM_centroid_vectors0(HBM_centroid_vectors0_len, 0);
    std::vector<ap_uint512_t, aligned_allocator<ap_uint512_t>> HBM_centroid_vectors1(HBM_centroid_vectors1_len, 0);
    std::vector<ap_uint512_t, aligned_allocator<ap_uint512_t>> HBM_centroid_vectors2(HBM_centroid_vectors2_len, 0);
    std::vector<ap_uint512_t, aligned_allocator<ap_uint512_t>> HBM_centroid_vectors3(HBM_centroid_vectors3_len, 0);
    std::vector<ap_uint512_t, aligned_allocator<ap_uint512_t>> HBM_centroid_vectors4(HBM_centroid_vectors4_len, 0);


    std::vector<int, aligned_allocator<int>> HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid(
        HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid_len, 0);
    std::vector<float, aligned_allocator<float>> HBM_query_vectors(HBM_query_vector_len, 0);
    std::vector<float, aligned_allocator<float>> HBM_vector_quantizer(HBM_vector_quantizer_len, 0);
    std::vector<float, aligned_allocator<float>> HBM_product_quantizer(HBM_product_quantizer_len, 0);
    std::vector<float, aligned_allocator<float>> HBM_OPQ_matrix(HBM_OPQ_matrix_len, 0);
    std::vector<ap_uint64_t, aligned_allocator<ap_uint64_t>> HBM_out(HBM_out_len, 0);
    std::vector<float, aligned_allocator<float>> HBM_meta_info(HBM_meta_info_len, 0);
    
    std::vector<int, aligned_allocator<int>> raw_gt_vec_ID(raw_gt_vec_ID_len, 0);
    std::vector<int, aligned_allocator<int>> gt_vec_ID(gt_vec_ID_len, 0);

//////////////////////////////   TEMPLATE END  //////////////////////////////

    char* HBM_embedding0_char = (char*) malloc(HBM_embedding0_size);
    char* HBM_embedding1_char = (char*) malloc(HBM_embedding1_size);
    char* HBM_embedding2_char = (char*) malloc(HBM_embedding2_size);
    char* HBM_embedding3_char = (char*) malloc(HBM_embedding3_size);
    char* HBM_embedding4_char = (char*) malloc(HBM_embedding4_size);
    char* HBM_embedding5_char = (char*) malloc(HBM_embedding5_size);
    char* HBM_embedding6_char = (char*) malloc(HBM_embedding6_size);
    char* HBM_embedding7_char = (char*) malloc(HBM_embedding7_size);
    char* HBM_embedding8_char = (char*) malloc(HBM_embedding8_size);
    char* HBM_embedding9_char = (char*) malloc(HBM_embedding9_size);
    char* HBM_embedding10_char = (char*) malloc(HBM_embedding10_size);
    char* HBM_embedding11_char = (char*) malloc(HBM_embedding11_size);


    char* HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid_char = 
        (char*) malloc(HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid_size);
    char* HBM_query_vector_char = (char*) malloc(HBM_query_vector_size);
    char* HBM_vector_quantizer_char = (char*) malloc(HBM_vector_quantizer_size);
    char* HBM_product_quantizer_char = (char*) malloc(HBM_product_quantizer_size);
    char* HBM_OPQ_matrix_char = (char*) malloc(HBM_OPQ_matrix_size);

    char* raw_gt_vec_ID_char = (char*) malloc(raw_gt_vec_ID_size);

  
    std::string HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid_dir_suffix = 
        "HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid_3_by_" + std::to_string(nlist) + "_raw";
    std::string HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid_dir = 
        dir_concat(data_dir_prefix, HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid_dir_suffix);
    std::ifstream HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid_fstream(
        HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid_dir, 
        std::ios::in | std::ios::binary);


    std::string HBM_query_vector_dir_suffix = "query_vectors_float32_10000_128_raw";
    std::string HBM_query_vector_path = dir_concat(data_dir_prefix, HBM_query_vector_dir_suffix);
    std::ifstream HBM_query_vector_fstream(
        HBM_query_vector_path,
        std::ios::in | std::ios::binary);

    
    std::string HBM_vector_quantizer_dir_suffix = "vector_quantizer_float32_" + std::to_string(nlist) + "_128_raw";
    std::string HBM_vector_quantizer_dir = dir_concat(data_dir_prefix, HBM_vector_quantizer_dir_suffix);
    std::ifstream HBM_vector_quantizer_fstream(
        HBM_vector_quantizer_dir, 
        std::ios::in | std::ios::binary);

    
    std::string HBM_product_quantizer_suffix_dir = "product_quantizer_float32_16_256_8_raw";
    std::string HBM_product_quantizer_dir = dir_concat(data_dir_prefix, HBM_product_quantizer_suffix_dir);
    std::ifstream HBM_product_quantizer_fstream(
        HBM_product_quantizer_dir,
        std::ios::in | std::ios::binary);


    if (OPQ_enable) {
        std::string HBM_OPQ_matrix_suffix_dir = "OPQ_matrix_float32_128_128_raw";
        std::string HBM_OPQ_matrix_dir = dir_concat(data_dir_prefix, HBM_OPQ_matrix_suffix_dir);
        std::ifstream HBM_OPQ_matrix_fstream(
            HBM_OPQ_matrix_dir,
            std::ios::in | std::ios::binary);
        HBM_OPQ_matrix_fstream.read(HBM_OPQ_matrix_char, HBM_OPQ_matrix_size);
        if (!HBM_OPQ_matrix_fstream) {
            std::cout << "error: only " << HBM_OPQ_matrix_fstream.gcount() << " could be read";
            exit(1);
        }
        memcpy(&HBM_OPQ_matrix[0], HBM_OPQ_matrix_char, HBM_OPQ_matrix_size);
    }


    std::string raw_gt_vec_ID_suffix_dir = "idx_100M.ivecs";
    std::string raw_gt_vec_ID_dir = dir_concat(gnd_dir, raw_gt_vec_ID_suffix_dir);
    std::ifstream raw_gt_vec_ID_fstream(
        raw_gt_vec_ID_dir,
        std::ios::in | std::ios::binary);
    if (!raw_gt_vec_ID_fstream) {
        std::cout << "error: only " << raw_gt_vec_ID_fstream.gcount() << " could be read";
        exit(1);
}

        
    HBM_embedding0_fstream.read(HBM_embedding0_char, HBM_embedding0_size);
    if (!HBM_embedding0_fstream) {
            std::cout << "error: only " << HBM_embedding0_fstream.gcount() << " could be read";
        exit(1);
     }
    HBM_embedding1_fstream.read(HBM_embedding1_char, HBM_embedding1_size);
    if (!HBM_embedding1_fstream) {
            std::cout << "error: only " << HBM_embedding1_fstream.gcount() << " could be read";
        exit(1);
     }
    HBM_embedding2_fstream.read(HBM_embedding2_char, HBM_embedding2_size);
    if (!HBM_embedding2_fstream) {
            std::cout << "error: only " << HBM_embedding2_fstream.gcount() << " could be read";
        exit(1);
     }
    HBM_embedding3_fstream.read(HBM_embedding3_char, HBM_embedding3_size);
    if (!HBM_embedding3_fstream) {
            std::cout << "error: only " << HBM_embedding3_fstream.gcount() << " could be read";
        exit(1);
     }
    HBM_embedding4_fstream.read(HBM_embedding4_char, HBM_embedding4_size);
    if (!HBM_embedding4_fstream) {
            std::cout << "error: only " << HBM_embedding4_fstream.gcount() << " could be read";
        exit(1);
     }
    HBM_embedding5_fstream.read(HBM_embedding5_char, HBM_embedding5_size);
    if (!HBM_embedding5_fstream) {
            std::cout << "error: only " << HBM_embedding5_fstream.gcount() << " could be read";
        exit(1);
     }
    HBM_embedding6_fstream.read(HBM_embedding6_char, HBM_embedding6_size);
    if (!HBM_embedding6_fstream) {
            std::cout << "error: only " << HBM_embedding6_fstream.gcount() << " could be read";
        exit(1);
     }
    HBM_embedding7_fstream.read(HBM_embedding7_char, HBM_embedding7_size);
    if (!HBM_embedding7_fstream) {
            std::cout << "error: only " << HBM_embedding7_fstream.gcount() << " could be read";
        exit(1);
     }
    HBM_embedding8_fstream.read(HBM_embedding8_char, HBM_embedding8_size);
    if (!HBM_embedding8_fstream) {
            std::cout << "error: only " << HBM_embedding8_fstream.gcount() << " could be read";
        exit(1);
     }
    HBM_embedding9_fstream.read(HBM_embedding9_char, HBM_embedding9_size);
    if (!HBM_embedding9_fstream) {
            std::cout << "error: only " << HBM_embedding9_fstream.gcount() << " could be read";
        exit(1);
     }
    HBM_embedding10_fstream.read(HBM_embedding10_char, HBM_embedding10_size);
    if (!HBM_embedding10_fstream) {
            std::cout << "error: only " << HBM_embedding10_fstream.gcount() << " could be read";
        exit(1);
     }
    HBM_embedding11_fstream.read(HBM_embedding11_char, HBM_embedding11_size);
    if (!HBM_embedding11_fstream) {
            std::cout << "error: only " << HBM_embedding11_fstream.gcount() << " could be read";
        exit(1);
     }


    HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid_fstream.read(
        HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid_char,
        HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid_size);
    if (!HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid_fstream) {
        std::cout << "error: only " << HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid_fstream.gcount() << " could be read";
        exit(1);
    }
    HBM_query_vector_fstream.read(HBM_query_vector_char, HBM_query_vector_size);
    if (!HBM_query_vector_fstream) {
        std::cout << "error: only " << HBM_query_vector_fstream.gcount() << " could be read";
        exit(1);
    }
    HBM_vector_quantizer_fstream.read(HBM_vector_quantizer_char, HBM_vector_quantizer_size);
    if (!HBM_vector_quantizer_fstream) {
        std::cout << "error: only " << HBM_vector_quantizer_fstream.gcount() << " could be read";
        exit(1);
    }
    HBM_product_quantizer_fstream.read(HBM_product_quantizer_char, HBM_product_quantizer_size);
    if (!HBM_product_quantizer_fstream) {
        std::cout << "error: only " << HBM_product_quantizer_fstream.gcount() << " could be read";
        exit(1);
    }

    raw_gt_vec_ID_fstream.read(raw_gt_vec_ID_char, raw_gt_vec_ID_size);
    if (!raw_gt_vec_ID_fstream) {
        std::cout << "error: only " << raw_gt_vec_ID_fstream.gcount() << " could be read";
        exit(1);
    }

    // std::cout << "HBM_query_vector_fstream read bytes: " << HBM_query_vector_fstream.gcount() << std::endl;
    // std::cout << "HBM_vector_quantizer_fstream read bytes: " << HBM_vector_quantizer_fstream.gcount() << std::endl;
    // std::cout << "HBM_product_quantizer_fstream read bytes: " << HBM_product_quantizer_fstream.gcount() << std::endl;
 
    memcpy(&HBM_embedding0[0], HBM_embedding0_char, HBM_embedding0_size);
    memcpy(&HBM_embedding1[0], HBM_embedding1_char, HBM_embedding1_size);
    memcpy(&HBM_embedding2[0], HBM_embedding2_char, HBM_embedding2_size);
    memcpy(&HBM_embedding3[0], HBM_embedding3_char, HBM_embedding3_size);
    memcpy(&HBM_embedding4[0], HBM_embedding4_char, HBM_embedding4_size);
    memcpy(&HBM_embedding5[0], HBM_embedding5_char, HBM_embedding5_size);
    memcpy(&HBM_embedding6[0], HBM_embedding6_char, HBM_embedding6_size);
    memcpy(&HBM_embedding7[0], HBM_embedding7_char, HBM_embedding7_size);
    memcpy(&HBM_embedding8[0], HBM_embedding8_char, HBM_embedding8_size);
    memcpy(&HBM_embedding9[0], HBM_embedding9_char, HBM_embedding9_size);
    memcpy(&HBM_embedding10[0], HBM_embedding10_char, HBM_embedding10_size);
    memcpy(&HBM_embedding11[0], HBM_embedding11_char, HBM_embedding11_size);


    memcpy(&HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid[0], 
        HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid_char, 
        HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid_size);
    memcpy(&HBM_query_vectors[0], HBM_query_vector_char, HBM_query_vector_size);
    memcpy(&HBM_vector_quantizer[0], HBM_vector_quantizer_char, HBM_vector_quantizer_size);
    memcpy(&HBM_product_quantizer[0], HBM_product_quantizer_char, HBM_product_quantizer_size);

    int start_addr_HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid = 0;
    int size_HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid = 3 * nlist;
    memcpy(&HBM_meta_info[start_addr_HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid], 
        &HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid[0],
        size_HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid * sizeof(float));

    int start_addr_HBM_product_quantizer = 
        start_addr_HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid + 
        size_HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid;
    int size_HBM_product_quantizer = K * D;
    memcpy(&HBM_meta_info[start_addr_HBM_product_quantizer], 
        &HBM_product_quantizer[0],
        size_HBM_product_quantizer * sizeof(float));

    int start_addr_OPQ_init;
    int size_OPQ_init;
    if (OPQ_enable) {
        start_addr_OPQ_init = start_addr_HBM_product_quantizer + size_HBM_product_quantizer;
        size_OPQ_init = D * D;
        memcpy(&HBM_meta_info[start_addr_OPQ_init], 
            &HBM_OPQ_matrix[0],
            size_OPQ_init * sizeof(float));
    }

    int start_addr_HBM_query_vectors;
    if (OPQ_enable) {
        start_addr_HBM_query_vectors = start_addr_OPQ_init + size_OPQ_init;
    }
    else { 
        start_addr_HBM_query_vectors = start_addr_HBM_product_quantizer + size_HBM_product_quantizer;
    }
    int size_HBM_query_vectors = query_num * D;
    memcpy(&HBM_meta_info[start_addr_HBM_query_vectors], 
        &HBM_query_vectors[0],
        size_HBM_query_vectors * sizeof(float));
    


    int HBM_centroid_vectors_stage2_start_addr_0 = 2 * 0 * centroids_per_partition_even * D * sizeof(float);
    memcpy(&HBM_centroid_vectors0[0], HBM_vector_quantizer_char + HBM_centroid_vectors_stage2_start_addr_0, HBM_centroid_vectors0_size);

    int HBM_centroid_vectors_stage2_start_addr_1 = 2 * 1 * centroids_per_partition_even * D * sizeof(float);
    memcpy(&HBM_centroid_vectors1[0], HBM_vector_quantizer_char + HBM_centroid_vectors_stage2_start_addr_1, HBM_centroid_vectors1_size);

    int HBM_centroid_vectors_stage2_start_addr_2 = 2 * 2 * centroids_per_partition_even * D * sizeof(float);
    memcpy(&HBM_centroid_vectors2[0], HBM_vector_quantizer_char + HBM_centroid_vectors_stage2_start_addr_2, HBM_centroid_vectors2_size);

    int HBM_centroid_vectors_stage2_start_addr_3 = 2 * 3 * centroids_per_partition_even * D * sizeof(float);
    memcpy(&HBM_centroid_vectors3[0], HBM_vector_quantizer_char + HBM_centroid_vectors_stage2_start_addr_3, HBM_centroid_vectors3_size);

    int HBM_centroid_vectors_stage2_start_addr_4 = 2 * 4 * centroids_per_partition_even * D * sizeof(float);
    memcpy(&HBM_centroid_vectors4[0], HBM_vector_quantizer_char + HBM_centroid_vectors_stage2_start_addr_4, HBM_centroid_vectors4_size);


    memcpy(&raw_gt_vec_ID[0], raw_gt_vec_ID_char, raw_gt_vec_ID_size);

    free(HBM_embedding0_char);
    free(HBM_embedding1_char);
    free(HBM_embedding2_char);
    free(HBM_embedding3_char);
    free(HBM_embedding4_char);
    free(HBM_embedding5_char);
    free(HBM_embedding6_char);
    free(HBM_embedding7_char);
    free(HBM_embedding8_char);
    free(HBM_embedding9_char);
    free(HBM_embedding10_char);
    free(HBM_embedding11_char);


    free(HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid_char);
    free(HBM_query_vector_char);
    free(HBM_vector_quantizer_char);
    free(HBM_product_quantizer_char);
    free(HBM_OPQ_matrix_char);

    free(raw_gt_vec_ID_char);
    // free(sw_result_vec_ID_char);
    // free(sw_result_dist_char);

    // copy contents from raw ground truth to needed ones
    // Format of ground truth (for 10000 query vectors):
    //   1000(topK), [1000 ids]
    //   1000(topK), [1000 ids]
    //        ...     ...
    //   1000(topK), [1000 ids]
    // 10000 rows in total, 10000 * 1001 elements, 10000 * 1001 * 4 bytes
    for (int i = 0; i < 10000; i++) {
        gt_vec_ID[i] = raw_gt_vec_ID[i * 1001 + 1];
    }

// OPENCL HOST CODE AREA START
	
// ------------------------------------------------------------------------------------
// Step 1: Get All PLATFORMS, then search for Target_Platform_Vendor (CL_PLATFORM_VENDOR)
//	   Search for Platform: Xilinx 
// Check if the current platform matches Target_Platform_Vendor
// ------------------------------------------------------------------------------------	
    std::vector<cl::Device> devices = get_devices("Xilinx");
    devices.resize(1);
    cl::Device device = devices[0];
    std::cout << "Finished getting device..." << std::endl;
// ------------------------------------------------------------------------------------
// Step 1: Create Context
// ------------------------------------------------------------------------------------
    OCL_CHECK(err, cl::Context context(device, NULL, NULL, NULL, &err));
	std::cout << "Finished creating context..." << std::endl;
// ------------------------------------------------------------------------------------
// Step 1: Create Command Queue
// ------------------------------------------------------------------------------------
    OCL_CHECK(err, cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE, &err));
	std::cout << "Finished creating command queue..." << std::endl;
// ------------------------------------------------------------------
// Step 1: Load Binary File from disk
// ------------------------------------------------------------------		
    char* fileBuf = read_binary_file(binaryFile, fileBufSize);
    cl::Program::Binaries bins{{fileBuf, fileBufSize}};
    	std::cout << "Finished loading binary..." << std::endl;
	
// -------------------------------------------------------------
// Step 1: Create the program object from the binary and program the FPGA device with it
// -------------------------------------------------------------	
    OCL_CHECK(err, cl::Program program(context, devices, bins, NULL, &err));
	std::cout << "Finished creating program..." << std::endl;
// -------------------------------------------------------------
// Step 1: Create Kernels
// -------------------------------------------------------------
    OCL_CHECK(err, cl::Kernel krnl_vector_add(program,"vadd", &err));
    std::cout << "Finished creating kernel..." << std::endl;

// ================================================================
// Step 2: Setup Buffers and run Kernels
// ================================================================
//   o) Allocate Memory to store the results 
//   o) Create Buffers in Global Memory to store data
// ================================================================

// ------------------------------------------------------------------
// Step 2: Create Buffers in Global Memory to store data
//             o) buffer_in1 - stores source_in1
//             o) buffer_in2 - stores source_in2
//             o) buffer_ouput - stores Results
// ------------------------------------------------------------------	

// .......................................................
// Allocate Global Memory for source_in1
// .......................................................	
//////////////////////////////   TEMPLATE START  //////////////////////////////
    std::cout << "Start to allocate device memory..." << std::endl;
    cl_mem_ext_ptr_t 
        HBM_embedding0Ext,
        HBM_embedding1Ext,
        HBM_embedding2Ext,
        HBM_embedding3Ext,
        HBM_embedding4Ext,
        HBM_embedding5Ext,
        HBM_embedding6Ext,
        HBM_embedding7Ext,
        HBM_embedding8Ext,
        HBM_embedding9Ext,
        HBM_embedding10Ext,
        HBM_embedding11Ext,

        HBM_centroid_vectors0Ext,
        HBM_centroid_vectors1Ext,
        HBM_centroid_vectors2Ext,
        HBM_centroid_vectors3Ext,
        HBM_centroid_vectors4Ext,

        // HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_validExt, // HBM 21
        // HBM_query_vectorExt, 
        HBM_meta_infoExt,
        HBM_vector_quantizerExt, 
        // HBM_product_quantizerExt, 
// #ifdef OPQ_ENABLE
//         HBM_OPQ_matrixExt, 
// #endif
        HBM_outExt;
//////////////////////////////   TEMPLATE END  //////////////////////////////

//////////////////////////////   TEMPLATE START  //////////////////////////////
    HBM_embedding0Ext.obj = HBM_embedding0.data();
    HBM_embedding0Ext.param = 0;
    HBM_embedding0Ext.flags = bank[0];
    HBM_embedding1Ext.obj = HBM_embedding1.data();
    HBM_embedding1Ext.param = 0;
    HBM_embedding1Ext.flags = bank[1];
    HBM_embedding2Ext.obj = HBM_embedding2.data();
    HBM_embedding2Ext.param = 0;
    HBM_embedding2Ext.flags = bank[2];
    HBM_embedding3Ext.obj = HBM_embedding3.data();
    HBM_embedding3Ext.param = 0;
    HBM_embedding3Ext.flags = bank[3];
    HBM_embedding4Ext.obj = HBM_embedding4.data();
    HBM_embedding4Ext.param = 0;
    HBM_embedding4Ext.flags = bank[4];
    HBM_embedding5Ext.obj = HBM_embedding5.data();
    HBM_embedding5Ext.param = 0;
    HBM_embedding5Ext.flags = bank[5];
    HBM_embedding6Ext.obj = HBM_embedding6.data();
    HBM_embedding6Ext.param = 0;
    HBM_embedding6Ext.flags = bank[6];
    HBM_embedding7Ext.obj = HBM_embedding7.data();
    HBM_embedding7Ext.param = 0;
    HBM_embedding7Ext.flags = bank[7];
    HBM_embedding8Ext.obj = HBM_embedding8.data();
    HBM_embedding8Ext.param = 0;
    HBM_embedding8Ext.flags = bank[8];
    HBM_embedding9Ext.obj = HBM_embedding9.data();
    HBM_embedding9Ext.param = 0;
    HBM_embedding9Ext.flags = bank[9];
    HBM_embedding10Ext.obj = HBM_embedding10.data();
    HBM_embedding10Ext.param = 0;
    HBM_embedding10Ext.flags = bank[10];
    HBM_embedding11Ext.obj = HBM_embedding11.data();
    HBM_embedding11Ext.param = 0;
    HBM_embedding11Ext.flags = bank[11];

    HBM_centroid_vectors0Ext.obj = HBM_centroid_vectors0.data();
    HBM_centroid_vectors0Ext.param = 0;
    HBM_centroid_vectors0Ext.flags = bank[19];
    HBM_centroid_vectors1Ext.obj = HBM_centroid_vectors1.data();
    HBM_centroid_vectors1Ext.param = 0;
    HBM_centroid_vectors1Ext.flags = bank[20];
    HBM_centroid_vectors2Ext.obj = HBM_centroid_vectors2.data();
    HBM_centroid_vectors2Ext.param = 0;
    HBM_centroid_vectors2Ext.flags = bank[21];
    HBM_centroid_vectors3Ext.obj = HBM_centroid_vectors3.data();
    HBM_centroid_vectors3Ext.param = 0;
    HBM_centroid_vectors3Ext.flags = bank[22];
    HBM_centroid_vectors4Ext.obj = HBM_centroid_vectors4.data();
    HBM_centroid_vectors4Ext.param = 0;
    HBM_centroid_vectors4Ext.flags = bank[23];


    // HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_validExt.obj = 
    //     HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid.data();
    // HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_validExt.param = 0;
    // HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_validExt.flags = bank[21];

    // HBM_query_vectorExt.obj = HBM_query_vectors.data();
    // HBM_query_vectorExt.param = 0;
    // HBM_query_vectorExt.flags = bank[22];

    HBM_meta_infoExt.obj = HBM_meta_info.data();
    HBM_vector_quantizerExt.param = 0;
    HBM_vector_quantizerExt.flags = bank[25];

    HBM_vector_quantizerExt.obj = HBM_vector_quantizer.data();
    HBM_vector_quantizerExt.param = 0;
    HBM_vector_quantizerExt.flags = bank[26];

    // HBM_product_quantizerExt.obj = HBM_product_quantizer.data();
    // HBM_product_quantizerExt.param = 0;
    // HBM_product_quantizerExt.flags = bank[24];

// #ifdef OPQ_ENABLE
    // HBM_OPQ_matrixExt.obj = HBM_OPQ_matrix.data();
    // HBM_OPQ_matrixExt.param = 0;
    // HBM_OPQ_matrixExt.flags = bank[25];
// #endif

    HBM_outExt.obj = HBM_out.data();
    HBM_outExt.param = 0;
    HBM_outExt.flags = bank[27];
    

//////////////////////////////   TEMPLATE START  //////////////////////////////
    OCL_CHECK(err, cl::Buffer buffer_HBM_embedding0(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX, 
            HBM_embedding0_size, &HBM_embedding0Ext, &err));
    OCL_CHECK(err, cl::Buffer buffer_HBM_embedding1(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX, 
            HBM_embedding1_size, &HBM_embedding1Ext, &err));
    OCL_CHECK(err, cl::Buffer buffer_HBM_embedding2(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX, 
            HBM_embedding2_size, &HBM_embedding2Ext, &err));
    OCL_CHECK(err, cl::Buffer buffer_HBM_embedding3(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX, 
            HBM_embedding3_size, &HBM_embedding3Ext, &err));
    OCL_CHECK(err, cl::Buffer buffer_HBM_embedding4(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX, 
            HBM_embedding4_size, &HBM_embedding4Ext, &err));
    OCL_CHECK(err, cl::Buffer buffer_HBM_embedding5(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX, 
            HBM_embedding5_size, &HBM_embedding5Ext, &err));
    OCL_CHECK(err, cl::Buffer buffer_HBM_embedding6(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX, 
            HBM_embedding6_size, &HBM_embedding6Ext, &err));
    OCL_CHECK(err, cl::Buffer buffer_HBM_embedding7(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX, 
            HBM_embedding7_size, &HBM_embedding7Ext, &err));
    OCL_CHECK(err, cl::Buffer buffer_HBM_embedding8(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX, 
            HBM_embedding8_size, &HBM_embedding8Ext, &err));
    OCL_CHECK(err, cl::Buffer buffer_HBM_embedding9(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX, 
            HBM_embedding9_size, &HBM_embedding9Ext, &err));
    OCL_CHECK(err, cl::Buffer buffer_HBM_embedding10(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX, 
            HBM_embedding10_size, &HBM_embedding10Ext, &err));
    OCL_CHECK(err, cl::Buffer buffer_HBM_embedding11(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX, 
            HBM_embedding11_size, &HBM_embedding11Ext, &err));

    OCL_CHECK(err, cl::Buffer buffer_HBM_centroid_vectors0(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX, 
            HBM_centroid_vectors0_size, &HBM_centroid_vectors0Ext, &err));
    OCL_CHECK(err, cl::Buffer buffer_HBM_centroid_vectors1(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX, 
            HBM_centroid_vectors1_size, &HBM_centroid_vectors1Ext, &err));
    OCL_CHECK(err, cl::Buffer buffer_HBM_centroid_vectors2(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX, 
            HBM_centroid_vectors2_size, &HBM_centroid_vectors2Ext, &err));
    OCL_CHECK(err, cl::Buffer buffer_HBM_centroid_vectors3(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX, 
            HBM_centroid_vectors3_size, &HBM_centroid_vectors3Ext, &err));
    OCL_CHECK(err, cl::Buffer buffer_HBM_centroid_vectors4(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX, 
            HBM_centroid_vectors4_size, &HBM_centroid_vectors4Ext, &err));


//     OCL_CHECK(err, cl::Buffer buffer_HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid(
//         context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX, 
//         HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid_size, 
//         &HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_validExt, &err));

//     OCL_CHECK(err, cl::Buffer buffer_HBM_query_vectors(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX, 
//             HBM_query_vector_size, &HBM_query_vectorExt, &err));
//     OCL_CHECK(err, cl::Buffer buffer_HBM_product_quantizer(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX, 
//             HBM_product_quantizer_size, &HBM_product_quantizerExt, &err));
// #ifdef OPQ_ENABLE
//     OCL_CHECK(err, cl::Buffer buffer_HBM_OPQ_matrix(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX, 
//             HBM_OPQ_matrix_size, &HBM_OPQ_matrixExt, &err));
// #endif

    OCL_CHECK(err, cl::Buffer buffer_HBM_vector_quantizer(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX, 
            HBM_vector_quantizer_size, &HBM_vector_quantizerExt, &err));
    OCL_CHECK(err, cl::Buffer buffer_HBM_meta_info(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX, 
            HBM_meta_info_size, &HBM_meta_infoExt, &err));
    
// .......................................................
// Allocate Global Memory for sourcce_hw_results
// .......................................................
    OCL_CHECK(err, cl::Buffer buffer_output(
        context, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY | CL_MEM_EXT_PTR_XILINX, 
        HBM_out_size, &HBM_outExt, &err));

// ============================================================================
// Step 2: Set Kernel Arguments and Run the Application
//         o) Set Kernel Arguments
//         o) Copy Input Data from Host to Global Memory on the device
//         o) Submit Kernels for Execution
//         o) Copy Results from Global Memory, device to Host
// ============================================================================	
    
//////////////////////////////   TEMPLATE START  //////////////////////////////
    int arg_counter = 0;
    OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, buffer_HBM_embedding0));
    OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, buffer_HBM_embedding1));
    OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, buffer_HBM_embedding2));
    OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, buffer_HBM_embedding3));
    OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, buffer_HBM_embedding4));
    OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, buffer_HBM_embedding5));
    OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, buffer_HBM_embedding6));
    OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, buffer_HBM_embedding7));
    OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, buffer_HBM_embedding8));
    OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, buffer_HBM_embedding9));
    OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, buffer_HBM_embedding10));
    OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, buffer_HBM_embedding11));

    OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, buffer_HBM_centroid_vectors0));
    OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, buffer_HBM_centroid_vectors1));
    OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, buffer_HBM_centroid_vectors2));
    OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, buffer_HBM_centroid_vectors3));
    OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, buffer_HBM_centroid_vectors4));

    
    // OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, buffer_HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid));
    // OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, buffer_HBM_query_vectors));
    OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, buffer_HBM_meta_info));
    OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, buffer_HBM_vector_quantizer));
    // OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, buffer_HBM_product_quantizer));
// #ifdef OPQ_ENABLE
//     OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, buffer_HBM_OPQ_matrix));
// #endif
    OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, nlist));
    OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, nprobe));
    OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, OPQ_enable));
    OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, centroids_per_partition_even));
    OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, centroids_per_partition_last_PE));
    OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, nprobe_per_table_construction_pe_larger));
    OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, nprobe_per_table_construction_pe_smaller));

    OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, buffer_output));
    
//////////////////////////////   TEMPLATE END  //////////////////////////////
// ------------------------------------------------------
// Step 2: Copy Input data from Host to Global Memory on the device
// ------------------------------------------------------
//////////////////////////////   TEMPLATE START  //////////////////////////////
    std::cout << "Starting copy from Host to device..." << std::endl;
    OCL_CHECK(
        err, err = q.enqueueMigrateMemObjects({
        buffer_HBM_embedding0,
        buffer_HBM_embedding1,
        buffer_HBM_embedding2,
        buffer_HBM_embedding3,
        buffer_HBM_embedding4,
        buffer_HBM_embedding5,
        buffer_HBM_embedding6,
        buffer_HBM_embedding7,
        buffer_HBM_embedding8,
        buffer_HBM_embedding9,
        buffer_HBM_embedding10,
        buffer_HBM_embedding11,

        buffer_HBM_centroid_vectors0,
        buffer_HBM_centroid_vectors1,
        buffer_HBM_centroid_vectors2,
        buffer_HBM_centroid_vectors3,
        buffer_HBM_centroid_vectors4,

        // buffer_HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid,
        // buffer_HBM_query_vectors,
        buffer_HBM_meta_info,
        buffer_HBM_vector_quantizer
//         buffer_HBM_product_quantizer,
// #ifdef OPQ_ENABLE
//         buffer_HBM_OPQ_matrix
// #endif
        }, 0/* 0 means from host*/));	
    std::cout << "Host to device finished..." << std::endl;
//////////////////////////////   TEMPLATE END  //////////////////////////////
// ----------------------------------------
// Step 2: Submit Kernels for Execution
// ----------------------------------------
    OCL_CHECK(err, err = q.enqueueTask(krnl_vector_add));
// --------------------------------------------------
// Step 2: Copy Results from Device Global Memory to Host
// --------------------------------------------------
    OCL_CHECK(err, err = q.enqueueMigrateMemObjects({buffer_output},CL_MIGRATE_MEM_OBJECT_HOST));

    q.finish();
// OPENCL HOST CODE AREA END

    // Compare the results of the Device to the simulation
    // only check the last batch (since other are not transfered back)
    std::cout << "Comparing Results..." << std::endl;
    bool match = true;
    int count = 0;
    int match_count = 0;


    for (int query_id = 0; query_id < query_num; query_id++) {

        std::vector<int> hw_result_vec_ID_partial(TOPK, 0);
        std::vector<float> hw_result_dist_partial(TOPK, 0);

        // Load data
        for (int k = 0; k < TOPK; k++) {

            ap_uint<64> reg = HBM_out[query_id * TOPK + k];
            ap_uint<32> raw_vec_ID = reg.range(31, 0); 
            ap_uint<32>  raw_dist = reg.range(63, 32);
            int vec_ID = *((int*) (&raw_vec_ID));
            float dist = *((float*) (&raw_dist));
            
            hw_result_vec_ID_partial[k] = vec_ID;
            hw_result_dist_partial[k] = dist;
        }
        
        // Check correctness
        count++;
        // std::cout << "query id" << query_id << std::endl;
        for (int k = 0; k < TOPK; k++) {
            // std::cout << "hw: " << hw_result_vec_ID_partial[k] << "gt: " << gt_vec_ID[query_id] << std::endl;
            if (hw_result_vec_ID_partial[k] == gt_vec_ID[query_id]) {
                match_count++;
                break;
            }
        } 
    }

    float recall = ((float) match_count / (float) count);
    printf("\n=====  Recall: %.8f  =====\n", recall);
// ============================================================================
// Step 3: Release Allocated Resources
// ============================================================================
    delete[] fileBuf;

    // std::cout << "TEST " << (match ? "PASSED" : "FAILED") << std::endl; 
    return (match ? EXIT_SUCCESS : EXIT_FAILURE);
}
