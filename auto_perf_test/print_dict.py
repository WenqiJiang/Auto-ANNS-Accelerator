# Example Usage:
#   python print_dict.py --dict_dir './FPGA_perf_dict_SIFT100M_K_1.pkl' 
#   python print_dict.py --dict_dir './cpu_recall_index_nprobe_pairs_SIFT100M.pkl' 

import pickle
import argparse 
parser = argparse.ArgumentParser()
parser.add_argument('--dict_dir', type=str, default='./cpu_recall_index_nprobe_pairs_SIFT100M.pkl', help="recall dictionary directory")

args = parser.parse_args()

with open(args.dict_dir, 'rb') as f:
    d = pickle.load(f)
    print("\n\n======= Dictionary Name: {} =======\n".format(args.dict_dir))
    print(d) 
    print("\n\n")
