# Descriptions

## get_best_hardware.py

Given a database, an FPGA device, a resource constraint, and a frequency setting,
    search the parameter space (nlist, nprobe, OPQ_enable) and find the best
    hardware solution.

## get_performance.py

Input a set of hardware settings (config.yaml), predict the performance and resource 
    consumption by the performance  model.

## evaluate_hardware_on_paramater_space.py

Given a fixed hardware setting (fixed PE number, but allow nlist and nprobe to change at runtime), predict the performance on a set of parameter space (e.g., whatever nlist and nprobe that can reach R@10=80%).