import torch
import time
import numpy as np

if torch.cuda.is_available():
    device = torch.device("cuda")
else:
    device = torch.device("cpu")

print(f'Using {device}')    

def fill_torch(N,K,M,n_exp):
    x = torch.randint(M, size = (N,1)).double()     # Filling vector
    A = torch.randint(M, size=(N,K)).double()       # Filling matrix
    if n_exp ==1: 
        print(f'Vector\n {x}')
        print(f'Matrix\n {A}')
    return x, A

def fill_np(N,K,M,n_exp):
    x = np.float64(np.random.randint(M, size = (N,1)))     # Filling vector
    A = np.float64(np.random.randint(M, size=(N,K)))       # Filling matrix
    if n_exp ==1: 
        print(f'Vector\n {x}')
        print(f'Matrix\n {A}')
    return x, A  

if __name__=="__main__":   
    torch.manual_seed(2)   # Setting random seed
    N = 1080          # Size of vector
    K = 1920               # No. columns in matrix
    n_vals = 256         # The range of values is [0,n_vals-1]
    n_exp = 100           # Number of realizations
    exec_times = torch.zeros(4)
    
    for i in range(n_exp):       
        x, A = fill_torch(N,K,n_vals, n_exp)

        # Calculate the product with naive solution in CPU
        t0 = time.time()
        y_cpu = torch.mul(x,A)
        t1 = time.time()
        t_cpu= t1-t0
        
        torch.cuda.empty_cache()
        torch.cuda.synchronize()
        #print(torch.cuda.memory_summary(device=None, abbreviated=False))

        # Calculate the time to copy the arrays to GPU
        t0 = time.time()
        x_gpu = x.to(device)
        A_gpu = A.to(device)
        if torch.cuda.current_device()==0:
            torch.cuda.synchronize()
        t1 = time.time()
        t_copy_gpu= t1-t0    
        #print(torch.cuda.memory_summary(device=None, abbreviated=False))
        
        

        t0 = time.time()
        y_gpu = torch.mul(x_gpu,A_gpu)
        if torch.cuda.current_device()==0:
            torch.cuda.synchronize()
        else:
            y_cuda = y_gpu
        t1 = time.time()
        t_gpu= t1-t0
        
        t0 = time.time()
        if torch.cuda.current_device()==0:
            y_cuda = y_gpu.to('cpu')
        t1 = time.time()
        t_copy_gpu+= t1-t0 
        
        x, A = fill_np(N,K,n_vals, n_exp)
        t0 = time.time()
        y_numpy = np.multiply(x,A)
        t1 = time.time()
        t_numpy= t1-t0
        #print(y_cpu.get_device())
        #print(y_cuda.get_device())

        exec_times[0] += t_cpu
        exec_times[1] += t_copy_gpu
        exec_times[2] += t_gpu
        exec_times[3] += t_numpy
    exec_times/=n_exp
    print(f'\n%%%%\nFinished execution\n')
    err = torch.sum(torch.sum(torch.pow(y_cpu-y_cuda,2)))

    print('Sum-squared error is {:0.3f}'.format(err))
    if torch.allclose(y_cpu,y_cuda):
        print(f'All the results are correct\nAverage elapsed time in milliseconds:')
        print('    PyTorch CPU: {:0.3f} '.format(exec_times[0]*1e3))
        print('    PyTorch copy to and from GPU: {:0.3f} '.format(exec_times[1]*1e3))
        print('    PyTorch GPU: {:0.3f} '.format(exec_times[2]*1e3))
        print('    Numpy: {:0.3f} '.format(exec_times[3]*1e3))

