from mpi4py import MPI

comm = MPI.COMM_WORLD
rank = comm.Get_rank()
world_size = comm.Get_size()

print(f'Hello from process {rank} out of {world_size}')
