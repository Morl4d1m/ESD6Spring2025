from mpi4py import MPI
import numpy as np

comm = MPI.COMM_WORLD
rank = comm.Get_rank()

s = 0
d = 1
d_tag = 11
d_len = 10

if rank == 0:
    data = np.arange(d_len, dtype=np.float64)
    comm.Send(data, dest=d, tag=d_tag)
elif rank == 1:
    data = np.empty(d_len, dtype=np.float64)
    comm.Recv(data, source=s, tag=d_tag)
    print(f'Process {rank} received {data} from {s}')
    
bcast_data = np.random.rand(d_len)

comm.Bcast(bcast_data, root=0)

print(f'Process {rank} has {bcast_data}')
