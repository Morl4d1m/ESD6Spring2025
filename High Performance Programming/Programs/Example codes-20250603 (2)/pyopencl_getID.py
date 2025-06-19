import numpy as np
import pyopencl as cl

n_elements = 20
result_host = np.zeros(n_elements).astype(np.float32)

#print(cl.get_platforms())



context = cl.create_some_context()
print(dir(context))
input('')
queue = cl.CommandQueue(context)

mf = cl.mem_flags		
result_g = cl.Buffer(context, mf.WRITE_ONLY, result_host.nbytes) 

prg = cl.Program(context, """
__kernel void only_get_id(
    __global float* output)
{
   int i = get_global_id(0);
   int j = get_global_id(1);
   output[i] = i;
}
""").build()


knl = prg.only_get_id # Use this Kernel object for repeated calls
knl(queue, result_host.shape, None, result_g)

cl.enqueue_copy(queue, result_host, result_g)

print(result_host)



