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
        HBM_centroid_vectors_len
        HBM_centroid_vectors_size
        HBM_centroid_vectors_allocate
        HBM_centroid_vectors_memcpy
        HBM_centroid_vectorsExt
        HBM_centroid_vectorsExt_set
        HBM_metainfoExt_set
        buffer_HBM_centroid_vectors
        buffer_HBM_centroid_vectors_set_krnl_arg
        buffer_HBM_centroid_vectors_enqueueMigrateMemObjects

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

/* for U250 specifically */
const int bank[8] = {
    /* 0 ~ 3 DDR (16GB per channel) */
    BANK_NAME(0),  BANK_NAME(1),  BANK_NAME(2),  BANK_NAME(3), 
    /* 4 ~ 7 PLRAM */ 
    BANK_NAME(4), BANK_NAME(5), BANK_NAME(6), BANK_NAME(7)};
    


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

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <XCLBIN File>" << std::endl;
		return EXIT_FAILURE;
	}

    std::string binaryFile = argv[1];
//////////////////////////////   TEMPLATE START  //////////////////////////////
    
    std::ifstream HBM_embedding0_fstream(
        "/home/wejiang/saved_npy_data/FPGA_data_SIFT100M_OPQ16,IVF8192,PQ16_HBM_4_banks/HBM_bank_0_raw", 
        std::ios::in | std::ios::binary);
    std::ifstream HBM_embedding1_fstream(
        "/home/wejiang/saved_npy_data/FPGA_data_SIFT100M_OPQ16,IVF8192,PQ16_HBM_4_banks/HBM_bank_1_raw", 
        std::ios::in | std::ios::binary);
    std::ifstream HBM_embedding2_fstream(
        "/home/wejiang/saved_npy_data/FPGA_data_SIFT100M_OPQ16,IVF8192,PQ16_HBM_4_banks/HBM_bank_2_raw", 
        std::ios::in | std::ios::binary);
    std::ifstream HBM_embedding3_fstream(
        "/home/wejiang/saved_npy_data/FPGA_data_SIFT100M_OPQ16,IVF8192,PQ16_HBM_4_banks/HBM_bank_3_raw", 
        std::ios::in | std::ios::binary);


    HBM_embedding0_fstream.seekg(0, HBM_embedding0_fstream.end);
    size_t HBM_embedding0_size =  HBM_embedding0_fstream.tellg();
    HBM_embedding1_fstream.seekg(0, HBM_embedding1_fstream.end);
    size_t HBM_embedding1_size =  HBM_embedding1_fstream.tellg();
    HBM_embedding2_fstream.seekg(0, HBM_embedding2_fstream.end);
    size_t HBM_embedding2_size =  HBM_embedding2_fstream.tellg();
    HBM_embedding3_fstream.seekg(0, HBM_embedding3_fstream.end);
    size_t HBM_embedding3_size =  HBM_embedding3_fstream.tellg();

    size_t HBM_embedding0_len = (int) (HBM_embedding0_size / sizeof(ap_uint512_t));
    size_t HBM_embedding1_len = (int) (HBM_embedding1_size / sizeof(ap_uint512_t));
    size_t HBM_embedding2_len = (int) (HBM_embedding2_size / sizeof(ap_uint512_t));
    size_t HBM_embedding3_len = (int) (HBM_embedding3_size / sizeof(ap_uint512_t));



    int query_num = 10000;
    size_t HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid_len = NLIST * 3;
    size_t HBM_query_vector_len = query_num * 128 < 10000 * 128? query_num * 128: 10000 * 128;
    size_t HBM_vector_quantizer_len = 8192 * 128;
    size_t HBM_product_quantizer_len = 16 * 256 * (128 / 16);
#ifdef OPQ_ENABLE
    size_t HBM_OPQ_matrix_len = 128 * 128;
#endif
    size_t HBM_out_len = TOPK * query_num; 

    // the raw ground truth size is the same for idx_1M.ivecs, idx_10M.ivecs, idx_100M.ivecs
    size_t raw_gt_vec_ID_len = 10000 * 1001; 
    // recall counts the very first nearest neighbor only
    size_t gt_vec_ID_len = 10000;



    size_t HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid_size = 
        HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid_len * sizeof(int);
    size_t HBM_query_vector_size = HBM_query_vector_len * sizeof(float);
    size_t HBM_vector_quantizer_size = HBM_vector_quantizer_len * sizeof(float);
    size_t HBM_product_quantizer_size = HBM_product_quantizer_len * sizeof(float);
    size_t HBM_OPQ_matrix_size = HBM_OPQ_matrix_len * sizeof(float);
    size_t HBM_out_size = HBM_out_len * sizeof(ap_uint64_t); 

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



    std::vector<int, aligned_allocator<int>> HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid(
        HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid_len, 0);
    std::vector<float, aligned_allocator<float>> HBM_query_vectors(HBM_query_vector_len, 0);
    std::vector<float, aligned_allocator<float>> HBM_vector_quantizer(HBM_vector_quantizer_len, 0);
    std::vector<float, aligned_allocator<float>> HBM_product_quantizer(HBM_product_quantizer_len, 0);
#ifdef OPQ_ENABLE
    std::vector<float, aligned_allocator<float>> HBM_OPQ_matrix(HBM_OPQ_matrix_len, 0);
#endif
    std::vector<ap_uint64_t, aligned_allocator<ap_uint64_t>> HBM_out(HBM_out_len, 0);
    
    std::vector<int, aligned_allocator<int>> raw_gt_vec_ID(raw_gt_vec_ID_len, 0);
    std::vector<int, aligned_allocator<int>> gt_vec_ID(gt_vec_ID_len, 0);

//////////////////////////////   TEMPLATE END  //////////////////////////////

    char* HBM_embedding0_char = (char*) malloc(HBM_embedding0_size);
    char* HBM_embedding1_char = (char*) malloc(HBM_embedding1_size);
    char* HBM_embedding2_char = (char*) malloc(HBM_embedding2_size);
    char* HBM_embedding3_char = (char*) malloc(HBM_embedding3_size);


    char* HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid_char = 
        (char*) malloc(HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid_size);
    char* HBM_query_vector_char = (char*) malloc(HBM_query_vector_size);
    char* HBM_vector_quantizer_char = (char*) malloc(HBM_vector_quantizer_size);
    char* HBM_product_quantizer_char = (char*) malloc(HBM_product_quantizer_size);
#ifdef OPQ_ENABLE
    char* HBM_OPQ_matrix_char = (char*) malloc(HBM_OPQ_matrix_size);
#endif

    char* raw_gt_vec_ID_char = (char*) malloc(raw_gt_vec_ID_size);

    std::ifstream HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid_fstream(
        "/home/wejiang/saved_npy_data/FPGA_data_SIFT100M_OPQ16,IVF8192,PQ16_HBM_4_banks/HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid_3_by_8192_raw", 
        std::ios::in | std::ios::binary);

    std::ifstream HBM_query_vector_fstream(
        "/home/wejiang/saved_npy_data/FPGA_data_SIFT100M_OPQ16,IVF8192,PQ16_HBM_4_banks/query_vectors_float32_10000_128_raw", 
        std::ios::in | std::ios::binary);

    std::ifstream HBM_vector_quantizer_fstream(
        "/home/wejiang/saved_npy_data/FPGA_data_SIFT100M_OPQ16,IVF8192,PQ16_HBM_4_banks/vector_quantizer_float32_8192_128_raw", 
        std::ios::in | std::ios::binary);

    std::ifstream HBM_product_quantizer_fstream(
        "/home/wejiang/saved_npy_data/FPGA_data_SIFT100M_OPQ16,IVF8192,PQ16_HBM_4_banks/product_quantizer_float32_16_256_8_raw", 
        std::ios::in | std::ios::binary);

#ifdef OPQ_ENABLE
    std::ifstream HBM_OPQ_matrix_fstream(
        "/home/wejiang/saved_npy_data/FPGA_data_SIFT100M_OPQ16,IVF8192,PQ16_HBM_4_banks/OPQ_matrix_float32_128_128_raw", 
        std::ios::in | std::ios::binary);

#endif


    std::ifstream raw_gt_vec_ID_fstream(
        "/home/wejiang/saved_npy_data/gnd/idx_100M.ivecs", 
        std::ios::in | std::ios::binary);

        
    HBM_embedding0_fstream.read(HBM_embedding0_char, HBM_embedding0_size);
    HBM_embedding1_fstream.read(HBM_embedding1_char, HBM_embedding1_size);
    HBM_embedding2_fstream.read(HBM_embedding2_char, HBM_embedding2_size);
    HBM_embedding3_fstream.read(HBM_embedding3_char, HBM_embedding3_size);


    HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid_fstream.read(
        HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid_char,
        HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid_size);
    HBM_query_vector_fstream.read(HBM_query_vector_char, HBM_query_vector_size);
    HBM_vector_quantizer_fstream.read(HBM_vector_quantizer_char, HBM_vector_quantizer_size);
    HBM_product_quantizer_fstream.read(HBM_product_quantizer_char, HBM_product_quantizer_size);
#ifdef OPQ_ENABLE
    HBM_OPQ_matrix_fstream.read(HBM_OPQ_matrix_char, HBM_OPQ_matrix_size);
#endif

    raw_gt_vec_ID_fstream.read(raw_gt_vec_ID_char, raw_gt_vec_ID_size);

    // std::cout << "HBM_query_vector_fstream read bytes: " << HBM_query_vector_fstream.gcount() << std::endl;
    // std::cout << "HBM_vector_quantizer_fstream read bytes: " << HBM_vector_quantizer_fstream.gcount() << std::endl;
    // std::cout << "HBM_product_quantizer_fstream read bytes: " << HBM_product_quantizer_fstream.gcount() << std::endl;
 
    memcpy(&HBM_embedding0[0], HBM_embedding0_char, HBM_embedding0_size);
    memcpy(&HBM_embedding1[0], HBM_embedding1_char, HBM_embedding1_size);
    memcpy(&HBM_embedding2[0], HBM_embedding2_char, HBM_embedding2_size);
    memcpy(&HBM_embedding3[0], HBM_embedding3_char, HBM_embedding3_size);


    memcpy(&HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid[0], 
        HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid_char, 
        HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid_size);
    memcpy(&HBM_query_vectors[0], HBM_query_vector_char, HBM_query_vector_size);
    memcpy(&HBM_vector_quantizer[0], HBM_vector_quantizer_char, HBM_vector_quantizer_size);
    memcpy(&HBM_product_quantizer[0], HBM_product_quantizer_char, HBM_product_quantizer_size);
#ifdef OPQ_ENABLE
    memcpy(&HBM_OPQ_matrix[0], HBM_OPQ_matrix_char, HBM_OPQ_matrix_size);
#endif



    memcpy(&raw_gt_vec_ID[0], raw_gt_vec_ID_char, raw_gt_vec_ID_size);

    free(HBM_embedding0_char);
    free(HBM_embedding1_char);
    free(HBM_embedding2_char);
    free(HBM_embedding3_char);


    free(HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid_char);
    free(HBM_query_vector_char);
    free(HBM_vector_quantizer_char);
    free(HBM_product_quantizer_char);
#ifdef OPQ_ENABLE
    free(HBM_OPQ_matrix_char);
#endif

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


        HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_validExt, // HBM 21
        HBM_query_vectorExt, 
        HBM_vector_quantizerExt, 
        HBM_product_quantizerExt, 
#ifdef OPQ_ENABLE
        HBM_OPQ_matrixExt, 
#endif
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
    

//////////////////////////////   TEMPLATE START  //////////////////////////////
    OCL_CHECK(err, cl::Buffer buffer_HBM_embedding0(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX, 
            HBM_embedding0_size, &HBM_embedding0Ext, &err));
    OCL_CHECK(err, cl::Buffer buffer_HBM_embedding1(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX, 
            HBM_embedding1_size, &HBM_embedding1Ext, &err));
    OCL_CHECK(err, cl::Buffer buffer_HBM_embedding2(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX, 
            HBM_embedding2_size, &HBM_embedding2Ext, &err));
    OCL_CHECK(err, cl::Buffer buffer_HBM_embedding3(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX, 
            HBM_embedding3_size, &HBM_embedding3Ext, &err));



    OCL_CHECK(err, cl::Buffer buffer_HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid(
        context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX, 
        HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid_size, 
        &HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_validExt, &err));

    OCL_CHECK(err, cl::Buffer buffer_HBM_query_vectors(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX, 
            HBM_query_vector_size, &HBM_query_vectorExt, &err));
    OCL_CHECK(err, cl::Buffer buffer_HBM_vector_quantizer(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX, 
            HBM_vector_quantizer_size, &HBM_vector_quantizerExt, &err));
    OCL_CHECK(err, cl::Buffer buffer_HBM_product_quantizer(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX, 
            HBM_product_quantizer_size, &HBM_product_quantizerExt, &err));
#ifdef OPQ_ENABLE
    OCL_CHECK(err, cl::Buffer buffer_HBM_OPQ_matrix(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX, 
            HBM_OPQ_matrix_size, &HBM_OPQ_matrixExt, &err));
#endif
    
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


    
    OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, buffer_HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid));
    OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, buffer_HBM_query_vectors));
    OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, buffer_HBM_vector_quantizer));
    OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, buffer_HBM_product_quantizer));
#ifdef OPQ_ENABLE
    OCL_CHECK(err, err = krnl_vector_add.setArg(arg_counter++, buffer_HBM_OPQ_matrix));
#endif

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


        buffer_HBM_info_start_addr_and_scanned_entries_every_cell_and_last_element_valid,
        buffer_HBM_query_vectors,
        buffer_HBM_vector_quantizer,
        buffer_HBM_product_quantizer,
#ifdef OPQ_ENABLE
        buffer_HBM_OPQ_matrix
#endif
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
        for (int k = 0; k < TOPK; k++) {
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