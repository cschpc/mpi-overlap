# Overlapping communication and communication

Simple test code for investigation how communication and computation can be overlapped in
practice.

Generally, there are two mechanisms inside the MPI library to allow communication to proceed concurrently with computations:
- progress thread within the library
- offloadng part of the communication to network interface (NIC)

Progress thread is typically useful only if there are unused CPU cores, otherwise the progress thread runs on same CPU core that is doing computations which slows down the computations.

OpenMPI does not support progress threads. Intel MPI (available in Puhti) supports progress threads (https://www.intel.com/content/www/us/en/develop/documentation/mpi-developer-guide-linux/top/additional-supported-features/asynchronous-progress-control.html). Cray MPI in LUMI (and MPICH in general) supports progress threads via `MPICH_ASYNC_PROGRESS` environment variable (see `man mpi` in LUMI).

Mellanox Infiniband NIC in Puhti and Mahti supports offload of "expected" messages when using the rendezvous protocol, i.e. for large messages. "Expected" means that irecv's need to be posted before the corresponding isend's, which is not always easy to achieve. In some cases NIC offload has caused message corruption, and it is thus not enabled by default. NIC offload is controlled by UCX (the lower level library below MPI) and can be enabled with the environment variable `export UCX_RC_MLX5_TM_ENABLE=y`. As might be expected, NIC offload does not work for intranode communication which uses shared memory.

The Cassini NIC in LUMI has more extensive offload capabilities, as also eager protocol (short messages) and "unexpected" messages can be offloaded.

As a final remark, hardware offload is typically possible only for communicating contiguous buffers, i.e. no MPI derived types.

Application developer can naturally use e.g. OpenMP threads for achieving overlap of communicating and computation. Also, when using GPUs it is relatively straightforward to have CPU progress communication while GPU is computing.

Some links that might be useful:
https://enterprise-support.nvidia.com/s/article/understanding-tag-matching-for-developers
https://www.youtube.com/watch?v=SzT6LFV3ggU (especially from 19:00 on)

