#include "global.h"

int isPrime(int n)
{
	int i;
	for (i = 2; i < n; ++i) {
		if (n%i == 0)
			return 0;
	}
	return 1;
}


int first_prime(int m) {
	while (m != 0) {
		if (isPrime(m)) return m;
		m++;
	}
	return -1;
}

int max(int x, int y, int z) {
	int m = x;
	if (m < y)
		m = y;
	if (m < z)
		m = z;
	return m;
}


void chip_3d::build_mols(Mols &mols)
{
	int i, j, k, n;
	int m = max(layer_num, row_num, column_num);
	n = first_prime(m);
	mols.max_ls_val = n;
	//初始化为0
	mols.ls_p = new ls*[n];
	for (i = 0; i < n; i++) {
		mols.ls_p[i] = new ls[layer_num];
	}

	mols.count = new int[mols.max_ls_val];
	for (int i = 0; i <= mols.max_ls_val; i++) mols.count[i] = 0;


	for (i = 0; i < layer_num; i++) {
		for (j = 0; j < row_num; j++)
			for (k = 0; k < column_num; k++) {
				//求得mols值,(i,j,k),i,j,k都大于1，单元ls值为(i*j+k)% n，不要第一行，也就是把（i，j+1,k)的ls值赋给(i,j,k)
				int val = ((i + 1)*(j + 1) + (k)) % n;
				info_block[i][j][k].ls_val = val;
				//保存指向单个ls值在k层第一列出现的位置
				if (mols.ls_p[val][i].flag == 0) {
					position p(i, j, k);
					mols.ls_p[val][i].pos = p;
					mols.ls_p[val][i].flag = 1;
				}
			}
	}
}

void initial()
{
	strcpy(infile, "newtrace.txt");
	//strcpy(outfile,prefix);
	strcat(outfile, "res_out.txt");
	scan_temperature_gap = 15415;

	//////////////////////////////降温 

	for (int i = 0; i<1000; i++)
	{
		if (i % 2)
			_pow[i] = -1;
		else
			_pow[i] = 1;
	}

	//////////////////////////////纠错
	temp_mapping_temp[0] = 25.0;
	for (int i = 1; i<temp_error_length; i++)
	{
		temp_mapping_temp[i] = temp_mapping_temp[i - 1] + temp_gap;
	}
	return;
}
