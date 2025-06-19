
__kernel void hadamard(
    __global const float *a_g, __global const float *b_g, __global float *result_g)
{
  int i = get_global_id(0);
  result_g[i]= a_g[i] * b_g[i];
}




