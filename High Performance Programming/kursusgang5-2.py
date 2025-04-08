# -*- coding: utf-8 -*-
"""
Created on Thu Mar 13 10:45:48 2025

@author: Christian Lykke
"""

import numpy as np
import time

def fill(N,K,M,n_exp):
    x = np.random.randint(M, size = (1,N))     # Filling vector
    A = np.random.randint(M, size=(N,N))       # Filling matrix
    if n_exp ==1: 
        print(f'Vector\n {x}')
        print(f'Matrix\n {A}')
    return x, A

def timer(f):                                   # Decorator to calculate execution time of a funciton
    def wrapper(*args, **kw):                   # Needed to decorate a function with input arguments
        t_start = time.time()
        result = f(*args, **kw)                 # Calling function
        t_end = time.time()
        return result, t_end-t_start            # Return the result AND the execution time
    return wrapper

@timer                                          # Decorate the function only_loops
def only_loops(x,A):
    N = x.shape[1]
    H, K  = A.shape 
    if N == H:
        y = np.zeros(K)
        for k in range(K):
            for n in range(N):
                y[k] += x[0,n]*A[n,k]
        return y
    else:
        print('Vector and matrix dimensions don\'t match')
        
@timer      
def partial_loops(x,A):
    N = x.shape[1]
    H, K = A.shape
    if N == H:
        y = np.zeros(K)
        for k in range(K):
            y[k] = np.dot(x[0,:],A[:,k])
        return y
    else:
        print('Vector and matrix dimensions don\'t match')
@timer        
def numpy_mult(x,A):
    y = np.matmul(x,A)
    return y
    
        
        
if __name__=="__main__":   
    np.random.seed(2)   # Setting random seed
    N = 100             # Size of vector
    K = N               # No. columns in matrix
    n_vals = 10         # The range of values is [0,n_vals-1]
    n_exp = 10          # Number of realizations
    exec_times = np.zeros(3)
    max_val = 10        # Upper limit for random number generation is max_val-1
    for i in range(n_exp):       
        x, A = fill(N,K, max_val, n_exp)

        # Calculate the product with naive solution
        
        y_loops, t_loops = only_loops(x,A)


        # Calculate the product with vector multiplications
        y_partial, t_partial = partial_loops(x,A)

        # Calculate the product with numpy
        y_numpy, t_numpy = numpy_mult(x,A)

        exec_times += [t_loops, t_partial, t_numpy]
    exec_times/=n_exp
    print(f'\n%%%%\nFinished execution\n')
    if np.allclose(y_loops,y_numpy) and np.allclose(y_partial,y_numpy):
        print(f'All the results are correct\nAverage elapsed time in milliseconds:')
        print('    Only loops: {:0.3e} '.format(exec_times[0]*1e3))
        print('    Partial loops: {:0.3e} '.format(exec_times[1]*1e3))
        print('    Numpy: {:0.3e} '.format(exec_times[2]*1e3))
    else:
        print(f'Results are incorrect!!!')


