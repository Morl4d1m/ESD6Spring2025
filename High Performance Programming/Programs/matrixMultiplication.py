import numpy as np
import time
import concurrent.futures
import multiprocessing as mp
import cupy as cp

def numpy_multiplication(A, B):
    return np.dot(A, B)

def multi_threading_multiplication(A, B, C, n):
    def multiply_row(i):
        C[i, :] = np.dot(A[i, :], B)
    with concurrent.futures.ThreadPoolExecutor() as executor:
        executor.map(multiply_row, range(n))

def multi_processing_multiplication(A, B, n):
    num_procs = mp.cpu_count()
    chunk_size = n // num_procs
    processes = []
    manager = mp.Manager()
    output = manager.Array('d', n * n)
    def worker(start, end, A, B, output):
        output[start:end] = np.dot(A[start:end, :], B).flatten()
    for i in range(num_procs):
        start_idx = i * chunk_size
        end_idx = (i + 1) * chunk_size if i != num_procs - 1 else n
        p = mp.Process(target=worker, args=(start_idx, end_idx, A, B, output))
        processes.append(p)
        p.start()
    for p in processes:
        p.join()
    return np.array(output).reshape(n, n)

def gpu_multiplication(A, B):
    A_gpu = cp.asarray(A)
    B_gpu = cp.asarray(B)
    C_gpu = cp.dot(A_gpu, B_gpu)
    return cp.asnumpy(C_gpu)

def tiled_multiplication(A, B, n, block_size):
    C = np.zeros((n, n))
    def compute_tile(i, j):
        C[i:i+block_size, j:j+block_size] = np.dot(A[i:i+block_size, :], B[:, j:j+block_size])
    with concurrent.futures.ThreadPoolExecutor() as executor:
        for i in range(0, n, block_size):
            for j in range(0, n, block_size):
                executor.submit(compute_tile, i, j)
    return C

if __name__ == "__main__":
    n = 10000
    block_size = 1000
    A = np.random.rand(n, n)
    B = np.random.rand(n, n)
    
    methods = {
        "NumPy": numpy_multiplication,
        "Multi-threading": lambda A, B: multi_threading_multiplication(A, B, np.zeros((n, n)), n),
        "Multi-processing": multi_processing_multiplication,
        "GPU": gpu_multiplication,
        "Tiled Multiplication": lambda A, B: tiled_multiplication(A, B, n, block_size)
    }
    
    for method_name, method in methods.items():
        start = time.time()
        C = method(A, B)
        elapsed = time.time() - start
        print(f"{method_name} Execution Time: {elapsed:.2f} seconds")
