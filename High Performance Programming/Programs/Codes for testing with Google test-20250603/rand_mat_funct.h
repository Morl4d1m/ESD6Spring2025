const int x = 500;
int A[x][x];
int B[x][x];
int C[x][x];

int matrix_vector_product(int i, int imax)
{
for (i; i<imax;i++)
{
	for (int k=0; k<x;k++)
	{
		for (int j=0; j<x;j++)
		{
			C[i][j]+=A[i][k]*B[k][j];
		//std::cout<<C[i][j]<<" ";
		}
	//std::cout<<std::endl;
	}

}
return 0;
}

int calculate_step(int nThreads)
{
	int step;
	step = x/nThreads;
	std::cout<<"Performing multiplication with "<<nThreads<<" threads with "<<step<<" rows"<< std::endl;
	return step;
}
