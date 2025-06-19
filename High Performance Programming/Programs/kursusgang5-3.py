import numpy as np
import time
from numba import prange, jit, vectorize, guvectorize

def fill(N, K, n_exp):
    x = np.random.randn(1, N).astype(np.float32)  # Filling vector with normal distribution
    A = np.random.randn(N, N).astype(np.float32)  # Filling matrix with normal distribution
    if n_exp == 1:
        print(f'Vector\n {x}')
        print(f'Matrix\n {A}')
    return x, A

def timer(f):  # Decorator to calculate execution time of a function
    def wrapper(*args, **kw):  # Needed to decorate a function with input arguments
        t_start = time.time()
        result = f(*args, **kw)  # Calling function
        t_end = time.time()
        return result, t_end - t_start  # Return the result AND the execution time
    return wrapper


@timer       
def numpy_mult(x, A):
    y = np.matmul(x, A)
    return y

@timer
@jit(nopython=True, parallel=False)
def numba_jit_mult(x, A):
    H, K = A.shape
    y = np.zeros((1, K), dtype=np.float32)  # Shape change (1, K)
    for n in range(H):
        for k in range(K):
            y[0, k] += x[0, n] * A[n, k]
    return y

@vectorize(['float32(float32, float32)'], target='cpu')
def vectorized_hadamard(a, b):
    return a * b

@guvectorize(['void(float32[:], float32[:,:], float32[:])'], '(n),(n,m)->(m)', target='parallel')
def guvectorized_matmul(x, A, y):
    for i in range(A.shape[1]):
        y[i] = 0  # Ensure the output starts at zero
        for j in range(A.shape[0]):
            y[i] += x[j] * A[j, i]

if __name__ == "__main__":   
    np.random.seed(2)  # Setting random seed
    N = 10000  # Size of vector
    K = N  # No. columns in matrix
    n_exp = 50  # Number of realizations
    exec_times = np.zeros(4)
    
    for i in range(n_exp):       
        x, A = fill(N, K, n_exp)
        
        # Calculate the product with numpy
        y_numpy, t_numpy = numpy_mult(x, A)
        
        # Calculate the product with JIT
        y_jit, t_jit = numba_jit_mult(x, A) 
        y_jit = y_jit.reshape(1, N)  # Ensure shape (1, N)
        
        # Corrected vectorized approach
        t_start = time.time()
        Y_vector = vectorized_hadamard(x, A)  # Element-wise multiplication
        y_vector = np.sum(Y_vector, axis=1, keepdims=True)  # Correct summation
        t_end = time.time()
        t_vector = t_end - t_start
        
        # Calculating the product with gufuncs
        t_start = time.time()
        y_gufunc = guvectorized_matmul(x, A).reshape(1, N)  # Ensure shape (1, N)
        t_end = time.time()
        t_gufunc = t_end - t_start
        
        exec_times += [t_numpy, t_jit, t_vector, t_gufunc]
    exec_times /= n_exp

    print(f'\n%%%%\nFinished execution\n')
    if np.allclose(y_numpy, y_jit) and np.allclose(y_numpy, y_vector) and np.allclose(y_numpy, y_gufunc):
        print(f'All the results are correct\nAverage elapsed time in milliseconds:')
        print('    Numpy: {:0.3e} ms'.format(exec_times[0] * 1e3))
        print('    JIT: {:0.3e} ms'.format(exec_times[1] * 1e3))
        print('    Vectorization: {:0.3e} ms'.format(exec_times[2] * 1e3))
        print('    GUFunc: {:0.3e} ms'.format(exec_times[3] * 1e3))
    else:
        print(f'Results are incorrect!!!')

print(f"Shapes: y_numpy={y_numpy.shape}, y_jit={y_jit.shape}, y_vector={y_vector.shape}, y_gufunc={y_gufunc.shape}")
print(f"Max diff: Numpy-JIT={np.max(np.abs(y_numpy - y_jit))}, Numpy-Vector={np.max(np.abs(y_numpy - y_vector))}, Numpy-GUFunc={np.max(np.abs(y_numpy - y_gufunc))}")
