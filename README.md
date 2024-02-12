# Overlapping communication and communication

Simple test code for investigation how communication and computation can be overlapped in
practice.

Generally, there are two mechanisms inside the MPI library to allow communication to proceed concurrently with computations:
- progress thread within the library
- offloadng part of the communication to network interface (NIC)

Progress thread is typically useful only if there are unused CPU cores, otherwise the progress thread runs on same CPU core that is doing computations which slows down the computations.

OpenMPI does not support progress threads. Intel MPI supports progress threads (https://www.intel.com/content/www/us/en/docs/mpi-library/developer-guide-linux/2021-6/asynchronous-progress-control.html). Cray MPI in LUMI (and MPICH in general) supports progress threads via `MPICH_ASYNC_PROGRESS` environment variable (see `man mpi` in LUMI).

Mellanox Infiniband NIC in Puhti and Mahti supports offload of "expected" messages when using the rendezvous protocol, i.e. for large messages. "Expected" means that irecv's need to be posted before the corresponding isend's, which is not always easy to achieve. In some cases NIC offload has caused message corruption, and it is thus not enabled by default. NIC offload is controlled by UCX (the lower level library below MPI) and can be enabled with the environment variable `export UCX_RC_MLX5_TM_ENABLE=y`. As might be expected, NIC offload does not work for intranode communication which uses shared memory.

The Cassini NIC in LUMI has more extensive offload capabilities, as also eager protocol (short messages) and "unexpected" messages can be offloaded.

Finally, hardware offload is typically possible only for communicating contiguous buffers, i.e. no MPI derived types.

Application developer can naturally use e.g. OpenMP threads for achieving overlap of communicating and computation. Also, when using GPUs it is relatively straightforward to have CPU progress communication while GPU is computing.

Some links that might be useful:
https://enterprise-support.nvidia.com/s/article/understanding-tag-matching-for-developers
https://www.youtube.com/watch?v=SzT6LFV3ggU (especially from 19:00 on)

## Examples in LUMI

Default settings (NIC offload)
```
srun -p debug --nodes=2 --ntasks-per-node=1 --hint=multithread ...
srun: job 3661200 queued and waiting for resources
srun: job 3661200 has been allocated resources
MPI time with sendrecv,    rank 0 :      0.023651
MPI time with isend/irecv, rank 0 : init 0.000632 complete 0.000057 (compute 1.618593)
MPI time with put,         rank 0 : put  0.000057 complete 0.006110 (compute 1.605775)
MPI time with persistent,  rank 0 : init 0.000050 complete 0.000019 (compute 1.607345)
MPI time with sendrecv,    rank 1 :      0.024266
MPI time with isend/irecv, rank 1 : init 0.000683 complete 0.000066 (compute 1.618397)
MPI time with put,         rank 1 : put  0.000066 complete 0.000112 (compute 1.611762)
MPI time with persistent,  rank 1 : init 0.000048 complete 0.000031 (compute 1.611544)
```

Disabling NIC offload and enabling progress thread:
```
MPICH_ASYNC_PROGRESS=1 FI_CXI_RX_MATCH_MODE=software srun -p debug --nodes=2 --ntasks-per-node=1 --cpus-per-task=2 --hint=multithread ...
srun: job 3661378 queued and waiting for resources
srun: job 3661378 has been allocated resources
MPI time with sendrecv,    rank 0 :      0.034875
MPI time with isend/irecv, rank 0 : init 0.000639 complete 0.000040 (compute 2.320915)
MPI time with put,         rank 0 : put  0.000088 complete 0.000122 (compute 2.320953)
MPI time with persistent,  rank 0 : init 0.000087 complete 0.000007 (compute 2.308032)
MPI time with sendrecv,    rank 1 :      0.034453
MPI time with isend/irecv, rank 1 : init 0.000675 complete 0.000054 (compute 2.320297)
MPI time with put,         rank 1 : put  0.000075 complete 0.013851 (compute 2.307240)
MPI time with persistent,  rank 1 : init 0.000081 complete 0.000018 (compute 2.309168)
```


