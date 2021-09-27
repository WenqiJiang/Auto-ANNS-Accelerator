#pragma once

#define NLIST_MAX 16384
#define NPROBE_MAX 128
// #define D 128
#define M 16
#define K 256
#define TOPK 10



#define QUERY_NUM 10000

#define LARGE_NUM 99999999 // used to init the heap

// stage 2
// 16 = 15 equal one + 1 (all equal) diff must be 1!
#define STAGE2_ON_CHIP 1
#define PE_NUM_CENTER_DIST_COMP 12

#define PE_NUM_CENTER_DIST_COMP_EVEN 11
#define CENTROIDS_PER_PARTITION_EVEN_MAX 1366
#define CENTROIDS_PER_PARTITION_LAST_PE_MAX 1358


// stage 3
// 2 levels, first level 2 queue, second level 1 queue
#define STAGE_3_PRIORITY_QUEUE_LEVEL 2
#define STAGE_3_PRIORITY_QUEUE_L1_NUM 2

// stage 4
// first PE: construct 9 tables per query, last one construct 8
#define PE_NUM_TABLE_CONSTRUCTION 6

#define PE_NUM_TABLE_CONSTRUCTION_LARGER 5
#define PE_NUM_TABLE_CONSTRUCTION_SMALLER 1


// stage 5
#define HBM_CHANNEL_NUM 16
#define STAGE5_COMP_PE_NUM 16
#define PQ_CODE_CHANNELS_PER_STREAM 3


// number of 16 outputs per cycle, e.g., HBM channel num = 10, comp PE num = 30, then 
//   SORT_GROUP_NUM = 2; if HBM channel = 12, PE_num = 36, then SORT_GROUP_NUM = 3
#define SORT_GROUP_NUM 1
#define STAGE_6_PRIORITY_QUEUE_LEVEL 2
#define STAGE_6_PRIORITY_QUEUE_L1_NUM 20
