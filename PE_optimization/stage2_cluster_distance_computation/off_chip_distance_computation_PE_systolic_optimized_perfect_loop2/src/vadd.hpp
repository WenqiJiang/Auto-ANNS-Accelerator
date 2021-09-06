#include <hls_stream.h>

#include "constants.hpp"
#include "types.hpp"

template<const int query_num>
void broadcast_query_vector(
    const float* table_DDR0,
    hls::stream<float>& s_query_vectors);

void broadcast_init_centroid_vectors(
    const float* table_DDR1,
    hls::stream<float>& s_centroid_vectors);

template<const int total_len>
void write_result(
    hls::stream<dist_cell_ID_t>& s_result, ap_uint<64>* results_out);
