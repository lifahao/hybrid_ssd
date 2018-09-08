#include "global.h"
#define _CRT_SECURE_NO_WARNINGS

/**
ecc 纠错
*/
void chip_3d::scan_and_correct()
{
	int i, j, k;

	int tot_biterrors = 0;
	for (i = 0; i < layer_num; i++)
	{
		for (j = 0; j < row_num; j++)
		{
			for (k = 0; k < column_num; k++)
			{
				tot_biterrors += block_err[i][j][k];
			}
		}
	}
	biterrors.push_back(tot_biterrors);

	for (i = 0; i < layer_num; i++)
	{
		for (j = 0; j < row_num; j++)
		{
			for (k = 0; k < column_num; k++)
			{
				block_err[i][j][k] -= ecc_strength;
				if (block_err[i][j][k] < 0)
				{
					block_err[i][j][k] = 0;
				}
			}
		}
	}
	double avg = 0;
	for (i = 0; i < layer_num; i++) {
		for (j = 0; j < row_num; j++) {
			for (k = 0; k < column_num; k++) {
				avg += block_err[i][j][k] * 1.0;
			}
		}
	}
	avg /= layer_num * row_num * column_num;

	return;
}

/**
温度主导的比特位翻转
*/

int chip_3d::find_err_num(double temp_val)
{
	int i;

	if (temp_val >= temp_mapping_error[25])
		return temp_mapping_error[25];

	for (i = 0; i<temp_error_length - 1; i++)
	{
		if (temp_val >= temp_mapping_temp[i] && temp_val < temp_mapping_temp[i + 1])
		{
			break;
		}
	}
	return temp_mapping_error[i];
}

void chip_3d::make_errors()
{

	int i, j, k;
	double val;
	for (i = 0; i < layer_num; i++)
	{
		for (j = 0; j < row_num; j++)
		{
			for (k = 0; k < column_num; k++)
			{
				val = block_temp[i][j][k];
				block_err[i][j][k] += find_err_num(val);
			}
		}
	}

	return;
}

/**
扫描物理结构，获取温度快照，以及错误率
*/

//void chip_3d::scan_temp()
//{
//    int i,j,k;
//    char filename[100];
//    char fname[100];
//
//    scan_count ++;
//    return ;
//}
