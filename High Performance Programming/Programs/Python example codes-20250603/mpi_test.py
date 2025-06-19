from mpi4py import MPI

comm = MPI.COMM_WORLD
rank = comm.Get_rank()

s = 0
d = 1
d_tag = 11

if rank == s:
    data = {'a': 7, 'b': 3.14}
    comm.send(data, dest=d, tag=d_tag)
    print(f'Process {rank} sending {data} to {d}')
elif rank == d:
    data = comm.recv(source=s, tag=d_tag)
    print(f'Process {rank} received {data} from {s}')
