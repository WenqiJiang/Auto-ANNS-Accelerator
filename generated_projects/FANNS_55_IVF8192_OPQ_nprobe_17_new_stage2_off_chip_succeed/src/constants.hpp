/*
Variable to be replaced (<--variable_name-->):

    NLIST
    NPROBE
    D
    M
    K
    PRIORITY_QUEUE_LEN
    
    OPQ_ENABLE

    QUERY_NUM

    LARGE_NUM

    // stage 2
    STAGE2_ON_CHIP
    PE_NUM_CENTER_DIST_COMP
    PE_NUM_CENTER_DIST_COMP_EVEN
    CENTROIDS_PER_PARTITION_EVEN
    CENTROIDS_PER_PARTITION_LAST_PE

    // stage 3
    STAGE_3_PRIORITY_QUEUE_LEVEL
    STAGE_3_PRIORITY_QUEUE_L1_NUM

    // stage 4
    PE_NUM_TABLE_CONSTRUCTION
    PE_NUM_TABLE_CONSTRUCTION_LARGER
    PE_NUM_TABLE_CONSTRUCTION_SMALLER
    NPROBE_PER_TABLE_CONSTRUCTION_PE_LARGER
    NPROBE_PER_TABLE_CONSTRUCTION_PE_SMALLER

    // stage 5
    HBM_CHANNEL_NUM
    SORT_GROUP_NUM
*/

#pragma once

#define NLIST 8192
#define NPROBE 17
#define D 128
#define M 16
#define K 256
#define PRIORITY_QUEUE_LEN 10

#define OPQ_ENABLE 1

#define QUERY_NUM 10000

#define LARGE_NUM 99999999 // used to init the heap

// stage 2
// 16 = 15 equal one + 1 (all equal) diff must be 1!

#define PE_NUM_CENTER_DIST_COMP 6
#define PE_NUM_CENTER_DIST_COMP_EVEN 5
#define CENTROIDS_PER_PARTITION_EVEN 1366
#define CENTROIDS_PER_PARTITION_LAST_PE 1362

// stage 3
// 2 levels, first level 2 queue, second level 1 queue
#define STAGE_3_PRIORITY_QUEUE_LEVEL 2
#define STAGE_3_PRIORITY_QUEUE_L1_NUM 2

// stage 4
// first PE: construct 9 tables per query, last one construct 8
#define PE_NUM_TABLE_CONSTRUCTION 6
#define PE_NUM_TABLE_CONSTRUCTION_LARGER 5
#define PE_NUM_TABLE_CONSTRUCTION_SMALLER 1
#define NPROBE_PER_TABLE_CONSTRUCTION_PE_LARGER 3
#define NPROBE_PER_TABLE_CONSTRUCTION_PE_SMALLER 2

// stage 5
#define HBM_CHANNEL_NUM 10
// number of 16 outputs per cycle, e.g., HBM channel num = 10, comp PE num = 30, then 
//   SORT_GROUP_NUM = 2; if HBM channel = 12, PE_num = 36, then SORT_GROUP_NUM = 3
#define SORT_GROUP_NUM 2


//////////////////////////////   TEMPLATE START  //////////////////////////////

#define PLRAM_BANK_NUM 4
#define HBM_BANK_NUM 32
#define DDR_BANK 2

/////////////////////////   HBM   ///////////////////////// 
// alignment of tables to HBM: 
// table 0 ~ 31 -> HBM 0 ~ 31
// table 32 ~ 63 -> HBM 0 ~ 31

// 256 MB = 268435456 bytes = 67108864 float32 = 4194304 ap_int<512>
// 128 MB = 2097152 ap_int<512>
#define HBM_BANK0_SIZE 2097152
#define HBM_BANK1_SIZE 2097152
#define HBM_BANK2_SIZE 2097152
#define HBM_BANK3_SIZE 2097152
#define HBM_BANK4_SIZE 2097152
#define HBM_BANK5_SIZE 2097152
#define HBM_BANK6_SIZE 2097152
#define HBM_BANK7_SIZE 2097152
#define HBM_BANK8_SIZE 2097152
#define HBM_BANK9_SIZE 2097152
#define HBM_BANK10_SIZE 2097152
#define HBM_BANK11_SIZE 2097152
#define HBM_BANK12_SIZE 2097152
#define HBM_BANK13_SIZE 2097152
#define HBM_BANK14_SIZE 2097152
#define HBM_BANK15_SIZE 2097152
#define HBM_BANK16_SIZE 2097152
#define HBM_BANK17_SIZE 2097152
#define HBM_BANK18_SIZE 2097152
#define HBM_BANK19_SIZE 2097152
#define HBM_BANK20_SIZE 2097152
#define HBM_BANK21_SIZE 2097152
#define HBM_BANK22_SIZE 2097152
#define HBM_BANK23_SIZE 2097152
#define HBM_BANK24_SIZE 2097152
#define HBM_BANK25_SIZE 2097152
#define HBM_BANK26_SIZE 2097152
#define HBM_BANK27_SIZE 2097152
#define HBM_BANK28_SIZE 2097152
#define HBM_BANK29_SIZE 2097152
#define HBM_BANK30_SIZE 2097152
#define HBM_BANK31_SIZE 2097152

#define DDR_BANK0_SIZE 1600000
#define DDR_BANK1_SIZE 1600000

#define PLRAM_BANK0_SIZE 28000
#define PLRAM_BANK1_SIZE 84000
#define PLRAM_BANK2_SIZE 84000
#define PLRAM_BANK3_SIZE 84000

//////////////////////////////   TEMPLATE END  //////////////////////////////



