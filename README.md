# Auto-ANNS-Accelerator

## Functionalities to support

* support U250
* support no OPQ option (stage 1)
* support IVF index off-chip, e.g., for 262,144 centers (stage 2)
* support more topK option (stage 6)
  * topK=1, topK=100

* Visualization
  * 4 dimensions visualization for computation intensity
    * nlist (x), 2^n
    * nprobe (y), 2^n
    * recall (z)
    * color (intensity)
    * problem 1: the length of y axix, nprobe can be up to 1024 for nlist=1024, but for nlist=262,144 this is just a small number
      * solution: only allow up to, e.g., 10% scan ratio?, it is fine that some part of the figure can be empty

* Performance model
  * add IVF index off-chip, e.g., for 262,144 centers (stage 2)
  * add more topK option (stage 6)
  * add generated YAML file