__kernel void matrix_hadamard(
    __global const float *x_g, __global const float *a_g, __global float *result_g)
{
  int i = get_global_id(0);
  int j = get_global_id(1);
  result_g[i+j*DIM]= x_g[j]*a_g[i+j*DIM];
}
