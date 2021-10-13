#pragma once 

#include "constants.hpp"
#include "types.hpp"


template<const int query_num>
void collect_result_after_stage5(
    // input
#if SORT_GROUP_NUM
    hls::stream<single_PQ_result> (&s_single_PQ_result)[SORT_GROUP_NUM][16],
#else
    hls::stream<single_PQ_result> (&s_single_PQ_result)[STAGE5_COMP_PE_NUM],
#endif
    // control signals to be consumed
#if SORT_GROUP_NUM
    hls::stream<int> &s_scanned_entries_per_query_Sort_and_reduction,
#endif 
    hls::stream<int> &s_scanned_entries_per_query_Priority_queue,
    // output
    ap_uint64_t* HBM_out
    ) {

#if SORT_GROUP_NUM
    single_PQ_result result_buffer[SORT_GROUP_NUM][16];
#pragma HLS array_partition variable=result_buffer complete
#else
    single_PQ_result result_buffer[STAGE5_COMP_PE_NUM];
#pragma HLS array_partition variable=result_buffer complete
#endif

    for (int query_id = 0; query_id < query_num; query_id++) {
        
        int scanned_entries_per_query = s_scanned_entries_per_query_Priority_queue.read();
#if SORT_GROUP_NUM
        s_scanned_entries_per_query_Sort_and_reduction.read();
#endif

        for (int iter = 0; iter < scanned_entries_per_query; iter++) {

#if SORT_GROUP_NUM
            for (int s1 = 0; s1 < SORT_GROUP_NUM; s1++) {
#pragma HLS UNROLL
                for (int s2 = 0; s2 < 16; s2++) {
#pragma HLS UNROLL
                    result_buffer[s1][s2] = s_single_PQ_result[s1][s2].read();
                }
            }
#else
            for (int s = 0; s < STAGE5_COMP_PE_NUM; s++) {
#pragma HLS UNROLL
                result_buffer[s] = s_single_PQ_result[s].read();
            }
#endif
        }
    }

#if SORT_GROUP_NUM
    for (int i1 = 0; i1 < SORT_GROUP_NUM; i1++) {
        for (int i2 = 0; i2 < 16; i2++) {
            ap_uint<64> reg;
            int vec_ID = result_buffer[i1][i2].vec_ID;
            float dist = result_buffer[i1][i2].dist;
            reg.range(31, 0) = *((ap_uint<32>*) (&vec_ID));
            reg.range(63, 32) = *((ap_uint<32>*) (&dist));
            HBM_out[i1 * 16 + i2] = reg;
        }
    }
#else
    for (int i = 0; i < STAGE5_COMP_PE_NUM; i++) {
        ap_uint<64> reg;
        int vec_ID = result_buffer[i].vec_ID;
        float dist = result_buffer[i].dist;
        reg.range(31, 0) = *((ap_uint<32>*) (&vec_ID));
        reg.range(63, 32) = *((ap_uint<32>*) (&dist));
        HBM_out[i] = reg;
    }
#endif
}
