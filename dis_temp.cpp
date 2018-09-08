#include "global.h"

/**
    升温降温模型在这里
*/
// 左右，前后，上下
int dir[6][3] = {0,0,-1, 0,0,1, 0,1,0, 0,-1,0, -1,0,0, 1,0,0};

int chip_3d::check_border(int lay, int x, int y)
{
    if(lay<0 || x<0 || y<0) return 0;
    if(lay>=layer_num || x>=row_num || y>=column_num) return 0;
    return 1;
}

int chip_3d::equ(double val)
{
    if( fabs(val - 25.0) < 0.000001 ){
        return 1;
    }
    return 0;
}

// cl, cx, cy 是目标的坐标
void chip_3d::temp_spread(int cl, int cx, int cy, int ty)
{
    if(check_border(cl, cx, cy) == 0 || equ(block_temp[cl][cx][cy]) )
        return ;

    int lay1, x1, y1;
    int lay2, x2, y2;
    double tris_sum = block_temp[cl][cx][cy];
    double percent_of_dissipation ;

    if(ty == 1)
    {
        lay1= cl + dir[0][0],
        x1  = cx + dir[0][1];
        y1  = cy + dir[0][2];

        lay2= cl + dir[1][0],
        x2  = cx + dir[1][1];
        y2  = cy + dir[1][2];

        percent_of_dissipation = percent_of_zy_dissipation ;
    }
    else if(ty == 2)
    {
        lay1= cl + dir[2][0],
        x1  = cx + dir[2][1];
        y1  = cy + dir[2][2];

        lay2= cl + dir[3][0],
        x2  = cx + dir[3][1];
        y2  = cy + dir[3][2];

        percent_of_dissipation = percent_of_qh_dissipation ;
    }
    else if(ty == 3)
    {
        lay1= cl + dir[4][0],
        x1  = cx + dir[4][1];
        y1  = cy + dir[4][2];

        lay2= cl + dir[5][0],
        x2  = cx + dir[5][1];
        y2  = cy + dir[5][2];

        percent_of_dissipation = percent_of_sx_dissipation ;
    }

//    计算三个位置的总和
    if(check_border(lay1,x1,y1))
        tris_sum += block_temp[lay1][x1][y1];
    else
        tris_sum += 25.0;

    if(check_border(lay2,x2,y2))
        tris_sum += block_temp[lay2][x2][y2];
    else
        tris_sum += 25.0;

//    计算中心块散热 / 吸热的量，正数放热，负数吸热
    double dis_temp = (block_temp[cl][cx][cy] - tris_sum/3.0) * percent_of_dissipation;//********
    double ori_temp = block_temp[cl][cx][cy];

	block_temp[cl][cx][cy] -=  dis_temp ;

    if(equ(block_temp[cl][cx][cy]) || block_temp[cl][cx][cy] < 25.0){
        dis_temp = dis_temp - (25.0 - block_temp[cl][cx][cy]);
		block_temp[cl][cx][cy] = 25.0;
    }

    double potential_percent1 = 0;
    double potential_percent2 = 0;
    double potential_sum = 0;

//    计算势差
    if(check_border(lay1,x1,y1))
        potential_percent1 = fabs(block_temp[lay1][x1][y1] - ori_temp );  //********
    else
        potential_percent1 = fabs(25.0 - block_temp[cl][cx][cy]);

    if(check_border(lay2,x2,y2))
        potential_percent2 = fabs(block_temp[lay2][x2][y2] - ori_temp );  ///**********
    else
        potential_percent2 = fabs(25.0 - block_temp[cl][cx][cy]);

    potential_sum = potential_percent1 + potential_percent2 ;

    if(potential_sum < 0.000000001){
        if(check_border(lay1,x1,y1))
			block_temp[lay1][x1][y1] += 0;

        if(check_border(lay2,x2,y2))
			block_temp[lay2][x2][y2] += 0;
    }
    else{
        if(check_border(lay1,x1,y1))
			block_temp[lay1][x1][y1] += dis_temp * (potential_percent1 / potential_sum);

        if(check_border(lay2,x2,y2))
			block_temp[lay2][x2][y2] += dis_temp * (potential_percent2 / potential_sum);
    }

    if( check_border(lay1, x1, y1) ){
        if( equ(block_temp[lay1][x1][y1]) || block_temp[lay1][x1][y1] < 25.0)
			block_temp[lay1][x1][y1] = 25.0;
    }
    if( check_border(lay2, x2, y2) ){
        if( equ(block_temp[lay2][x2][y2]) || block_temp[lay2][x2][y2] < 25.0 )
			block_temp[lay2][x2][y2] = 25.0;
    }

    return ;
}


void chip_3d::dissipation_temp()
{
	for (int cn = 0; cn < chip_num; cn++) {

		int i, j, k;
		int layer_count, row_count, column_count;

		// 左右
		layer_count = row_count = column_count = 0;

		for (i = 0; i < layer_num; i++) {
			for (j = 0; j < row_num; j++) {
				for (k = column_num / 2; ; k += column_count * _pow[column_count - 1])
				{
					temp_spread( i, j, k, 1);
					column_count++;
					if (k == 0)
					{ 
						column_count = 0;
						break;
					}
				}
			}
		}

		layer_count = row_count = column_count = 0;
		for (i = 0; i < layer_num; i++)
		{
			for (j = 0; j < column_num; j++)
			{
				for (k = row_num / 2; ; k += row_count * _pow[row_count - 1])
				{
					temp_spread( i, k, j, 2);
					row_count++;
					if (k == 0)
					{
						row_count = 0;
						break;
					}
				}
			}
		}


		layer_count = row_count = column_count = 0;

		for (i = 0; i < row_num; i++) {
			for (j = 0; j < column_num; j++) {
				for (k = layer_num / 2; ; k += layer_count * _pow[layer_count - 1]) {
					temp_spread(k, i, j, 3);
					layer_count++;
					if (k == 0)
					{
						layer_count = 0;
						break;
					}
				}
			}
		}

	}

    return ;
}



/**
严格来说是涉及温度的因素都在这里进行监测，
并且调用相关函数处理
*/
void chip_3d::change_temperature(char op, position aim)
{
	double val_temp, val_time;

	if (op == 'r')
		val_time = read_time;
	else if (op == 'w')
		val_time = write_time;
	else if (op == 'e')
		val_time = erase_time;
	else
		val_time = 0.0;

	now_dissipation_temp_time += val_time;
	now_correct_time += val_time;
	now_make_errors_time += val_time;
	now_scan_time += val_time;
	total_Time += val_time;

	if (now_make_errors_time >= make_errors_time)
	{
		now_make_errors_time -= make_errors_time;
		for (int i = 0; i < chip_num; i++) {
			chips3d[i]->make_errors();
		}

	}

	if (now_correct_time >= correct_time)
	{
		for (int i = 0; i < chip_num; i++) {
			chips3d[i]->scan_and_correct();
		}

	}

	if (now_dissipation_temp_time >= dissipation_temp_time)
	{
		now_dissipation_temp_time -= dissipation_temp_time;
		//要对所有芯片散热
		for (int i = 0; i < chip_num; i++) {
			chips3d[i]->dissipation_temp();
		}

	}

	/*if (now_scan_time >= scan_temp_time)
	{
	now_scan_time -= scan_temp_time;
	scan_temp();
	}*/

	if (aim.layer == -1)
		return;

	if (op == 'r') val_temp = read_tmp;
	else if (op == 'w') val_temp = write_tmp;
	else val_temp = erase_tmp;

	block_temp[aim.layer][aim.x][aim.y] += val_temp;

	return;
}
//double lastpeak[530000];
//map<int, int> mapping_error;
