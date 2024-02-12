#include <cstdio>
#include <vector>
#include <cmath>
#include <mpi.h>
#include <unistd.h>

int compute()
{
  int niter = 4000000;  // number of iterations per FOR loop
  double x, y, z;

  x = 0.0;
  y = 0.0;
  z = 0.0;
  for (int i=0; i<niter; i++)              // main loop
  {
    x = cos(i*0.1)*exp(i*0.04);
    y = sin(i*0.1)*exp(i*0.04);
    z = ((x*x)+(y*y));
  }

  return x + y + z;
}


int main(int argc, char *argv[])
{
    int myid, ntasks, src, dest;
    constexpr int arraysize = 10000000;
    constexpr int msgsize = 10000000;
    std::vector<int> message(arraysize);
    std::vector<int> receiveBuffer(arraysize);
    MPI_Request req[2];
    MPI_Win rma_window;
    int tag;

    int res;

    double t0, tmpi, tmpi1, tmpi2, tcomp;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &ntasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);

    // Initialize message
    for (int i = 0; i < arraysize; i++) {
        message[i] = myid;
    }

    if (myid == 0) {
      src = 1;
      dest = 1;
    } else {
      src = 0;
      dest = 0;
    }

    tmpi = 0.0;
    MPI_Barrier(MPI_COMM_WORLD);
    // No overlap
    for (int r=0; r < 10; r++) 
      {
        t0 = MPI_Wtime();
        MPI_Sendrecv(message.data(), msgsize, MPI_INT, dest, 0, 
                     receiveBuffer.data(), arraysize, MPI_INT, src, 0, 
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE); 
        tmpi += MPI_Wtime() - t0;
        res = compute();
      }

    MPI_Barrier(MPI_COMM_WORLD);
    printf("MPI time with sendrecv,    rank %d :      %f\n", myid, tmpi);

    tmpi1 = 0.0;
    tmpi2 = 0.0;
    tcomp = 0.0;
    // tag = MPI_ANY_TAG;
    tag = 1;
    useconds_t sleep_time = 10000;
    MPI_Barrier(MPI_COMM_WORLD);
    // Overlap
    for (int r=0; r < 10; r++) 
      {
        t0 = MPI_Wtime();
        MPI_Irecv(receiveBuffer.data(), arraysize, MPI_INT, src, tag, MPI_COMM_WORLD, &req[0]);
        //tmpi1 += MPI_Wtime() - t0;
        // res = compute();
        usleep(sleep_time);
        MPI_Isend(message.data(), msgsize, MPI_INT, dest, tag, MPI_COMM_WORLD, &req[1]);
        //t0 = MPI_Wtime();
        tmpi1 += MPI_Wtime() - t0 - sleep_time * 1.0e-6;
        t0 = MPI_Wtime();
        res = compute();
        tcomp += MPI_Wtime() - t0;
        t0 = MPI_Wtime();
        MPI_Waitall(2, req, MPI_STATUSES_IGNORE);
        tmpi2 += MPI_Wtime() - t0;
      }

    MPI_Barrier(MPI_COMM_WORLD);
    printf("MPI time with isend/irecv, rank %d : init %f complete %f (compute %f)\n", myid, tmpi1, tmpi2, tcomp);

    tmpi1 = 0.0;
    tmpi2 = 0.0;
    tcomp = 0.0;
    MPI_Win_create(receiveBuffer.data(), arraysize *  sizeof(int), sizeof(int),
                   MPI_INFO_NULL, MPI_COMM_WORLD, &rma_window);
    // tag = MPI_ANY_TAG;
    tag = 2;
    MPI_Barrier(MPI_COMM_WORLD);
    t0 = MPI_Wtime();
    MPI_Win_fence(0, rma_window);
    tmpi1 += MPI_Wtime() - t0;
    // Overlap with one-sided
    for (int r=0; r < 10; r++) 
      {
        t0 = MPI_Wtime();
        MPI_Put(message.data(), msgsize, MPI_INT, dest, 0, arraysize, MPI_INT, rma_window);
        tmpi1 += MPI_Wtime() - t0;
        t0 = MPI_Wtime();
        res = compute();
        tcomp += MPI_Wtime() - t0;
        t0 = MPI_Wtime();
        MPI_Win_fence(0, rma_window);
        tmpi2 += MPI_Wtime() - t0;
      }

    MPI_Barrier(MPI_COMM_WORLD);
    printf("MPI time with put,         rank %d : put  %f complete %f (compute %f)\n", myid, tmpi1, tmpi2, tcomp);

    MPI_Win_free(&rma_window);

    tmpi1 = 0.0;
    tmpi2 = 0.0;
    tcomp = 0.0;
    // tag = MPI_ANY_TAG;
    tag = 3;
    MPI_Recv_init(receiveBuffer.data(), arraysize, MPI_INT, src, tag, MPI_COMM_WORLD, &req[0]);
    MPI_Send_init(message.data(), msgsize, MPI_INT, dest, tag, MPI_COMM_WORLD, &req[1]);
    MPI_Barrier(MPI_COMM_WORLD);
    // Overlap
    for (int r=0; r < 10; r++) 
      {
        t0 = MPI_Wtime();
        MPI_Startall(2, req);
        tmpi1 += MPI_Wtime() - t0;
        t0 = MPI_Wtime();
        res = compute();
        tcomp += MPI_Wtime() - t0;
        t0 = MPI_Wtime();
        MPI_Waitall(2, req, MPI_STATUSES_IGNORE);
        tmpi2 += MPI_Wtime() - t0;
      }

    MPI_Barrier(MPI_COMM_WORLD);
    printf("MPI time with persistent,  rank %d : init %f complete %f (compute %f)\n", myid, tmpi1, tmpi2, tcomp);

    MPI_Finalize();
    return 0;
}

