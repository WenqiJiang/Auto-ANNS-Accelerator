#pragma once 

#include "constants.hpp"
#include "types.hpp"

void collect_result_after_stage5(
    // input
    hls::stream<single_PQ_result> (&s_single_PQ_result)[STAGE5_COMP_PE_NUM],
    // control signals to be consumed
    hls::stream<int> &s_scanned_entries_per_query_Sort_and_reduction,
    // output
    ap_uint64_t* HBM_out
    ) {

    single_PQ_result result_buffer[STAGE5_COMP_PE_NUM];
#pragma HLS array_partition variable=result_buffer complete

    for (int query_id = 0; query_id < query_num; query_id++) {
        
        int scanned_entries_per_query = s_scanned_entries_per_query_Sort_and_reduction.read();

        for (int iter = 0; iter < scanned_entries_per_query; iter++) {

            for (int s = 0; s < STAGE5_COMP_PE_NUM; s++) {
#pragma HLS UNROLL
                result_buffer[s] = s_single_PQ_result[s].read();
            }
        }
    }

    for (int i = 0; i < STAGE5_COMP_PE_NUM; i++) {
        ap_uint<64> reg;
        int vec_ID = result_buffer[i].vec_ID;
        float dist = result_buffer[i].dist;
        reg.range(31, 0) = *((ap_uint<32>*) (&vec_ID));
        reg.range(63, 32) = *((ap_uint<32>*) (&dist));
        output[i] = reg;
    }
}

