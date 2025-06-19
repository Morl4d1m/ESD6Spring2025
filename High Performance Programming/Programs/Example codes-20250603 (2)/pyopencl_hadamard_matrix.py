import numpy as np
import pyopencl as cl
import time 

def hadamard(a,b):
    x,y =b.shape
    c = np.zeros(b.shape)
    for i in range(x):
        for j in range(y):
            c[i,j] = a[i]*b[i,j]
    return c
	
def fill_np(N,K,M,n_exp):
    x = np.random.randint(M, size = (N,1)).astype(np.float32)     # Filling vector
    A = np.random.randint(M, size=(N,K)).astype(np.float32)       # Filling matrix
    if n_exp ==1: 
        print(f'Vector\n {x}')
        print(f'Matrix\n {A}')
    return x, A  
    
    
np.random.seed(4)    

N = 1080          # Size of vector
K = 1920               # No. columns in matrix
n_vals = 256         # The range of values is [0,n_vals-1]
n_exp = 100           # Number of realizations
exec_times = np.zeros(3)
ctx = cl.create_some_context()
queue = cl.CommandQueue(ctx)
mf = cl.mem_flags

compare = False             #Set to True to compare the results with For loop and Numpy
for i in range(n_exp):
    x_host, a_host = fill_np(N,K,n_vals,n_exp)
    result_host = np.empty_like(a_host)

    #print(x_host)
    #print(a_host)
    

    
    x_gpu = cl.Buffer(ctx, mf.READ_ONLY | mf.COPY_HOST_PTR, hostbuf=x_host)		
    a_gpu = cl.Buffer(ctx, mf.READ_ONLY | mf.COPY_HOST_PTR, hostbuf=a_host)

    kernel_source = open("hadamard_matrix_kernel.cl").read()

    prg = cl.Program(ctx, kernel_source).build("-D DIM={}".format(K))

    result_g = cl.Buffer(ctx, mf.WRITE_ONLY, a_host.nbytes)
    knl = prg.matrix_hadamard  # Use this Kernel object for repeated calls

    t0 = time.time()
    knl(queue, (K,N), None, x_gpu, a_gpu, result_g)

    cl.enqueue_copy(queue, result_host, result_g)
    exec_time_OCL = time.time() - t0
    exec_times[0]+=exec_time_OCL
    #print(f'Result GPU {result_host}')
    if compare:
        t0 = time.time()
        expected_result = x_host * a_host
        exec_time_numpy = time.time() - t0

        #print(f'Result Numpy {expected_result}')
        t0 = time.time()
        c = hadamard(x_host,a_host)
        exec_time_loop = time.time() - t0
        exec_times[1]+=exec_time_numpy
        exec_times[2]+=exec_time_loop
        
exec_times/=n_exp
if compare:
    print(f"Error = {np.linalg.norm(result_host - expected_result)}")
    if np.allclose(result_host, expected_result):
        print(f'All the results are correct\nElapsed time in milliseconds:')
        print('    PyOpenCL: {:0.3f} '.format(exec_times[0]*1e3))
        print('    Numpy: {:0.3f} '.format(exec_times[1]*1e3))
        print('    For loop: {:0.3f} '.format(exec_time_loop*1e3))
else:        
    print('Elapsed time with PyOpenCL is {:0.3f} in milliseconds'.format(exec_times[0]*1e3))

