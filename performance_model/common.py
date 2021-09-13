class Resource:
    """ Consumed FPGA resources of a component (PE, stage, etc.) """
    def __init__(self):
        self.HBM_bank = 0
        self.BRAM_18K = 0
        self.URAM = 0
        self.FF = 0
        self.LUT = 0
        self.DSP48E = 0

    def add_resource(self, another_Resource_obj, num=1):
        # add another component(s) to this componen
        self.HBM_bank += num * another_Resource_obj.HBM_bank
        self.BRAM_18K += num * another_Resource_obj.BRAM_18K
        self.URAM += num * another_Resource_obj.URAM
        self.FF += num * another_Resource_obj.FF
        self.LUT += num * another_Resource_obj.LUT
        self.DSP48E += num * another_Resource_obj.DSP48E
    
    def copy_from_Resource(self, Resource_obj):
        self.HBM_bank = Resource_obj.HBM_bank
        self.BRAM_18K = Resource_obj.BRAM_18K
        self.URAM = Resource_obj.URAM
        self.FF = Resource_obj.FF
        self.LUT = Resource_obj.LUT
        self.DSP48E = Resource_obj.DSP48E

    def print_resource(self):
        print("""
        HBM_bank: {HBM_bank}
        BRAM_18K: {BRAM_18K}
        URAM: {URAM}
        FF: {FF}
        LUT: {LUT}
        DSP48E: {DSP48E}
        """.format(
            HBM_bank=self.HBM_bank,
            BRAM_18K=self.BRAM_18K,
            URAM=self.URAM,
            FF=self.FF,
            LUT=self.LUT,
            DSP48E=self.DSP48E))


def sum_resource(Resource_obj_list, PE_num_list=None):
    """
    Given a list of Resource objects, return the total resource (Resource object)
    """
    total_resource = Resource()
    
    if PE_num_list is None:
        for Resource_obj in Resource_obj_list:
            total_resource.add_resource(Resource_obj)
    else:
        assert len(Resource_obj_list) == len(PE_num_list), \
            "Resource_obj_list and PE_num_list must have the same length"
        for i, Resource_obj in enumerate(Resource_obj_list):
            total_resource.add_resource(Resource_obj, PE_num_list[i])

    return total_resource


class Performance:
    """ Performance of a component (PE, stage, etc.) """
    def __init__(self):
        self.cycles_per_query = None
        self.QPS = None
    
    def print_performance(self):
        print("QPS: {QPS}\nCycles per query: {cycles_per_query}".format(
            QPS=self.QPS, cycles_per_query=self.cycles_per_query))

class Resource_Performance(Resource, Performance):
    """ Resource & Performance info """
    def __init__(self):
        pass

    def copy_from_Performance_Resource(self, Resource_Performance_obj):
        self.copy_from_Resource(Resource_Performance_obj)
        self.cycles_per_query = Resource_Performance_obj.cycles_per_query
        self.QPS = Resource_Performance_obj.QPS

    
class Resource_Performance_Stage1(Resource_Performance):
    """ OPQ """
    def __init__(self):
        self.OPQ_ENABLE = None
        # Factor = 4 or 8, the larger the better performance
        # only used when OPQ_ENABLE=True
        self.OPQ_UNROLL_FACTOR = None 

class Resource_Performance_Stage2(Resource_Performance):
    """ Cluster (vector quantizer) distance computation """
    def __init__(self):
        # except last PE: compute more distances per query, last one compute less
        # e.g., nlist = 8192, PE num = 15, 
        #   each of the first 14 PEs construct 547 tables (8192 / 15 round up), 
        #   while the last constructs 534: 14 * 547 + 534 = 8192
        self.STAGE2_ON_CHIP = None
        self.STAGE2_OFF_CHIP_START_CHANNEL = None  # Only used when STAGE2_ON_CHIP = True
        self.PE_NUM_CENTER_DIST_COMP = None 

class Resource_Performance_Stage3(Resource_Performance):
    """ Select Voronoi cells to scan """
    def __init__(self):
        self.STAGE_3_PRIORITY_QUEUE_LEVEL = None # support 1 or 2
        self.STAGE_3_PRIORITY_QUEUE_L1_NUM = None # only used when STAGE_3_PRIORITY_QUEUE_LEVEL=2

class Resource_Performance_Stage4(Resource_Performance):
    """ Construct LUT """
    def __init__(self):
        # except last PE: construct more tables per query, last one construct less
        # e.g., nprobe = 17, PE num = 6, each of the first 5 PEs construct 3 tables, 
        #   while the last constructs 2: 5 * 3 + 2 = 17
        self.PE_NUM_TABLE_CONSTRUCTION = None 

class Resource_Performance_Stage5(Resource_Performance):
    """ Load PQ code and estimate distance by LUT """
    def __init__(self):
        # (HBM_CHANNEL_NUM * 3 / STAGE5_COMP_PE_NUM) must be integar
        # e.g., default 1 HBM channel -> 3 PQ code streams -> STAGE5_COMP_PE_NUM = 3 * HBM_CHANNEL_NUM
        # e.g., merge content of 1 HBM channel to 1 PQ code stream -> STAGE5_COMP_PE_NUM = HBM_CHANNEL_NUM
        # e.g., merge content of 2 HBM channels to 1 PQ code stream -> STAGE5_COMP_PE_NUM = HBM_CHANNEL_NUM / 
        self.HBM_CHANNEL_NUM = None # PQ code stream num = 3 * HBM_CHANNEL_NUM
        self.STAGE5_COMP_PE_NUM = None 

class Resource_Performance_Stage6(Resource_Performance):
    """ Select the topK results """
    def __init__(self):
        # there could be a sorting network before the priority queue group (SORT_GROUP_ENABLE)
        #   if not, set SORT_GROUP_ENABLE to False, and SORT_GROUP_NUM to 0 or None
        # number of 16 outputs per cycle, e.g., HBM channel num = 10, comp PE num = 30, then 
        #   SORT_GROUP_NUM = 2; if HBM channel = 12, PE_num = 36, then SORT_GROUP_NUM = 3
        self.SORT_GROUP_ENABLE = None
        self.SORT_GROUP_NUM = None 
        self.STAGE_6_PRIORITY_QUEUE_LEVEL = None 
        
        # only fill STAGE_6_PRIORITY_QUEUE_L2_NUM, STAGE_6_STREAM_PER_L2_QUEUE_LARGER, STAGE_6_STREAM_PER_L2_QUEUE_SMALLER
        #   when STAGE_6_PRIORITY_QUEUE_LEVEL = 3, else left them blank
        # Must subject to: L1 stream num = (without sort-reduction unit) STAGE5_COMP_PE_NUM * 2 
        #                  or = (with sort reduction) 16 * 2
        # (STAGE_6_PRIORITY_QUEUE_L2_NUM - 1) * STAGE_6_STREAM_PER_L2_QUEUE_LARGER + STAGE_6_STREAM_PER_L2_QUEUE_SMALLER
        self.STAGE_6_PRIORITY_QUEUE_L2_NUM = None # only used when when STAGE_6_PRIORITY_QUEUE_LEVEL = 3
        self.STAGE_6_STREAM_PER_L2_QUEUE_LARGER = None # only used when when STAGE_6_PRIORITY_QUEUE_LEVEL = 3
        self.STAGE_6_STREAM_PER_L2_QUEUE_SMALLER = None # only used when when STAGE_6_PRIORITY_QUEUE_LEVEL = 3