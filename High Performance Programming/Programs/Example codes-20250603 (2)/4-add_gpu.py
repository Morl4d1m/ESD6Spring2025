from numba import vectorize, jit, cuda
import numpy as np
import time
import torch

def torch_fill_vector(N,K,n_vals):
    x = torch.randint(n_vals, size = (N,K))     # Filling vector
    return x

def np_fill_vector(N,K,n_vals):
    x = np.float32(np.random.randint(n_vals, size = (N,K)))     # Filling vector
    return x
    
def timer(f):                                   # Decorator to calculate execution time of a funciton
    def wrapper(*args, **kw):                   # Needed to decorate a function with input arguments
        t_start = time.time()
        result = f(*args, **kw)                 # Calling function
        t_end = time.time()
        return result, t_end-t_start            # Return the result AND the execution time
    return wrapper

@timer
def loop(x,c, N, K):
    y = np.zeros((N,K))
    for n in range(N):
        for k in range(K):
            y[n,k] = x[n,k]+c
    return y

@timer       
def np_add(x,c):
    y = x+c
    return y
    
@timer
@vectorize(['float32(float32, float32)'], target='cpu')
def vectorized_add(a,c):
    return a+c

@timer        
@vectorize(['float32(float32, float32)'], target='cuda')
def gpu_add(a,c):
    return a+c
   
    
if __name__=="__main__":  

    if torch.cuda.is_available():
        device = torch.device("cpu")
    else:
        device = torch.device("cpu")
    np.random.seed(2)   # Setting random seed
    N = 1080          # Size of vector
    K = 1920               # No. columns in matrix
    n_vals = 256         # The range of values is [0,n_vals-1]
    n_exp = 100           # Number of realizations
    exec_times = np.zeros(5)

    c = 4.0
        
    for i in range(n_exp):       
        a = np_fill_vector(N,K,n_vals).astype(np.float32)


        # Calculate the product with naive solution
        y_loops, t_loops = 0,0#loop(a,c, N, K)


        # Calculate the product with numpy

        y_numpy, t_numpy = np_add(a,c)
 
       

        
        # Vectorized version in CPU
        y_vector, t_vector = vectorized_add(a,c)

        
        # Vectorized version in GPU
        y_gpu, t_gpu = gpu_add(a,c)

        #####
        # PyTorch
        torch.cuda.empty_cache()
        torch.cuda.synchronize()

        a = torch_fill_vector(N,K,n_vals)
        A_gpu = a.to(device)
        if torch.cuda.current_device()==0:
            torch.cuda.synchronize()


        t0 = time.time()
        y_gpu = A_gpu+c
        if torch.cuda.current_device()==0:
            torch.cuda.synchronize()
            y_cuda = y_gpu.to('cpu')
        else:
            y_cuda = y_gpu
        t1 = time.time()
        t_torch= t1-t0

        exec_times += [t_loops, t_numpy, t_vector, t_gpu, t_torch]
    exec_times/=n_exp
    print(f'\n%%%%\nFinished execution\n')
    print(f'All the results are correct\nAverage elapsed time in milliseconds:')
    print('    Only loops: {:0.3f} '.format(exec_times[0]*1e3))
    print('    Numpy: {:0.3f} '.format(exec_times[1]*1e3))
    print('    Vectorization in CPU: {:0.3f} '.format(exec_times[2]*1e3))   
    print('    Vectorization and CUDA: {:0.3f} '.format(exec_times[3]*1e3)) 
    print('    PyTorch and CUDA: {:0.3f} '.format(exec_times[4]*1e3))    
