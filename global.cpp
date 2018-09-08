#include "global.h"

chip_3d::chip_3d() {
	//把4个链表清空
	address_mapping_table.clear();
	hotdata_to_2d.clear();
	delay_copy_block.clear();
	reused_primary_block.clear();
	int i, j, k;
	for (i = 0; i<layer_num; i++)
	{
		for (j = 0; j<row_num; j++)
		{
			for (k = 0; k<column_num; k++)
			{
				block_temp[i][j][k] = 25.0;
			}
		}
	}
	GC_count = 0;
	GC_count_gap = 0;
	d_erase_count = 0;
	free_block_num = layer_num * row_num * column_num;
	build_mols(mols);
	//初始化延迟链表指针
	d_erase_p = delay_copy_block.begin();
	
}

char infile[100];
char outfile[100];
char prefix[100];

double now_dissipation_temp_time;
double now_make_errors_time;
double now_correct_time;
double now_scan_time;

double block_temp[layer_num][row_num][column_num];

list<cache_cell> wr_cache;

//////////////////////////////降温相关参数
int _pow[1000];

//////////////////////////////纠错相关参数
double temp_mapping_temp[26] ;
int block_err[layer_num][row_num][column_num];

int scan_temperature_gap ;
int cnt_temperature_gap;
int scan_count;
double total_Time;

//指向芯片的指针为全局变量
chip_2d *chip2d;
chip_3d* chips3d[chip_num];
