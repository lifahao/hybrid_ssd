#include "global.h"
/*
通过只写入重要数据来验证2d重要数据的读写，验证通过。重要数据比例1/4，在总的写LPN比例大约在26%。
通过只写入非重要数据，来验证单片3D数据的读写。


*/



int checkData(int LPN, Info_Page rd) {
	if (rd.LPN != LPN)
		return 0;
	return 1;
}

int main()
{
	initial();
	if(freopen(infile, "r", stdin)==NULL)
		cout<<"freopen error!"<<endl;

    chip2d = new chip_2d();

    for(int i=0; i < chip_num;i++){
        chips3d[i] = new chip_3d();
    }

    char op;
    int LPN;
    Info_Page rd,init_p(0,0,0,0);
	int records = 0;
	int impdata = 0;
	int r_w = 0, imp_r_w = 0;
    while(scanf("%c	%d\n",&op,&LPN)!=EOF)
	{
		if (op == 'w')r_w++;
		if (LPN >= important_data_limit) {
			//读写的数据就是LPN
			//cout << op << "  " << LPN << endl;
			cnt_temperature_gap++;
			if (op == 'w')
			{
				imp_r_w++;
				write_cache(LPN, LPN);
			}
			else
			{
				rd = init_p;
				read_cache(LPN, rd);
				//assert(checkData(LPN, rd));
				if (rd.LPN != LPN)
					cout << "read error" << endl;
				/*else
					cout << " read right" << endl;*/
			}
			if (cnt_temperature_gap == scan_temperature_gap) {
				cnt_temperature_gap = 0;
				for (int i = 0; i < chip_num; i++)
					chips3d[i]->scan_avg_peak_temp();
			}
			impdata++;
		}
		records++;
		if(r_w % 1000 == 0)cout<<"writed important data: "<<1.0 * imp_r_w / r_w <<endl;
    }
	
	for(int i = 0;i<chip_num;i++)
        output_overall(chips3d[i]);

	fclose(stdin);
    return 0;
}
