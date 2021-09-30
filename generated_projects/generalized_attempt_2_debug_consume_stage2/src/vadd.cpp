/*
Variable to be replaced (<--variable_name-->):

    multiple lines (depends on HBM channel num):
        HBM_in_vadd_arg
        HBM_in_m_axi
        HBM_in_s_axilite
        load_and_split_PQ_codes_wrapper_arg

    multiple lines (depends on stage 2 PE num / on or off-chip):
        stage2_vadd_arg
        stage2_m_axi
        stage2_s_axilite

    single line:
        vadd_arg_OPQ_matrix
        vadd_m_axi_HBM_OPQ_matrix
        vadd_s_axilite_HBM_OPQ_matrix
        stage_1_OPQ_preprocessing
        stage_2_IVF_center_distance_computation
        stage6_sort_reduction
        stage6_priority_queue_group_L2_wrapper_stream_num
        stage6_priority_queue_group_L2_wrapper_arg
        
    basic constants:

*/

#include <stdio.h>
#include <hls_stream.h>

#include "constants.hpp"
#include "debugger.hpp"
#include "helpers.hpp"
#include "types.hpp"

// stage 1
#if OPQ_ENABLE
#include "OPQ_preprocessing.hpp"
#endif 

// stage 2
#include "cluster_distance_computation.hpp"

// stage 3
#include "priority_queue_vector_quantizer.hpp"
#include "select_Voronoi_cell.hpp"

// stage 4
#include "LUT_construction.hpp"

// stage 5
#include "distance_estimation_by_LUT.hpp"
#include "HBM_interconnections.hpp"

// stage 6
#include "priority_queue_distance_results_wrapper.hpp"
#if SORT_GROUP_NUM
#include "sort_reduction_with_vecID.hpp"
#endif

extern "C" {

// The argument of top-level function must not be too long,
//   otherwise there will be error when loading bitstreams.
// The safe number is <= 20 chararcters, but it could be longer
//   (the limit is not tested yet)
void vadd(  
    const ap_uint512_t* HBM_in0,
    const ap_uint512_t* HBM_in1,
    const ap_uint512_t* HBM_in2,
    const ap_uint512_t* HBM_in3,
    const ap_uint512_t* HBM_in4,
    const ap_uint512_t* HBM_in5,
    const ap_uint512_t* HBM_in6,
    const ap_uint512_t* HBM_in7,
    const ap_uint512_t* HBM_in8,
    const ap_uint512_t* HBM_in9,
    const ap_uint512_t* HBM_in10,
    const ap_uint512_t* HBM_in11,
    const ap_uint512_t* HBM_in12,
    const ap_uint512_t* HBM_in13,
    const ap_uint512_t* HBM_in14,
    const ap_uint512_t* HBM_in15,


    // HBM21: assigned for HBM_addr_info
    const int* HBM_addr_info, 
    // HBM22: query vectors
    float* HBM_query_vectors,
    // HBM23: center vector table (Vector_quantizer)
    float* HBM_vector_quantizer,
    // HBM24: PQ quantizer
    float* HBM_product_quantizer,

    const int nlist,
    const int nprobe,
    // stage 1 parameter
    const bool OPQ_enable,
    // stage 2 parameters
    const int c_per_part_even, 
    const int c_per_part_last, 
    // stage 4 parameters, if PE_NUM==1, set the same value
    //   nprobe_per_table_construction_pe_larger = nprobe_per_table_construction_pe_smaller
    const int np_per_pe_larger,
    const int np_per_pe_smaller,
    // HBM26: output (vector_ID, distance)
    ap_uint64_t* HBM_out
    // const ap_uint512_t* HBM_in22, const ap_uint512_t* HBM_in23, 
    // const ap_uint512_t* HBM_in24, const ap_uint512_t* HBM_in25, 
    // const ap_uint512_t* HBM_in26, const ap_uint512_t* HBM_in27, 
    // const ap_uint512_t* HBM_in28, const ap_uint512_t* HBM_in29, 
    // const ap_uint512_t* HBM_in30, const ap_uint512_t* HBM_in31, 
    // const ap_uint512_t* table_DDR0, const ap_uint512_t* table_DDR1, 
    )
{
#pragma HLS INTERFACE m_axi port=HBM_in0 offset=slave bundle=gmem0
#pragma HLS INTERFACE m_axi port=HBM_in1 offset=slave bundle=gmem1
#pragma HLS INTERFACE m_axi port=HBM_in2 offset=slave bundle=gmem2
#pragma HLS INTERFACE m_axi port=HBM_in3 offset=slave bundle=gmem3
#pragma HLS INTERFACE m_axi port=HBM_in4 offset=slave bundle=gmem4
#pragma HLS INTERFACE m_axi port=HBM_in5 offset=slave bundle=gmem5
#pragma HLS INTERFACE m_axi port=HBM_in6 offset=slave bundle=gmem6
#pragma HLS INTERFACE m_axi port=HBM_in7 offset=slave bundle=gmem7
#pragma HLS INTERFACE m_axi port=HBM_in8 offset=slave bundle=gmem8
#pragma HLS INTERFACE m_axi port=HBM_in9 offset=slave bundle=gmem9
#pragma HLS INTERFACE m_axi port=HBM_in10 offset=slave bundle=gmem10
#pragma HLS INTERFACE m_axi port=HBM_in11 offset=slave bundle=gmem11
#pragma HLS INTERFACE m_axi port=HBM_in12 offset=slave bundle=gmem12
#pragma HLS INTERFACE m_axi port=HBM_in13 offset=slave bundle=gmem13
#pragma HLS INTERFACE m_axi port=HBM_in14 offset=slave bundle=gmem14
#pragma HLS INTERFACE m_axi port=HBM_in15 offset=slave bundle=gmem15



#pragma HLS INTERFACE m_axi port=HBM_addr_info  offset=slave bundle=gmemA
#pragma HLS INTERFACE m_axi port=HBM_query_vectors  offset=slave bundle=gmemB
#pragma HLS INTERFACE m_axi port=HBM_vector_quantizer  offset=slave bundle=gmemC
#pragma HLS INTERFACE m_axi port=HBM_product_quantizer  offset=slave bundle=gmemD


#pragma HLS INTERFACE m_axi port=HBM_out offset=slave bundle=gmemF

#pragma HLS INTERFACE s_axilite port=HBM_in0
#pragma HLS INTERFACE s_axilite port=HBM_in1
#pragma HLS INTERFACE s_axilite port=HBM_in2
#pragma HLS INTERFACE s_axilite port=HBM_in3
#pragma HLS INTERFACE s_axilite port=HBM_in4
#pragma HLS INTERFACE s_axilite port=HBM_in5
#pragma HLS INTERFACE s_axilite port=HBM_in6
#pragma HLS INTERFACE s_axilite port=HBM_in7
#pragma HLS INTERFACE s_axilite port=HBM_in8
#pragma HLS INTERFACE s_axilite port=HBM_in9
#pragma HLS INTERFACE s_axilite port=HBM_in10
#pragma HLS INTERFACE s_axilite port=HBM_in11
#pragma HLS INTERFACE s_axilite port=HBM_in12
#pragma HLS INTERFACE s_axilite port=HBM_in13
#pragma HLS INTERFACE s_axilite port=HBM_in14
#pragma HLS INTERFACE s_axilite port=HBM_in15



#pragma HLS INTERFACE s_axilite port=HBM_addr_info 
#pragma HLS INTERFACE s_axilite port=HBM_query_vectors 
#pragma HLS INTERFACE s_axilite port=HBM_vector_quantizer 
#pragma HLS INTERFACE s_axilite port=HBM_product_quantizer 


#pragma HLS INTERFACE s_axilite port=nlist
#pragma HLS INTERFACE s_axilite port=nprobe
#pragma HLS INTERFACE s_axilite port=OPQ_enable
#pragma HLS INTERFACE s_axilite port=c_per_part_even
#pragma HLS INTERFACE s_axilite port=c_per_part_last
#pragma HLS INTERFACE s_axilite port=np_per_pe_larger
#pragma HLS INTERFACE s_axilite port=np_per_pe_smaller

#pragma HLS INTERFACE s_axilite port=HBM_out

#pragma HLS INTERFACE s_axilite port=return
    
#pragma HLS dataflow

    // name the input argument to longer version
    int centroids_per_partition_even = c_per_part_even;
    int centroids_per_partition_last_PE = c_per_part_last;

    int nprobe_per_table_construction_pe_larger = np_per_pe_larger;
    int nprobe_per_table_construction_pe_smaller = np_per_pe_smaller;

    ////////////////////     Init     ////////////////////

    hls::stream<float> s_query_vectors;
#pragma HLS stream variable=s_query_vectors depth=512
// #pragma HLS resource variable=s_query_vectors core=FIFO_BRAM

    load_query_vectors<QUERY_NUM>(
        HBM_query_vectors, 
        s_query_vectors);

    hls::stream<float> s_center_vectors_init_lookup_PE;
#pragma HLS stream variable=s_center_vectors_init_lookup_PE depth=2
// #pragma HLS resource variable=s_center_vectors_init_lookup_PE core=FIFO_SRL

#ifdef STAGE2_ON_CHIP
    hls::stream<float> s_center_vectors_init_distance_computation_PE;
#pragma HLS stream variable=s_center_vectors_init_distance_computation_PE depth=8

    load_center_vectors(
        nlist,
        HBM_vector_quantizer,
        s_center_vectors_init_distance_computation_PE,
        s_center_vectors_init_lookup_PE);
#endif

    hls::stream<float> s_PQ_quantizer_init;
#pragma HLS stream variable=s_PQ_quantizer_init depth=4
// #pragma HLS resource variable=s_PQ_quantizer_init core=FIFO_SRL

    load_PQ_quantizer(HBM_product_quantizer, s_PQ_quantizer_init);

    ////////////////////     Preprocessing    ////////////////////


    hls::stream<float> s_preprocessed_query_vectors_lookup_PE;
#pragma HLS stream variable=s_preprocessed_query_vectors_lookup_PE depth=512
// #pragma HLS resource variable=s_preprocessed_query_vectors_lookup_PE core=FIFO_BRAM

    hls::stream<float> s_preprocessed_query_vectors_distance_computation_PE;
#pragma HLS stream variable=s_preprocessed_query_vectors_distance_computation_PE depth=512
// #pragma HLS resource variable=s_preprocessed_query_vectors_distance_computation_PE core=FIFO_BRAM

    broadcast_preprocessed_query_vectors<QUERY_NUM>(
        s_query_vectors,
        s_preprocessed_query_vectors_distance_computation_PE,
        s_preprocessed_query_vectors_lookup_PE);

    ////////////////////      Center Distance Computation    ////////////////////

    hls::stream<dist_cell_ID_t> s_merged_cell_distance;
#pragma HLS stream variable=s_merged_cell_distance depth=512
// #pragma HLS resource variable=s_merged_cell_distance core=FIFO_BRAM


    compute_cell_distance_wrapper<QUERY_NUM>(
        centroids_per_partition_even, 
        centroids_per_partition_last_PE, 
        nlist,
        s_center_vectors_init_distance_computation_PE, 
        s_preprocessed_query_vectors_distance_computation_PE, 
        s_merged_cell_distance);

    collect_result_after_stage2<QUERY_NUM>(
        nlist,
        // input
        s_merged_cell_distance,
        // consume some unused streams
        s_PQ_quantizer_init,
        s_center_vectors_init_lookup_PE,
        s_preprocessed_query_vectors_lookup_PE,
        // output
        HBM_out);

//     ////////////////////     Select Scanned Cells     ////////////////////    

//     hls::stream<dist_cell_ID_t> s_selected_distance_cell_ID;
// #pragma HLS stream variable=s_selected_distance_cell_ID depth=512
// // #pragma HLS resource variable=s_selected_distance_cell_ID core=FIFO_BRAM

//     select_Voronoi_cell<STAGE_3_PRIORITY_QUEUE_LEVEL, STAGE_3_PRIORITY_QUEUE_L1_NUM, NPROBE_MAX>(
//         nlist,
//         nprobe,
//         s_merged_cell_distance,
//         s_selected_distance_cell_ID);

//     hls::stream<int> s_searched_cell_id_lookup_PE;
// #pragma HLS stream variable=s_searched_cell_id_lookup_PE depth=512
// // #pragma HLS resource variable=s_searched_cell_id_lookup_PE core=FIFO_BRAM

//     hls::stream<int> s_searched_cell_id_scan_controller;
// #pragma HLS stream variable=s_searched_cell_id_scan_controller depth=512
// // #pragma HLS resource variable=s_searched_cell_id_scan_controller core=FIFO_BRAM

//     //  dist struct to cell ID (int)
//     split_cell_ID<QUERY_NUM>(
//         nprobe,
//         s_selected_distance_cell_ID, 
//         s_searched_cell_id_lookup_PE, 
//         s_searched_cell_id_scan_controller);

//     ////////////////////     Center Vector Lookup     ////////////////////    

//     hls::stream<float> s_center_vectors_lookup_PE;
// #pragma HLS stream variable=s_center_vectors_lookup_PE depth=128
// // #pragma HLS resource variable=s_center_vectors_lookup_PE core=FIFO_BRAM

// #ifdef STAGE2_ON_CHIP
//     lookup_center_vectors<QUERY_NUM>(
//         nlist,
//         nprobe,
//         s_center_vectors_init_lookup_PE, 
//         s_searched_cell_id_lookup_PE, 
//         s_center_vectors_lookup_PE);
// #else
//     lookup_center_vectors<QUERY_NUM>(
//         nprobe,
//         HBM_vector_quantizer, 
//         s_searched_cell_id_lookup_PE, 
//         s_center_vectors_lookup_PE);
// #endif

//     ////////////////////     Distance Lookup Table Construction     ////////////////////    

//     hls::stream<distance_LUT_PQ16_t> s_distance_LUT;
// #pragma HLS stream variable=s_distance_LUT depth=512
// // #pragma HLS resource variable=s_distance_LUT core=FIFO_BRAM

// #if PE_NUM_TABLE_CONSTRUCTION == 1
//     lookup_table_construction_wrapper<QUERY_NUM>(
//         nprobe_per_table_construction_pe_larger,
//         s_PQ_quantizer_init, 
//         s_center_vectors_lookup_PE, 
//         s_preprocessed_query_vectors_lookup_PE, 
//         s_distance_LUT);
// #else
//     lookup_table_construction_wrapper<QUERY_NUM>(
//         nprobe_per_table_construction_pe_larger,
//         nprobe_per_table_construction_pe_smaller,
//         s_PQ_quantizer_init, 
//         s_center_vectors_lookup_PE, 
//         s_preprocessed_query_vectors_lookup_PE, 
//         s_distance_LUT);
// #endif
//     ////////////////////     Load PQ Codes     ////////////////////    

//     hls::stream<int> s_scanned_entries_every_cell_Load_unit;
// #pragma HLS stream variable=s_scanned_entries_every_cell_Load_unit depth=512
// // #pragma HLS RESOURCE variable=s_scanned_entries_every_cell_Load_unit core=FIFO_BRAM

//     hls::stream<int> s_scanned_entries_every_cell_PQ_lookup_computation;
// #pragma HLS stream variable=s_scanned_entries_every_cell_PQ_lookup_computation depth=512
// // #pragma HLS RESOURCE variable=s_scanned_entries_every_cell_PQ_lookup_computation core=FIFO_BRAM

//     hls::stream<int> s_last_valid_channel;
// #pragma HLS stream variable=s_last_valid_channel depth=512
// // #pragma HLS RESOURCE variable=s_last_valid_channel core=FIFO_BRAM

//     hls::stream<int> s_start_addr_every_cell;
// #pragma HLS stream variable=s_start_addr_every_cell depth=512
// // #pragma HLS RESOURCE variable=s_start_addr_every_cell core=FIFO_BRAM

// #if SORT_GROUP_NUM
//     hls::stream<int> s_scanned_entries_per_query_Sort_and_reduction;
// #pragma HLS stream variable=s_scanned_entries_per_query_Sort_and_reduction depth=512
// // #pragma HLS RESOURCE variable=s_scanned_entries_per_query_Sort_and_reduction core=FIFO_BRAM
// #endif
//     hls::stream<int> s_scanned_entries_per_query_Priority_queue;
// #pragma HLS stream variable=s_scanned_entries_per_query_Priority_queue depth=512
// // #pragma HLS RESOURCE variable=s_scanned_entries_per_query_Priority_queue core=FIFO_BRAM

//     scan_controller<QUERY_NUM>(
//         nlist, 
//         nprobe,
//         HBM_addr_info,
//         s_searched_cell_id_scan_controller, 
//         s_start_addr_every_cell,
//         s_scanned_entries_every_cell_Load_unit, 
//         s_scanned_entries_every_cell_PQ_lookup_computation,
//         s_last_valid_channel, 
// #if SORT_GROUP_NUM
//         s_scanned_entries_per_query_Sort_and_reduction,
// #endif
//         s_scanned_entries_per_query_Priority_queue);

//     // each 512 bit can store 3 set of (vecID, PQ code)
//     hls::stream<single_PQ> s_single_PQ[STAGE5_COMP_PE_NUM];
// #pragma HLS stream variable=s_single_PQ depth=8
// #pragma HLS array_partition variable=s_single_PQ complete
// // #pragma HLS RESOURCE variable=s_single_PQ core=FIFO_SRL

//     load_and_split_PQ_codes_wrapper<QUERY_NUM>(
//         nprobe,
//         HBM_in0,
//         HBM_in1,
//         HBM_in2,
//         HBM_in3,
//         HBM_in4,
//         HBM_in5,
//         HBM_in6,
//         HBM_in7,
//         HBM_in8,
//         HBM_in9,
//         HBM_in10,
//         HBM_in11,
//         HBM_in12,
//         HBM_in13,
//         HBM_in14,
//         HBM_in15,

//         s_start_addr_every_cell,
//         s_scanned_entries_every_cell_Load_unit,
//         s_single_PQ);

// #if SORT_GROUP_NUM
//     hls::stream<single_PQ_result> s_single_PQ_result[SORT_GROUP_NUM][16];
// #pragma HLS stream variable=s_single_PQ_result depth=8
// #pragma HLS array_partition variable=s_single_PQ_result complete
// // #pragma HLS RESOURCE variable=s_single_PQ_result core=FIFO_SRL
// #else
//     hls::stream<single_PQ_result> s_single_PQ_result[STAGE5_COMP_PE_NUM];
// #pragma HLS stream variable=s_single_PQ_result depth=8
// #pragma HLS array_partition variable=s_single_PQ_result complete
// // #pragma HLS RESOURCE variable=s_single_PQ_result core=FIFO_SRL
// #endif

//     ////////////////////     Estimate Distance by LUT     ////////////////////    

//     PQ_lookup_computation_wrapper<QUERY_NUM, STAGE5_COMP_PE_NUM, PQ_CODE_CHANNELS_PER_STREAM>(
//         nprobe,
//         s_single_PQ, 
//         s_distance_LUT, 
//         s_scanned_entries_every_cell_PQ_lookup_computation,
//         s_last_valid_channel,
//         s_single_PQ_result);


//         ////////////////////     Sort Results     ////////////////////    
//     Sort_reduction<single_PQ_result, SORT_GROUP_NUM * 16, TOPK, Collect_smallest> sort_reduction_module;

//     hls::stream<single_PQ_result> s_sorted_PQ_result[TOPK];
// #pragma HLS stream variable=s_sorted_PQ_result depth=8
// #pragma HLS array_partition variable=s_sorted_PQ_result complete
// // #pragma HLS RESOURCE variable=s_sorted_PQ_result core=FIFO_SRL

//     sort_reduction_module.sort_and_reduction<QUERY_NUM>(
//         s_scanned_entries_per_query_Sort_and_reduction, 
//         s_single_PQ_result, 
//         s_sorted_PQ_result);

//     hls::stream<single_PQ_result> s_output; // the top 10 numbers
// #pragma HLS stream variable=s_output depth=512
// // #pragma HLS RESOURCE variable=s_output core=FIFO_BRAM

//     stage6_priority_queue_group_L2_wrapper<QUERY_NUM, TOPK>(
//         s_scanned_entries_per_query_Priority_queue, 
//         s_sorted_PQ_result,
//         s_output);

//     ////////////////////     Write Results     ////////////////////    
//     write_result<QUERY_NUM>(s_output, HBM_out);
}

}