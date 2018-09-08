#include "global.h"
#define _CRT_SECURE_NO_WARNINGS



void output_overall(chip_3d* chips)
{
	for (int cn = 0; cn < chip_num; cn++) {
		//输出单个芯片p
		chip_3d *p = chips + cn;
		int i, j, k;
		double avg_lay, max_lay, min_lay;
		position maxn, minn;

		char fname[100],
			num[10];//文件名后缀，不同芯片数据放在不同文件
		strcpy(fname, prefix);
		strcat(fname, outfile);
		_itoa(cn, num, 10);
		strcat(fname, num);
		strcat(fname, ".txt");
		freopen(fname, "w", stdout);

		printf("Total_Time: %.3lf\n", total_Time);

		int tot_erase_cnt = 0;
		for (i = 0; i < layer_num; i++) {
			for (j = 0; j < row_num; j++) {
				for (k = 0; k < column_num; k++) {
					tot_erase_cnt += p->info_block[i][j][k].erase_count;
				}
			}
		}
		cout << "Total_eraseTime:\t" << tot_erase_cnt << endl;

		for (i = 0; i < layer_num; i++)
		{
			avg_lay = 0.0;
			max_lay = -1.0;
			min_lay = 9999999.0;
			for (j = 0; j < row_num; j++)
			{
				for (k = 0; k < column_num; k++)
				{
					avg_lay += p->block_temp[i][j][k];

					if (p->block_temp[i][j][k] > max_lay)
					{
						max_lay = p->block_temp[i][j][k];
						maxn.layer = i, maxn.x = j, maxn.y = k;
					}
					if (p->block_temp[i][j][k] < min_lay)
					{
						min_lay = p->block_temp[i][j][k];
						minn.layer = i, minn.x = j, minn.y = k;
					}
				}
			}
			avg_lay /= row_num * column_num;
			printf("layer#%2d max_temp:%3.3lf(%2d,%3d)\t\tmin_temp:%.3lf(%2d,%3d)\t\tavg_temp:%.3lf\n", i, max_lay, maxn.x, maxn.y, min_lay, minn.x, minn.y, avg_lay);
		}

		strcpy(fname, prefix);
		strcat(fname, "peakTemperature");
		strcat(fname, num);
		strcat(fname, ".txt");
		freopen(fname, "w", stdout);

		for (i = 0; i <p->peak_temperature.size(); i++) {
			printf("%3.3lf\n", p->peak_temperature[i]);
		}


		strcpy(fname, prefix);
		strcat(fname, "avgTemperature");
		strcat(fname, num);
		strcat(fname, ".txt");
		freopen(fname, "w", stdout);

		for (i = 0; i <p->peak_temperature.size(); i++) {
			printf("%3.3lf\n", p->average_temperture[i]);
		}

		strcpy(fname, prefix);
		strcat(fname, "final_temperature");
		strcat(fname, num);
		strcat(fname, ".txt");
		freopen(fname, "w", stdout);

		int distribution[130], tmp_val;
		memset(distribution, 0, sizeof(distribution));

		for (i = 0; i < layer_num; i++) {
			for (j = 0; j < row_num; j++) {
				for (k = 0; k < column_num; k++) {
					tmp_val = (int)p->block_temp[i][j][k];
					tmp_val = tmp_val / 5 * 5;
					if (tmp_val >= 120)
						tmp_val = 120;
					distribution[tmp_val] ++;
				}
			}
		}
		for (i = 25; i < 125; i += 5) {
			printf("%d\t%d\n", i, distribution[i]);
		}

		strcpy(fname, prefix);
		strcat(fname, "biterrors");
		strcat(fname, num);
		strcat(fname, ".txt");
		freopen(fname, "w", stdout);

 		for (i = 0; i <p->biterrors.size(); i++) {
			printf("%d\n", p->biterrors[i]);
		}
	}
	return;
}

void chip_3d::scan_avg_peak_temp()
{
	int cn = 0;
	for (; cn < chip_num; cn++) {

		int i, j, k;
		int p_i, p_j, p_k;
		double peak_temp = -1.0;
		double avg_temp = 0.0;
		int erase = 0;
		for (i = 0; i < layer_num; i++)
		{
			for (j = 0; j < row_num; j++)
			{
				for (k = 0; k < column_num; k++)
				{
					if (peak_temp < block_temp[i][j][k]) {
						peak_temp = block_temp[i][j][k];
						erase = info_block[i][j][k].erase_count;
						p_i = i, p_j = j, p_k = k;
					}
					avg_temp += block_temp[i][j][k];
				}
			}
		}
		cout << "  chip_" << cn << "  peak_temp: " << peak_temp << endl;
		avg_temp /= (layer_num * row_num * column_num * 1.0);
		peak_temperature.push_back(peak_temp);
		average_temperture.push_back(avg_temp);

		return;
	}
}







