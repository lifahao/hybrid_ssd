#include "global.h"


int read_cache(int LPN, Info_Page &val)
{

	list<cache_cell>::iterator it;

	for (it = wr_cache.begin(); it != wr_cache.end(); it++)
	{
		//如果在缓存里
		if ((*it).LPN == LPN)
		{
			val.data = (*it).data;
			val.LPN = LPN;
			val.status = 1;
			val.ecc = ecc_strength;

			// 在cache里面读到也要算时间
			change_time('r');
			return 1;
		}
	}
	//如果不在，从芯片中找
	int ret = 0;
	//重要数据从2D芯片找
	if(LPN < important_data_limit){
		ret = chip2d->read_2dflash(LPN,val);
	}
	else{
		int LCN = LPN % chip_num;
		int LPN_in_chip = LPN / chip_num;
		ret = chips3d[LCN]->Read3dFlash(LPN_in_chip, val);
	}
	
	if (ret == 1) {
		//如果成功找到了，将数据从芯片读到cache
		write_cache(val.LPN, val.data);
	}
	//如果没有读到，证明trace有问题 ret-0，或者流程有问题，ret-2
	return ret;
}
