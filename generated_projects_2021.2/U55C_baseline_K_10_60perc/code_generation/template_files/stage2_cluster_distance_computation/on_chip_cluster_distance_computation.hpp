#pragma once 

#include "constants.hpp"
#include "types.hpp"

////////////////////     Function to call in top-level     ////////////////////
template<const int query_num>
void compute_cell_distance_wrapper(
    const int centroids_per_partition_even, 
    const int centroids_per_partition_last_PE, 
    const int total_centriods,
    hls::stream<float> &s_centroid_vectors,
    hls::stream<float> &s_query_vectors,
    hls::stream<dist_cell_ID_t> &s_cell_distance);

////////////////////     Function to call in top-level     ////////////////////

//////////////////////////////////////////////////////////////////////////////////////////
//                    ARCHITECTURE DESIGN 
// Each PE in the systolic array contains 3~4 components
//  component A -> output compute 16 numbers per 2 CC
//  component B -> reduction 16 numbers per 2 CC -> output 1 number per 2 CC
//  component C -> consume 8 (D=128/M=16) numbers in 16 cycles, do reduction sum, and output
//  (optional) forward -> forward the result of the previous PE to the next PE
//////////////////////////////////////////////////////////////////////////////////////////

// Note, somehow the FIFOs can result in deadlock when the PE number is large,
//   for example, PE num = 12, FIFO depth 8 -> pass; PE num = 14, FIFO depth 8 -> stuck when nlist = 1024,8192...
// After using FIFO depth = 16 instead of 8, the PE num = 14 version works

template<const int query_num, const int centroids_per_partition_max>
void compute_cell_distance_head_component_A(
    const int systolic_array_id,
    const int centroids_per_partition,
    const int total_centriods, // nlist
    hls::stream<float>& s_centroid_vectors_in,
    hls::stream<float>& s_centroid_vectors_out,
    hls::stream<float>& s_query_vectors_in,
    hls::stream<float>& s_query_vectors_out,
    hls::stream<ap_uint512_t>& s_square_dist_pack) {

    // systolic_array_id should be the middle ids, 
    //   e.g., for systolic array of 16 PEs, one head, one tail, then middle should be 1~14

    // what is the speed needed to compute these distances? need to match HBM
    // python performance_estimation_three_nodes_IVF.py --nlist=8192 --nprobe=32
    // Estimated HBM Latency: 23.58 us -> 3K cycles
    // each URAM -> 64bit port x 2 -> 4 elements per cycle -> 64 cycles per vec
    //   -> at most 50 vectors per URAM (actually 1 URAM can only fit 32)
    // My strategy: 32 Partitions, each responsible for 256 vectors, 
    //   each vector finish in 10 cycles
    ap_uint<64> cell_centroids_partition[centroids_per_partition_max * D / 2];
#pragma HLS array_partition variable=cell_centroids_partition cyclic factor=8 
#pragma HLS resource variable=cell_centroids_partition core=RAM_2P_URAM

    float local_query_buffer[D];
#pragma HLS array_partition variable=local_query_buffer cyclic factor=8

    for (int c = 0; c < total_centriods; c++) {
        for (int d = 0; d < D / 2; d++) {
#pragma HLS pipeline II=1
            float reg_A = s_centroid_vectors_in.read();
            float reg_B = s_centroid_vectors_in.read();
            s_centroid_vectors_out.write(reg_A);
            s_centroid_vectors_out.write(reg_B);
            if ((c >= systolic_array_id * centroids_per_partition) && (c < (systolic_array_id + 1) * centroids_per_partition)) {
                ap_uint<32> reg_A_uint = *((ap_uint<32>*) (&reg_A));
                ap_uint<32> reg_B_uint = *((ap_uint<32>*) (&reg_B));
                ap_uint<64> val;
                val.range(31, 0) = reg_A_uint;
                val.range(63, 32) = reg_B_uint;
                cell_centroids_partition[(c - systolic_array_id * centroids_per_partition) * D / 2 + d] = val;
            }
        }
    }

    int start_cell_ID = centroids_per_partition * systolic_array_id;

    for (int query_id = 0; query_id < query_num; query_id++) {

        // load query vec
        for (int d = 0; d < D; d++) {
#pragma HLS pipeline II=1
            float reg = s_query_vectors_in.read();
            local_query_buffer[d] = reg;
            s_query_vectors_out.write(reg);
        }

        // compute distance and write results out
        for (int c = 0; c < centroids_per_partition; c++) {

            float distance = 0;

            // Manually unroll 16, auto-unroll doesn't work well
            for (int d = 0; d < D / 16; d++) {
#pragma HLS pipeline II=2
            //#pragma HLS UNROLL factor=16
                ap_uint<64> tmp_0 = cell_centroids_partition[(c * D + d * 16) / 2 + 0];
                ap_uint<64> tmp_1 = cell_centroids_partition[(c * D + d * 16) / 2 + 1];
                ap_uint<64> tmp_2 = cell_centroids_partition[(c * D + d * 16) / 2 + 2];
                ap_uint<64> tmp_3 = cell_centroids_partition[(c * D + d * 16) / 2 + 3];
                ap_uint<64> tmp_4 = cell_centroids_partition[(c * D + d * 16) / 2 + 4];
                ap_uint<64> tmp_5 = cell_centroids_partition[(c * D + d * 16) / 2 + 5];
                ap_uint<64> tmp_6 = cell_centroids_partition[(c * D + d * 16) / 2 + 6];
                ap_uint<64> tmp_7 = cell_centroids_partition[(c * D + d * 16) / 2 + 7];

                ap_uint<32> cell_centroids_0_int = tmp_0.range(31, 0);
                ap_uint<32> cell_centroids_1_int = tmp_0.range(63, 32);
                ap_uint<32> cell_centroids_2_int = tmp_1.range(31, 0);
                ap_uint<32> cell_centroids_3_int = tmp_1.range(63, 32);
                ap_uint<32> cell_centroids_4_int = tmp_2.range(31, 0);
                ap_uint<32> cell_centroids_5_int = tmp_2.range(63, 32);
                ap_uint<32> cell_centroids_6_int = tmp_3.range(31, 0);
                ap_uint<32> cell_centroids_7_int = tmp_3.range(63, 32);
                ap_uint<32> cell_centroids_8_int = tmp_4.range(31, 0);
                ap_uint<32> cell_centroids_9_int = tmp_4.range(63, 32);
                ap_uint<32> cell_centroids_10_int = tmp_5.range(31, 0);
                ap_uint<32> cell_centroids_11_int = tmp_5.range(63, 32);
                ap_uint<32> cell_centroids_12_int = tmp_6.range(31, 0);
                ap_uint<32> cell_centroids_13_int = tmp_6.range(63, 32);
                ap_uint<32> cell_centroids_14_int = tmp_7.range(31, 0);
                ap_uint<32> cell_centroids_15_int = tmp_7.range(63, 32);

                float cell_centroids_0 = *((float*) (&cell_centroids_0_int));
                float cell_centroids_1 = *((float*) (&cell_centroids_1_int));
                float cell_centroids_2 = *((float*) (&cell_centroids_2_int));
                float cell_centroids_3 = *((float*) (&cell_centroids_3_int));
                float cell_centroids_4 = *((float*) (&cell_centroids_4_int));
                float cell_centroids_5 = *((float*) (&cell_centroids_5_int));
                float cell_centroids_6 = *((float*) (&cell_centroids_6_int));
                float cell_centroids_7 = *((float*) (&cell_centroids_7_int));
                float cell_centroids_8 = *((float*) (&cell_centroids_8_int));
                float cell_centroids_9 = *((float*) (&cell_centroids_9_int));
                float cell_centroids_10 = *((float*) (&cell_centroids_10_int));
                float cell_centroids_11 = *((float*) (&cell_centroids_11_int));
                float cell_centroids_12 = *((float*) (&cell_centroids_12_int));
                float cell_centroids_13 = *((float*) (&cell_centroids_13_int));
                float cell_centroids_14 = *((float*) (&cell_centroids_14_int));
                float cell_centroids_15 = *((float*) (&cell_centroids_15_int));

                float scalar_dist_0 = local_query_buffer[d * 16 + 0] - cell_centroids_0;
                float scalar_dist_1 = local_query_buffer[d * 16 + 1] - cell_centroids_1;
                float scalar_dist_2 = local_query_buffer[d * 16 + 2] - cell_centroids_2;
                float scalar_dist_3 = local_query_buffer[d * 16 + 3] - cell_centroids_3;
                float scalar_dist_4 = local_query_buffer[d * 16 + 4] - cell_centroids_4;
                float scalar_dist_5 = local_query_buffer[d * 16 + 5] - cell_centroids_5;
                float scalar_dist_6 = local_query_buffer[d * 16 + 6] - cell_centroids_6;
                float scalar_dist_7 = local_query_buffer[d * 16 + 7] - cell_centroids_7;
                float scalar_dist_8 = local_query_buffer[d * 16 + 8] - cell_centroids_8;
                float scalar_dist_9 = local_query_buffer[d * 16 + 9] - cell_centroids_9;
                float scalar_dist_10 = local_query_buffer[d * 16 + 10] - cell_centroids_10;
                float scalar_dist_11 = local_query_buffer[d * 16 + 11] - cell_centroids_11;
                float scalar_dist_12 = local_query_buffer[d * 16 + 12] - cell_centroids_12;
                float scalar_dist_13 = local_query_buffer[d * 16 + 13] - cell_centroids_13;
                float scalar_dist_14 = local_query_buffer[d * 16 + 14] - cell_centroids_14;
                float scalar_dist_15 = local_query_buffer[d * 16 + 15] - cell_centroids_15;

                float square_dist_0 = scalar_dist_0 * scalar_dist_0;
                float square_dist_1 = scalar_dist_1 * scalar_dist_1;
                float square_dist_2 = scalar_dist_2 * scalar_dist_2;
                float square_dist_3 = scalar_dist_3 * scalar_dist_3;
                float square_dist_4 = scalar_dist_4 * scalar_dist_4;
                float square_dist_5 = scalar_dist_5 * scalar_dist_5;
                float square_dist_6 = scalar_dist_6 * scalar_dist_6;
                float square_dist_7 = scalar_dist_7 * scalar_dist_7;
                float square_dist_8 = scalar_dist_8 * scalar_dist_8;
                float square_dist_9 = scalar_dist_9 * scalar_dist_9;
                float square_dist_10 = scalar_dist_10 * scalar_dist_10;
                float square_dist_11 = scalar_dist_11 * scalar_dist_11;
                float square_dist_12 = scalar_dist_12 * scalar_dist_12;
                float square_dist_13 = scalar_dist_13 * scalar_dist_13;
                float square_dist_14 = scalar_dist_14 * scalar_dist_14;
                float square_dist_15 = scalar_dist_15 * scalar_dist_15;

                ap_uint<32> square_dist_0_uint = *((ap_uint<32>*) (&square_dist_0));
                ap_uint<32> square_dist_1_uint = *((ap_uint<32>*) (&square_dist_1));
                ap_uint<32> square_dist_2_uint = *((ap_uint<32>*) (&square_dist_2));
                ap_uint<32> square_dist_3_uint = *((ap_uint<32>*) (&square_dist_3));
                ap_uint<32> square_dist_4_uint = *((ap_uint<32>*) (&square_dist_4));
                ap_uint<32> square_dist_5_uint = *((ap_uint<32>*) (&square_dist_5));
                ap_uint<32> square_dist_6_uint = *((ap_uint<32>*) (&square_dist_6));
                ap_uint<32> square_dist_7_uint = *((ap_uint<32>*) (&square_dist_7));
                ap_uint<32> square_dist_8_uint = *((ap_uint<32>*) (&square_dist_8));
                ap_uint<32> square_dist_9_uint = *((ap_uint<32>*) (&square_dist_9));
                ap_uint<32> square_dist_10_uint = *((ap_uint<32>*) (&square_dist_10));
                ap_uint<32> square_dist_11_uint = *((ap_uint<32>*) (&square_dist_11));
                ap_uint<32> square_dist_12_uint = *((ap_uint<32>*) (&square_dist_12));
                ap_uint<32> square_dist_13_uint = *((ap_uint<32>*) (&square_dist_13));
                ap_uint<32> square_dist_14_uint = *((ap_uint<32>*) (&square_dist_14));
                ap_uint<32> square_dist_15_uint = *((ap_uint<32>*) (&square_dist_15));

                // pack 16 square distances, and send it to the next sub-PE component
                ap_uint512_t square_dist_pack;
                square_dist_pack.range((1 + 0) * 32 - 1, 0 * 32) = square_dist_0_uint;
                square_dist_pack.range((1 + 1) * 32 - 1, 1 * 32) = square_dist_1_uint;
                square_dist_pack.range((1 + 2) * 32 - 1, 2 * 32) = square_dist_2_uint;
                square_dist_pack.range((1 + 3) * 32 - 1, 3 * 32) = square_dist_3_uint;
                square_dist_pack.range((1 + 4) * 32 - 1, 4 * 32) = square_dist_4_uint;
                square_dist_pack.range((1 + 5) * 32 - 1, 5 * 32) = square_dist_5_uint;
                square_dist_pack.range((1 + 6) * 32 - 1, 6 * 32) = square_dist_6_uint;
                square_dist_pack.range((1 + 7) * 32 - 1, 7 * 32) = square_dist_7_uint;
                square_dist_pack.range((1 + 8) * 32 - 1, 8 * 32) = square_dist_8_uint;
                square_dist_pack.range((1 + 9) * 32 - 1, 9 * 32) = square_dist_9_uint;
                square_dist_pack.range((1 + 10) * 32 - 1, 10 * 32) = square_dist_10_uint;
                square_dist_pack.range((1 + 11) * 32 - 1, 11 * 32) = square_dist_11_uint;
                square_dist_pack.range((1 + 12) * 32 - 1, 12 * 32) = square_dist_12_uint;
                square_dist_pack.range((1 + 13) * 32 - 1, 13 * 32) = square_dist_13_uint;
                square_dist_pack.range((1 + 14) * 32 - 1, 14 * 32) = square_dist_14_uint;
                square_dist_pack.range((1 + 15) * 32 - 1, 15 * 32) = square_dist_15_uint;

                s_square_dist_pack.write(square_dist_pack);
            }
        }
    }
}


template<const int query_num, const int centroids_per_partition_max>
void compute_cell_distance_middle_component_A(
    const int systolic_array_id,
    const int centroids_per_partition,
    const int total_centriods, // nlist
    hls::stream<float>& s_centroid_vectors_in,
    hls::stream<float>& s_centroid_vectors_out,
    hls::stream<float>& s_query_vectors_in,
    hls::stream<float>& s_query_vectors_out,
    hls::stream<ap_uint512_t>& s_square_dist_pack) {

    // systolic_array_id should be the middle ids, 
    //   e.g., for systolic array of 16 PEs, one head, one tail, then middle should be 1~14

    // what is the speed needed to compute these distances? need to match HBM
    // python performance_estimation_three_nodes_IVF.py --nlist=8192 --nprobe=32
    // Estimated HBM Latency: 23.58 us -> 3K cycles
    // each URAM -> 64bit port x 2 -> 4 elements per cycle -> 64 cycles per vec
    //   -> at most 50 vectors per URAM (actually 1 URAM can only fit 32)
    // My strategy: 32 Partitions, each responsible for 256 vectors, 
    //   each vector finish in 10 cycles

    ap_uint<64> cell_centroids_partition[centroids_per_partition_max * D / 2];
#pragma HLS array_partition variable=cell_centroids_partition cyclic factor=8 
#pragma HLS resource variable=cell_centroids_partition core=RAM_2P_URAM

    float local_query_buffer[D];
#pragma HLS array_partition variable=local_query_buffer cyclic factor=8

    int start_cell_ID = centroids_per_partition * systolic_array_id;

    for (int c = 0; c < total_centriods; c++) {
        for (int d = 0; d < D / 2; d++) {
#pragma HLS pipeline II=1
            float reg_A = s_centroid_vectors_in.read();
            float reg_B = s_centroid_vectors_in.read();
            s_centroid_vectors_out.write(reg_A);
            s_centroid_vectors_out.write(reg_B);
            if ((c >= systolic_array_id * centroids_per_partition) && (c < (systolic_array_id + 1) * centroids_per_partition)) {
                ap_uint<32> reg_A_uint = *((ap_uint<32>*) (&reg_A));
                ap_uint<32> reg_B_uint = *((ap_uint<32>*) (&reg_B));
                ap_uint<64> val;
                val.range(31, 0) = reg_A_uint;
                val.range(63, 32) = reg_B_uint;
                cell_centroids_partition[(c - systolic_array_id * centroids_per_partition) * D / 2 + d] = val;
            }
        }
    }

    for (int query_id = 0; query_id < query_num; query_id++) {

        // load query vec
        for (int d = 0; d < D; d++) {
#pragma HLS pipeline II=1
            float reg = s_query_vectors_in.read();
            local_query_buffer[d] = reg;
            s_query_vectors_out.write(reg);
        }

        // compute distance and write results out
        for (int c = 0; c < centroids_per_partition; c++) {

            float distance = 0;

            // Manually unroll 16, auto-unroll doesn't work well
            for (int d = 0; d < D / 16; d++) {
#pragma HLS pipeline II=2
                ap_uint<64> tmp_0 = cell_centroids_partition[(c * D + d * 16) / 2 + 0];
                ap_uint<64> tmp_1 = cell_centroids_partition[(c * D + d * 16) / 2 + 1];
                ap_uint<64> tmp_2 = cell_centroids_partition[(c * D + d * 16) / 2 + 2];
                ap_uint<64> tmp_3 = cell_centroids_partition[(c * D + d * 16) / 2 + 3];
                ap_uint<64> tmp_4 = cell_centroids_partition[(c * D + d * 16) / 2 + 4];
                ap_uint<64> tmp_5 = cell_centroids_partition[(c * D + d * 16) / 2 + 5];
                ap_uint<64> tmp_6 = cell_centroids_partition[(c * D + d * 16) / 2 + 6];
                ap_uint<64> tmp_7 = cell_centroids_partition[(c * D + d * 16) / 2 + 7];

                ap_uint<32> cell_centroids_0_int = tmp_0.range(31, 0);
                ap_uint<32> cell_centroids_1_int = tmp_0.range(63, 32);
                ap_uint<32> cell_centroids_2_int = tmp_1.range(31, 0);
                ap_uint<32> cell_centroids_3_int = tmp_1.range(63, 32);
                ap_uint<32> cell_centroids_4_int = tmp_2.range(31, 0);
                ap_uint<32> cell_centroids_5_int = tmp_2.range(63, 32);
                ap_uint<32> cell_centroids_6_int = tmp_3.range(31, 0);
                ap_uint<32> cell_centroids_7_int = tmp_3.range(63, 32);
                ap_uint<32> cell_centroids_8_int = tmp_4.range(31, 0);
                ap_uint<32> cell_centroids_9_int = tmp_4.range(63, 32);
                ap_uint<32> cell_centroids_10_int = tmp_5.range(31, 0);
                ap_uint<32> cell_centroids_11_int = tmp_5.range(63, 32);
                ap_uint<32> cell_centroids_12_int = tmp_6.range(31, 0);
                ap_uint<32> cell_centroids_13_int = tmp_6.range(63, 32);
                ap_uint<32> cell_centroids_14_int = tmp_7.range(31, 0);
                ap_uint<32> cell_centroids_15_int = tmp_7.range(63, 32);

                float cell_centroids_0 = *((float*) (&cell_centroids_0_int));
                float cell_centroids_1 = *((float*) (&cell_centroids_1_int));
                float cell_centroids_2 = *((float*) (&cell_centroids_2_int));
                float cell_centroids_3 = *((float*) (&cell_centroids_3_int));
                float cell_centroids_4 = *((float*) (&cell_centroids_4_int));
                float cell_centroids_5 = *((float*) (&cell_centroids_5_int));
                float cell_centroids_6 = *((float*) (&cell_centroids_6_int));
                float cell_centroids_7 = *((float*) (&cell_centroids_7_int));
                float cell_centroids_8 = *((float*) (&cell_centroids_8_int));
                float cell_centroids_9 = *((float*) (&cell_centroids_9_int));
                float cell_centroids_10 = *((float*) (&cell_centroids_10_int));
                float cell_centroids_11 = *((float*) (&cell_centroids_11_int));
                float cell_centroids_12 = *((float*) (&cell_centroids_12_int));
                float cell_centroids_13 = *((float*) (&cell_centroids_13_int));
                float cell_centroids_14 = *((float*) (&cell_centroids_14_int));
                float cell_centroids_15 = *((float*) (&cell_centroids_15_int));

                float scalar_dist_0 = local_query_buffer[d * 16 + 0] - cell_centroids_0;
                float scalar_dist_1 = local_query_buffer[d * 16 + 1] - cell_centroids_1;
                float scalar_dist_2 = local_query_buffer[d * 16 + 2] - cell_centroids_2;
                float scalar_dist_3 = local_query_buffer[d * 16 + 3] - cell_centroids_3;
                float scalar_dist_4 = local_query_buffer[d * 16 + 4] - cell_centroids_4;
                float scalar_dist_5 = local_query_buffer[d * 16 + 5] - cell_centroids_5;
                float scalar_dist_6 = local_query_buffer[d * 16 + 6] - cell_centroids_6;
                float scalar_dist_7 = local_query_buffer[d * 16 + 7] - cell_centroids_7;
                float scalar_dist_8 = local_query_buffer[d * 16 + 8] - cell_centroids_8;
                float scalar_dist_9 = local_query_buffer[d * 16 + 9] - cell_centroids_9;
                float scalar_dist_10 = local_query_buffer[d * 16 + 10] - cell_centroids_10;
                float scalar_dist_11 = local_query_buffer[d * 16 + 11] - cell_centroids_11;
                float scalar_dist_12 = local_query_buffer[d * 16 + 12] - cell_centroids_12;
                float scalar_dist_13 = local_query_buffer[d * 16 + 13] - cell_centroids_13;
                float scalar_dist_14 = local_query_buffer[d * 16 + 14] - cell_centroids_14;
                float scalar_dist_15 = local_query_buffer[d * 16 + 15] - cell_centroids_15;

                float square_dist_0 = scalar_dist_0 * scalar_dist_0;
                float square_dist_1 = scalar_dist_1 * scalar_dist_1;
                float square_dist_2 = scalar_dist_2 * scalar_dist_2;
                float square_dist_3 = scalar_dist_3 * scalar_dist_3;
                float square_dist_4 = scalar_dist_4 * scalar_dist_4;
                float square_dist_5 = scalar_dist_5 * scalar_dist_5;
                float square_dist_6 = scalar_dist_6 * scalar_dist_6;
                float square_dist_7 = scalar_dist_7 * scalar_dist_7;
                float square_dist_8 = scalar_dist_8 * scalar_dist_8;
                float square_dist_9 = scalar_dist_9 * scalar_dist_9;
                float square_dist_10 = scalar_dist_10 * scalar_dist_10;
                float square_dist_11 = scalar_dist_11 * scalar_dist_11;
                float square_dist_12 = scalar_dist_12 * scalar_dist_12;
                float square_dist_13 = scalar_dist_13 * scalar_dist_13;
                float square_dist_14 = scalar_dist_14 * scalar_dist_14;
                float square_dist_15 = scalar_dist_15 * scalar_dist_15;

                ap_uint<32> square_dist_0_uint = *((ap_uint<32>*) (&square_dist_0));
                ap_uint<32> square_dist_1_uint = *((ap_uint<32>*) (&square_dist_1));
                ap_uint<32> square_dist_2_uint = *((ap_uint<32>*) (&square_dist_2));
                ap_uint<32> square_dist_3_uint = *((ap_uint<32>*) (&square_dist_3));
                ap_uint<32> square_dist_4_uint = *((ap_uint<32>*) (&square_dist_4));
                ap_uint<32> square_dist_5_uint = *((ap_uint<32>*) (&square_dist_5));
                ap_uint<32> square_dist_6_uint = *((ap_uint<32>*) (&square_dist_6));
                ap_uint<32> square_dist_7_uint = *((ap_uint<32>*) (&square_dist_7));
                ap_uint<32> square_dist_8_uint = *((ap_uint<32>*) (&square_dist_8));
                ap_uint<32> square_dist_9_uint = *((ap_uint<32>*) (&square_dist_9));
                ap_uint<32> square_dist_10_uint = *((ap_uint<32>*) (&square_dist_10));
                ap_uint<32> square_dist_11_uint = *((ap_uint<32>*) (&square_dist_11));
                ap_uint<32> square_dist_12_uint = *((ap_uint<32>*) (&square_dist_12));
                ap_uint<32> square_dist_13_uint = *((ap_uint<32>*) (&square_dist_13));
                ap_uint<32> square_dist_14_uint = *((ap_uint<32>*) (&square_dist_14));
                ap_uint<32> square_dist_15_uint = *((ap_uint<32>*) (&square_dist_15));

                // pack 16 square distances, and send it to the next sub-PE component
                ap_uint512_t square_dist_pack;
                square_dist_pack.range((1 + 0) * 32 - 1, 0 * 32) = square_dist_0_uint;
                square_dist_pack.range((1 + 1) * 32 - 1, 1 * 32) = square_dist_1_uint;
                square_dist_pack.range((1 + 2) * 32 - 1, 2 * 32) = square_dist_2_uint;
                square_dist_pack.range((1 + 3) * 32 - 1, 3 * 32) = square_dist_3_uint;
                square_dist_pack.range((1 + 4) * 32 - 1, 4 * 32) = square_dist_4_uint;
                square_dist_pack.range((1 + 5) * 32 - 1, 5 * 32) = square_dist_5_uint;
                square_dist_pack.range((1 + 6) * 32 - 1, 6 * 32) = square_dist_6_uint;
                square_dist_pack.range((1 + 7) * 32 - 1, 7 * 32) = square_dist_7_uint;
                square_dist_pack.range((1 + 8) * 32 - 1, 8 * 32) = square_dist_8_uint;
                square_dist_pack.range((1 + 9) * 32 - 1, 9 * 32) = square_dist_9_uint;
                square_dist_pack.range((1 + 10) * 32 - 1, 10 * 32) = square_dist_10_uint;
                square_dist_pack.range((1 + 11) * 32 - 1, 11 * 32) = square_dist_11_uint;
                square_dist_pack.range((1 + 12) * 32 - 1, 12 * 32) = square_dist_12_uint;
                square_dist_pack.range((1 + 13) * 32 - 1, 13 * 32) = square_dist_13_uint;
                square_dist_pack.range((1 + 14) * 32 - 1, 14 * 32) = square_dist_14_uint;
                square_dist_pack.range((1 + 15) * 32 - 1, 15 * 32) = square_dist_15_uint;

                s_square_dist_pack.write(square_dist_pack);
            }
        }
    }
}

// centroids_per_partition_last_PE must < centroids_per_partition
template<const int query_num, const int centroids_per_partition_last_PE_max>
void compute_cell_distance_tail_component_A(
    const int systolic_array_id,
    const int centroids_per_partition,
    const int centroids_per_partition_last_PE,
    const int total_centriods, // nlist
    hls::stream<float>& s_centroid_vectors_in,
    hls::stream<float>& s_query_vectors_in,
    hls::stream<ap_uint512_t>& s_square_dist_pack) {

    // systolic_array_id should be the middle ids, 
    //   e.g., for systolic array of 16 PEs, one head, one tail, then middle should be 1~14

    // what is the speed needed to compute these distances? need to match HBM
    // python performance_estimation_three_nodes_IVF.py --nlist=8192 --nprobe=32
    // Estimated HBM Latency: 23.58 us -> 3K cycles
    // each URAM -> 64bit port x 2 -> 4 elements per cycle -> 64 cycles per vec
    //   -> at most 50 vectors per URAM (actually 1 URAM can only fit 32)
    // My strategy: 32 Partitions, each responsible for 256 vectors, 
    //   each vector finish in 10 cycles
    ap_uint<64> cell_centroids_partition[centroids_per_partition_last_PE_max * D / 2];
#pragma HLS array_partition variable=cell_centroids_partition cyclic factor=8 
#pragma HLS resource variable=cell_centroids_partition core=RAM_2P_URAM

    float local_query_buffer[D];
#pragma HLS array_partition variable=local_query_buffer cyclic factor=8

    // init before any computation
    for (int c = 0; c < total_centriods; c++) {
        for (int d = 0; d < D / 2; d++) {
#pragma HLS pipeline II=1
            float reg_A = s_centroid_vectors_in.read();
            float reg_B = s_centroid_vectors_in.read();
            if ((c >= systolic_array_id * centroids_per_partition) && (c < systolic_array_id * centroids_per_partition + centroids_per_partition_last_PE)) {
                ap_uint<32> reg_A_uint = *((ap_uint<32>*) (&reg_A));
                ap_uint<32> reg_B_uint = *((ap_uint<32>*) (&reg_B));
                ap_uint<64> val;
                val.range(31, 0) = reg_A_uint;
                val.range(63, 32) = reg_B_uint;
                cell_centroids_partition[(c - systolic_array_id * centroids_per_partition) * D / 2 + d] = val;
            }
        }
    }


    int start_cell_ID = centroids_per_partition * systolic_array_id;

    for (int query_id = 0; query_id < query_num; query_id++) {

        // load query vec
        for (int d = 0; d < D; d++) {
#pragma HLS pipeline II=1
            float reg = s_query_vectors_in.read();
            local_query_buffer[d] = reg;
        }

        // compute distance and write results out
        for (int c = 0; c < centroids_per_partition_last_PE; c++) {

            float distance = 0;

            // Manually unroll 16, auto-unroll doesn't work well
            for (int d = 0; d < D / 16; d++) {
#pragma HLS pipeline II=2
            //#pragma HLS UNROLL factor=16
                ap_uint<64> tmp_0 = cell_centroids_partition[(c * D + d * 16) / 2 + 0];
                ap_uint<64> tmp_1 = cell_centroids_partition[(c * D + d * 16) / 2 + 1];
                ap_uint<64> tmp_2 = cell_centroids_partition[(c * D + d * 16) / 2 + 2];
                ap_uint<64> tmp_3 = cell_centroids_partition[(c * D + d * 16) / 2 + 3];
                ap_uint<64> tmp_4 = cell_centroids_partition[(c * D + d * 16) / 2 + 4];
                ap_uint<64> tmp_5 = cell_centroids_partition[(c * D + d * 16) / 2 + 5];
                ap_uint<64> tmp_6 = cell_centroids_partition[(c * D + d * 16) / 2 + 6];
                ap_uint<64> tmp_7 = cell_centroids_partition[(c * D + d * 16) / 2 + 7];

                ap_uint<32> cell_centroids_0_int = tmp_0.range(31, 0);
                ap_uint<32> cell_centroids_1_int = tmp_0.range(63, 32);
                ap_uint<32> cell_centroids_2_int = tmp_1.range(31, 0);
                ap_uint<32> cell_centroids_3_int = tmp_1.range(63, 32);
                ap_uint<32> cell_centroids_4_int = tmp_2.range(31, 0);
                ap_uint<32> cell_centroids_5_int = tmp_2.range(63, 32);
                ap_uint<32> cell_centroids_6_int = tmp_3.range(31, 0);
                ap_uint<32> cell_centroids_7_int = tmp_3.range(63, 32);
                ap_uint<32> cell_centroids_8_int = tmp_4.range(31, 0);
                ap_uint<32> cell_centroids_9_int = tmp_4.range(63, 32);
                ap_uint<32> cell_centroids_10_int = tmp_5.range(31, 0);
                ap_uint<32> cell_centroids_11_int = tmp_5.range(63, 32);
                ap_uint<32> cell_centroids_12_int = tmp_6.range(31, 0);
                ap_uint<32> cell_centroids_13_int = tmp_6.range(63, 32);
                ap_uint<32> cell_centroids_14_int = tmp_7.range(31, 0);
                ap_uint<32> cell_centroids_15_int = tmp_7.range(63, 32);

                float cell_centroids_0 = *((float*) (&cell_centroids_0_int));
                float cell_centroids_1 = *((float*) (&cell_centroids_1_int));
                float cell_centroids_2 = *((float*) (&cell_centroids_2_int));
                float cell_centroids_3 = *((float*) (&cell_centroids_3_int));
                float cell_centroids_4 = *((float*) (&cell_centroids_4_int));
                float cell_centroids_5 = *((float*) (&cell_centroids_5_int));
                float cell_centroids_6 = *((float*) (&cell_centroids_6_int));
                float cell_centroids_7 = *((float*) (&cell_centroids_7_int));
                float cell_centroids_8 = *((float*) (&cell_centroids_8_int));
                float cell_centroids_9 = *((float*) (&cell_centroids_9_int));
                float cell_centroids_10 = *((float*) (&cell_centroids_10_int));
                float cell_centroids_11 = *((float*) (&cell_centroids_11_int));
                float cell_centroids_12 = *((float*) (&cell_centroids_12_int));
                float cell_centroids_13 = *((float*) (&cell_centroids_13_int));
                float cell_centroids_14 = *((float*) (&cell_centroids_14_int));
                float cell_centroids_15 = *((float*) (&cell_centroids_15_int));

                float scalar_dist_0 = local_query_buffer[d * 16 + 0] - cell_centroids_0;
                float scalar_dist_1 = local_query_buffer[d * 16 + 1] - cell_centroids_1;
                float scalar_dist_2 = local_query_buffer[d * 16 + 2] - cell_centroids_2;
                float scalar_dist_3 = local_query_buffer[d * 16 + 3] - cell_centroids_3;
                float scalar_dist_4 = local_query_buffer[d * 16 + 4] - cell_centroids_4;
                float scalar_dist_5 = local_query_buffer[d * 16 + 5] - cell_centroids_5;
                float scalar_dist_6 = local_query_buffer[d * 16 + 6] - cell_centroids_6;
                float scalar_dist_7 = local_query_buffer[d * 16 + 7] - cell_centroids_7;
                float scalar_dist_8 = local_query_buffer[d * 16 + 8] - cell_centroids_8;
                float scalar_dist_9 = local_query_buffer[d * 16 + 9] - cell_centroids_9;
                float scalar_dist_10 = local_query_buffer[d * 16 + 10] - cell_centroids_10;
                float scalar_dist_11 = local_query_buffer[d * 16 + 11] - cell_centroids_11;
                float scalar_dist_12 = local_query_buffer[d * 16 + 12] - cell_centroids_12;
                float scalar_dist_13 = local_query_buffer[d * 16 + 13] - cell_centroids_13;
                float scalar_dist_14 = local_query_buffer[d * 16 + 14] - cell_centroids_14;
                float scalar_dist_15 = local_query_buffer[d * 16 + 15] - cell_centroids_15;

                float square_dist_0 = scalar_dist_0 * scalar_dist_0;
                float square_dist_1 = scalar_dist_1 * scalar_dist_1;
                float square_dist_2 = scalar_dist_2 * scalar_dist_2;
                float square_dist_3 = scalar_dist_3 * scalar_dist_3;
                float square_dist_4 = scalar_dist_4 * scalar_dist_4;
                float square_dist_5 = scalar_dist_5 * scalar_dist_5;
                float square_dist_6 = scalar_dist_6 * scalar_dist_6;
                float square_dist_7 = scalar_dist_7 * scalar_dist_7;
                float square_dist_8 = scalar_dist_8 * scalar_dist_8;
                float square_dist_9 = scalar_dist_9 * scalar_dist_9;
                float square_dist_10 = scalar_dist_10 * scalar_dist_10;
                float square_dist_11 = scalar_dist_11 * scalar_dist_11;
                float square_dist_12 = scalar_dist_12 * scalar_dist_12;
                float square_dist_13 = scalar_dist_13 * scalar_dist_13;
                float square_dist_14 = scalar_dist_14 * scalar_dist_14;
                float square_dist_15 = scalar_dist_15 * scalar_dist_15;

                ap_uint<32> square_dist_0_uint = *((ap_uint<32>*) (&square_dist_0));
                ap_uint<32> square_dist_1_uint = *((ap_uint<32>*) (&square_dist_1));
                ap_uint<32> square_dist_2_uint = *((ap_uint<32>*) (&square_dist_2));
                ap_uint<32> square_dist_3_uint = *((ap_uint<32>*) (&square_dist_3));
                ap_uint<32> square_dist_4_uint = *((ap_uint<32>*) (&square_dist_4));
                ap_uint<32> square_dist_5_uint = *((ap_uint<32>*) (&square_dist_5));
                ap_uint<32> square_dist_6_uint = *((ap_uint<32>*) (&square_dist_6));
                ap_uint<32> square_dist_7_uint = *((ap_uint<32>*) (&square_dist_7));
                ap_uint<32> square_dist_8_uint = *((ap_uint<32>*) (&square_dist_8));
                ap_uint<32> square_dist_9_uint = *((ap_uint<32>*) (&square_dist_9));
                ap_uint<32> square_dist_10_uint = *((ap_uint<32>*) (&square_dist_10));
                ap_uint<32> square_dist_11_uint = *((ap_uint<32>*) (&square_dist_11));
                ap_uint<32> square_dist_12_uint = *((ap_uint<32>*) (&square_dist_12));
                ap_uint<32> square_dist_13_uint = *((ap_uint<32>*) (&square_dist_13));
                ap_uint<32> square_dist_14_uint = *((ap_uint<32>*) (&square_dist_14));
                ap_uint<32> square_dist_15_uint = *((ap_uint<32>*) (&square_dist_15));

                // pack 16 square distances, and send it to the next sub-PE component
                ap_uint512_t square_dist_pack;
                square_dist_pack.range((1 + 0) * 32 - 1, 0 * 32) = square_dist_0_uint;
                square_dist_pack.range((1 + 1) * 32 - 1, 1 * 32) = square_dist_1_uint;
                square_dist_pack.range((1 + 2) * 32 - 1, 2 * 32) = square_dist_2_uint;
                square_dist_pack.range((1 + 3) * 32 - 1, 3 * 32) = square_dist_3_uint;
                square_dist_pack.range((1 + 4) * 32 - 1, 4 * 32) = square_dist_4_uint;
                square_dist_pack.range((1 + 5) * 32 - 1, 5 * 32) = square_dist_5_uint;
                square_dist_pack.range((1 + 6) * 32 - 1, 6 * 32) = square_dist_6_uint;
                square_dist_pack.range((1 + 7) * 32 - 1, 7 * 32) = square_dist_7_uint;
                square_dist_pack.range((1 + 8) * 32 - 1, 8 * 32) = square_dist_8_uint;
                square_dist_pack.range((1 + 9) * 32 - 1, 9 * 32) = square_dist_9_uint;
                square_dist_pack.range((1 + 10) * 32 - 1, 10 * 32) = square_dist_10_uint;
                square_dist_pack.range((1 + 11) * 32 - 1, 11 * 32) = square_dist_11_uint;
                square_dist_pack.range((1 + 12) * 32 - 1, 12 * 32) = square_dist_12_uint;
                square_dist_pack.range((1 + 13) * 32 - 1, 13 * 32) = square_dist_13_uint;
                square_dist_pack.range((1 + 14) * 32 - 1, 14 * 32) = square_dist_14_uint;
                square_dist_pack.range((1 + 15) * 32 - 1, 15 * 32) = square_dist_15_uint;

                s_square_dist_pack.write(square_dist_pack);
            }
        }
    }
}


template<const int query_num>
void compute_cell_distance_component_B(
    const int centroids_per_partition,
    hls::stream<ap_uint512_t>& s_square_dist_pack,
    hls::stream<float>& s_partial_dist) {

    for (int query_id = 0; query_id < query_num; query_id++) {

        for (int c = 0; c < centroids_per_partition; c++) {

            // Manually unroll 16, auto-unroll doesn't work well
            for (int d = 0; d < D / 16; d++) {
#pragma HLS pipeline II=2

                // pack 16 square distances, and send it to the next sub-PE component
                ap_uint512_t square_dist_pack = s_square_dist_pack.read();
                ap_uint<32> square_dist_0_uint = square_dist_pack.range((1 + 0) * 32 - 1, 0 * 32);
                ap_uint<32> square_dist_1_uint = square_dist_pack.range((1 + 1) * 32 - 1, 1 * 32);
                ap_uint<32> square_dist_2_uint = square_dist_pack.range((1 + 2) * 32 - 1, 2 * 32);
                ap_uint<32> square_dist_3_uint = square_dist_pack.range((1 + 3) * 32 - 1, 3 * 32);
                ap_uint<32> square_dist_4_uint = square_dist_pack.range((1 + 4) * 32 - 1, 4 * 32);
                ap_uint<32> square_dist_5_uint = square_dist_pack.range((1 + 5) * 32 - 1, 5 * 32);
                ap_uint<32> square_dist_6_uint = square_dist_pack.range((1 + 6) * 32 - 1, 6 * 32);
                ap_uint<32> square_dist_7_uint = square_dist_pack.range((1 + 7) * 32 - 1, 7 * 32);
                ap_uint<32> square_dist_8_uint = square_dist_pack.range((1 + 8) * 32 - 1, 8 * 32);
                ap_uint<32> square_dist_9_uint = square_dist_pack.range((1 + 9) * 32 - 1, 9 * 32);
                ap_uint<32> square_dist_10_uint = square_dist_pack.range((1 + 10) * 32 - 1, 10 * 32);
                ap_uint<32> square_dist_11_uint = square_dist_pack.range((1 + 11) * 32 - 1, 11 * 32);
                ap_uint<32> square_dist_12_uint = square_dist_pack.range((1 + 12) * 32 - 1, 12 * 32);
                ap_uint<32> square_dist_13_uint = square_dist_pack.range((1 + 13) * 32 - 1, 13 * 32);
                ap_uint<32> square_dist_14_uint = square_dist_pack.range((1 + 14) * 32 - 1, 14 * 32);
                ap_uint<32> square_dist_15_uint = square_dist_pack.range((1 + 15) * 32 - 1, 15 * 32);

                float square_dist_0 = *((float*) (&square_dist_0_uint));
                float square_dist_1 = *((float*) (&square_dist_1_uint));
                float square_dist_2 = *((float*) (&square_dist_2_uint));
                float square_dist_3 = *((float*) (&square_dist_3_uint));
                float square_dist_4 = *((float*) (&square_dist_4_uint));
                float square_dist_5 = *((float*) (&square_dist_5_uint));
                float square_dist_6 = *((float*) (&square_dist_6_uint));
                float square_dist_7 = *((float*) (&square_dist_7_uint));
                float square_dist_8 = *((float*) (&square_dist_8_uint));
                float square_dist_9 = *((float*) (&square_dist_9_uint));
                float square_dist_10 = *((float*) (&square_dist_10_uint));
                float square_dist_11 = *((float*) (&square_dist_11_uint));
                float square_dist_12 = *((float*) (&square_dist_12_uint));
                float square_dist_13 = *((float*) (&square_dist_13_uint));
                float square_dist_14 = *((float*) (&square_dist_14_uint));
                float square_dist_15 = *((float*) (&square_dist_15_uint));

                float distance = 
                    square_dist_0 + square_dist_1 + square_dist_2 + square_dist_3 + 
                    square_dist_4 + square_dist_5 + square_dist_6 + square_dist_7 + 
                    square_dist_8 + square_dist_9 + square_dist_10 + square_dist_11 + 
                    square_dist_12 + square_dist_13 + square_dist_14 + square_dist_15; 
                s_partial_dist.write(distance);
            }
        }
    }
}

template<const int query_num>
void compute_cell_distance_component_C(
    const int centroids_per_partition,
    const int start_cell_ID,
    hls::stream<float>& s_partial_dist,
    hls::stream<dist_cell_ID_t>& s_partial_cell_PE_result) {

    for (int query_id = 0; query_id < query_num; query_id++) {

        // compute distance and write results out
        for (int c = 0; c < centroids_per_partition; c++) {
#pragma HLS pipeline II=16 // match the speed of component A & B

            float distances[D / 16];
#pragma HLS array_partition variable=distances complete
            for (int d = 0; d < D / 16; d++) {
                distances[d] = s_partial_dist.read();
            }

            float distance = distances[0] + distances[1] + distances[2] + 
                distances[3] + distances[4] + distances[5] + distances[6] + distances[7];
            dist_cell_ID_t out;
            out.dist = distance;
            out.cell_ID = start_cell_ID + c;

            s_partial_cell_PE_result.write(out);
        }
    }
}

template<const int query_num>
void forward_cell_distance_middle(
    const int systolic_array_id,
    const int centroids_per_partition,
    hls::stream<dist_cell_ID_t>& s_partial_cell_PE_result,
    hls::stream<dist_cell_ID_t>& s_partial_cell_distance_in,
    hls::stream<dist_cell_ID_t>& s_partial_cell_distance_out) {


    for (int query_id = 0; query_id < query_num; query_id++) {

        // compute distance and write results out
        for (int c = 0; c < centroids_per_partition; c++) {

            // when the PE with larger ID finished computing, previous PE should already finished them
            for (int forward_iter = 0; forward_iter < systolic_array_id + 1; forward_iter++) {
#pragma HLS pipeline II=1
                dist_cell_ID_t out;
                if (forward_iter < systolic_array_id) {
                    out = s_partial_cell_distance_in.read();
                }
                else{
                    out = s_partial_cell_PE_result.read();
                }
                s_partial_cell_distance_out.write(out);
            }
        }
    }
}


// centroids_per_partition_last_PE must < centroids_per_partition
template<const int query_num>
void forward_cell_distance_tail(
    const int systolic_array_id,
    const int centroids_per_partition, 
    const int centroids_per_partition_last_PE, 
    const int total_centriods,
    hls::stream<dist_cell_ID_t>& s_partial_cell_PE_result,
    hls::stream<dist_cell_ID_t>& s_partial_cell_distance_in,
    hls::stream<dist_cell_ID_t>& s_cell_distance_out) {

    for (int query_id = 0; query_id < query_num; query_id++) {

        // compute distance and write results out
        for (int c = 0; c < centroids_per_partition_last_PE; c++) {

            // when the PE with larger ID finished computing, previous PE should already finished them
            for (int forward_iter = 0; forward_iter < systolic_array_id + 1; forward_iter++) {
#pragma HLS pipeline II=1
                dist_cell_ID_t out;
                if (forward_iter < systolic_array_id) {
                    out = s_partial_cell_distance_in.read();
                }
                else{
                    out = s_partial_cell_PE_result.read();
                }
                s_cell_distance_out.write(out);
            }
        }

        // forward the rest
        for (int c = 0; c < centroids_per_partition - centroids_per_partition_last_PE; c++) {
            for (int forward_iter = 0; forward_iter < systolic_array_id; forward_iter++) {
#pragma HLS pipeline II=1
                s_cell_distance_out.write(s_partial_cell_distance_in.read());
            }
        }
    }
}


template<const int query_num, const int centroids_per_partition_max>
void compute_cell_distance_head_PE(
    const int systolic_array_id,
    const int centroids_per_partition, 
    const int total_centriods,
    hls::stream<float>& s_centroid_vectors_in,
    hls::stream<float>& s_centroid_vectors_out,
    hls::stream<float>& s_query_vectors_in,
    hls::stream<float>& s_query_vectors_out,
    hls::stream<dist_cell_ID_t>& s_partial_cell_distance_out) {

#pragma HLS dataflow

    hls::stream<ap_uint512_t> s_square_dist_pack;
#pragma HLS stream variable=s_square_dist_pack depth=16

    hls::stream<float> s_partial_dist;
#pragma HLS stream variable=s_partial_dist depth=16

    compute_cell_distance_head_component_A<query_num, centroids_per_partition_max>(
        systolic_array_id,
        centroids_per_partition,
        total_centriods, 
        s_centroid_vectors_in,
        s_centroid_vectors_out,
        s_query_vectors_in,
        s_query_vectors_out,
        s_square_dist_pack); 

    compute_cell_distance_component_B<query_num>(
        centroids_per_partition,
        s_square_dist_pack,
        s_partial_dist);
        
    const int start_cell_ID = centroids_per_partition * systolic_array_id;
    compute_cell_distance_component_C<query_num>(
        centroids_per_partition,
        start_cell_ID,
        s_partial_dist,
        s_partial_cell_distance_out); 
}

template<const int query_num, const int centroids_per_partition_max>
void compute_cell_distance_middle_PE(
    const int systolic_array_id,
    const int centroids_per_partition, 
    const int total_centriods,
    hls::stream<float>& s_centroid_vectors_in,
    hls::stream<float>& s_centroid_vectors_out,
    hls::stream<float>& s_query_vectors_in,
    hls::stream<float>& s_query_vectors_out,
    hls::stream<dist_cell_ID_t>& s_partial_cell_distance_in,
    hls::stream<dist_cell_ID_t>& s_partial_cell_distance_out) {

#pragma HLS dataflow

    hls::stream<dist_cell_ID_t> s_partial_cell_PE_result;
#pragma HLS stream variable=s_partial_cell_PE_result depth=16

    hls::stream<ap_uint512_t> s_square_dist_pack;
#pragma HLS stream variable=s_square_dist_pack depth=16

    hls::stream<float> s_partial_dist;
#pragma HLS stream variable=s_partial_dist depth=16

    compute_cell_distance_middle_component_A<query_num, centroids_per_partition_max>(
        systolic_array_id,
        centroids_per_partition,
        total_centriods, 
        s_centroid_vectors_in,
        s_centroid_vectors_out,
        s_query_vectors_in,
        s_query_vectors_out,
        s_square_dist_pack);
   
    compute_cell_distance_component_B<query_num>(
        centroids_per_partition,
        s_square_dist_pack,
        s_partial_dist);
        
    const int start_cell_ID = centroids_per_partition * systolic_array_id;
    compute_cell_distance_component_C<query_num>(
        centroids_per_partition,
        start_cell_ID,
        s_partial_dist,
        s_partial_cell_PE_result); 

    forward_cell_distance_middle<query_num>(
        systolic_array_id,
        centroids_per_partition,
        s_partial_cell_PE_result,
        s_partial_cell_distance_in,
        s_partial_cell_distance_out);
}

// centroids_per_partition_last_PE must < centroids_per_partition
template<const int query_num, const int centroids_per_partition_last_PE_max>
void compute_cell_distance_tail_PE(
    const int systolic_array_id,
    const int centroids_per_partition, 
    const int centroids_per_partition_last_PE, 
    const int total_centriods,
    hls::stream<float>& s_centroid_vectors_in,
    hls::stream<float>& s_query_vectors_in,
    hls::stream<dist_cell_ID_t>& s_partial_cell_distance_in,
    hls::stream<dist_cell_ID_t>& s_cell_distance_out) {
       
#pragma HLS dataflow

    hls::stream<dist_cell_ID_t> s_partial_cell_PE_result;
#pragma HLS stream variable=s_partial_cell_PE_result depth=16 

    hls::stream<ap_uint512_t> s_square_dist_pack;
#pragma HLS stream variable=s_square_dist_pack depth=16 

    hls::stream<float> s_partial_dist;
#pragma HLS stream variable=s_partial_dist depth=16

    compute_cell_distance_tail_component_A<query_num, centroids_per_partition_last_PE_max>(
        systolic_array_id,
        centroids_per_partition, 
        centroids_per_partition_last_PE, 
        total_centriods,
        s_centroid_vectors_in,
        s_query_vectors_in,
        s_square_dist_pack);
    
    compute_cell_distance_component_B<query_num>(
        centroids_per_partition_last_PE,
        s_square_dist_pack,
        s_partial_dist);
        
    const int start_cell_ID = centroids_per_partition * systolic_array_id;
    compute_cell_distance_component_C<query_num>(
        centroids_per_partition_last_PE,
        start_cell_ID,
        s_partial_dist,
        s_partial_cell_PE_result); 

    forward_cell_distance_tail<query_num>(
        systolic_array_id,
        centroids_per_partition, 
        centroids_per_partition_last_PE, 
        total_centriods,
        s_partial_cell_PE_result,
        s_partial_cell_distance_in,
        s_cell_distance_out);
}

template<const int query_num>
void compute_cell_distance_wrapper(
    const int centroids_per_partition_even, 
    const int centroids_per_partition_last_PE, 
    const int total_centriods,
    hls::stream<float> &s_centroid_vectors,
    hls::stream<float> &s_query_vectors,
    hls::stream<dist_cell_ID_t> &s_cell_distance) {
#pragma HLS inline

    hls::stream<float> s_query_vectors_forward[PE_NUM_CENTER_DIST_COMP_EVEN];
#pragma HLS stream variable=s_query_vectors_forward depth=16
    hls::stream<float> s_centroid_vectors_forward[PE_NUM_CENTER_DIST_COMP_EVEN];
#pragma HLS stream variable=s_centroid_vectors_forward depth=16

    hls::stream<dist_cell_ID_t> s_partial_cell_distance_forward[PE_NUM_CENTER_DIST_COMP_EVEN];
#pragma HLS stream variable=s_partial_cell_distance_forward depth=16

    // head
    compute_cell_distance_head_PE<query_num, CENTROIDS_PER_PARTITION_EVEN_MAX>(
        0,
        centroids_per_partition_even, 
        total_centriods,
        s_centroid_vectors,
        s_centroid_vectors_forward[0],
        s_query_vectors,
        s_query_vectors_forward[0],
        s_partial_cell_distance_forward[0]); 

    // middle 
    for (int s = 1; s < PE_NUM_CENTER_DIST_COMP_EVEN; s++) {
#pragma HLS UNROLL
        compute_cell_distance_middle_PE<query_num, CENTROIDS_PER_PARTITION_EVEN_MAX>(
            s,
            centroids_per_partition_even, 
            total_centriods,
            s_centroid_vectors_forward[s - 1],
            s_centroid_vectors_forward[s],
            s_query_vectors_forward[s - 1],
            s_query_vectors_forward[s],
            s_partial_cell_distance_forward[s - 1],
            s_partial_cell_distance_forward[s]);
    }

    // tail
    compute_cell_distance_tail_PE<query_num, CENTROIDS_PER_PARTITION_LAST_PE_MAX>(
        PE_NUM_CENTER_DIST_COMP_EVEN,
        centroids_per_partition_even, 
        centroids_per_partition_last_PE, 
        total_centriods,
        s_centroid_vectors_forward[PE_NUM_CENTER_DIST_COMP_EVEN - 1],
        s_query_vectors_forward[PE_NUM_CENTER_DIST_COMP_EVEN - 1],
        s_partial_cell_distance_forward[PE_NUM_CENTER_DIST_COMP_EVEN - 1],
        s_cell_distance);
}
