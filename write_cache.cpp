#include "global.h"
//LPN 全局LPN val 要写入的值
int write_cache(int LPN, int val)
{
    // 要保留一个LPN关键字
    cache_cell st(LPN,val);
    list<cache_cell>::iterator it;
    int ok = 0;

    // 检查是否在cache
    for(it=wr_cache.begin(); it!=wr_cache.end(); it++)
    {
		//在cache内，更新
        if(*it == st)  
        {
			change_time('w');
            ok = 1;
            wr_cache.erase(it);
            wr_cache.push_back(st);
            break;

        }
    }

	//更新后返回
    if( ok )  return 1;

    // 如果不在，检查是否还有空间
    if( wr_cache.size() == cache_threshold )
    {
        // 到这里说明链表长度已经达到了阈值
        it = wr_cache.begin();
        cache_cell tmp = (*it);
        //先查看是否是重要数据，如果是重要数据，写入2D
        if(LPN < important_data_limit){
            chip2d->write_2dflash(tmp);
        }
        else{
            //将第一个元素写到3D闪存,先计算第一个元素的芯片号
            int LCN = tmp.LPN % chip_num;
            int LPN_in_chip = tmp.LPN / chip_num;

            chips3d[LCN]->Write3dFlash(LPN_in_chip,tmp);
        }
		
        wr_cache.erase(it);
  
    }
   
	//到这一定有空间
    wr_cache.push_back(st);


    return 1;
}
