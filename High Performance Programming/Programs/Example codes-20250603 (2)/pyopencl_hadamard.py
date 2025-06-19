import numpy as np
import pyopencl as cl
import time 

def hadamard(a,b):
	length = len(a)
	c = np.zeros(a.shape)
	for i in range(length):
		c[i] = a[i]*b[i]
	return c

PROBLEM_SIZE = int(1e7)

a_host = np.random.rand(PROBLEM_SIZE).astype(np.float32)
b_host = np.random.rand(PROBLEM_SIZE).astype(np.float32)
result_host = np.empty_like(a_host)


ctx = cl.create_some_context()
queue = cl.CommandQueue(ctx)

mf = cl.mem_flags
a_g = cl.Buffer(ctx, mf.READ_ONLY | mf.COPY_HOST_PTR, hostbuf=a_host)		
b_g = cl.Buffer(ctx, mf.READ_ONLY | mf.COPY_HOST_PTR, hostbuf=b_host)

kernel_source = open("hadamard_kernel.cl").read()

prg = cl.Program(ctx, kernel_source).build()

result_g = cl.Buffer(ctx, mf.WRITE_ONLY, a_host.nbytes)
knl = prg.hadamard  # Use this Kernel object for repeated calls

t0 = time.time()
knl(queue, a_host.shape, None, a_g, b_g, result_g)

cl.enqueue_copy(queue, result_host, result_g)
exec_time_OCL = time.time() - t0

t0 = time.time()
expected_result = a_host * b_host
exec_time_numpy = time.time() - t0

t0 = time.time()
c = hadamard(a_host,b_host)
exec_time_loop = time.time() - t0

# Check on CPU with Numpy:
print(f"Error = {np.linalg.norm(result_host - expected_result)}")
if np.allclose(result_host, expected_result):
    print(f'All the results are correct\nElapsed time in milliseconds:')
    print('    PyOpenCL: {:0.3f} '.format(exec_time_OCL*1e3))
    print('    Numpy: {:0.3f} '.format(exec_time_numpy*1e3))
    print('    For loop: {:0.3f} '.format(exec_time_loop*1e3))
    

