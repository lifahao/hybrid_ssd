#include "global.h"


int chip_3d::read1(position PBNX, int LPN, Info_Page &val)
{
    int OFFSET = LPN % page_per_block;

    if(info_block[PBNX.layer][PBNX.x][PBNX.y].ip[OFFSET].status == 1 )
    {
        change_temperature('r',PBNX);
        val = info_block[PBNX.layer][PBNX.x][PBNX.y].ip[OFFSET];
        return 1;
    }
    return 0;
}

int chip_3d::read2(position PBNY, int LPN, Info_Page &val)
{
    // 一定是读最新的那一个，所以是从后往前的第一个
    for(int i=page_per_block-1; i>=0 ; i--)
    {
        if(info_block[PBNY.layer][PBNY.x][PBNY.y].ip[i].LPN == LPN )
        {
            val = info_block[PBNY.layer][PBNY.x][PBNY.y].ip[i];
            change_temperature('r',PBNY);
            return 1;
        }
    }
    return 0;
}

int chip_3d::Read3dFlash(int LPN_in_chip, Info_Page &val)
{

	int LBN = LPN_in_chip / page_per_block;
	int OFFSET = LPN_in_chip % page_per_block;

	//先看是否是热数据，在2D上
	auto result = find(hotdata_to_2d.cbegin(), hotdata_to_2d.cend(), LBN);
	if (result != hotdata_to_2d.cend()) {
		//再计算芯片号
		int LCN = val.LPN % chip_num;
		chip2d->read_hotdata_2dflash(val, LPN_in_chip, LCN);
		//返回2意味着写入2D,不往下弄了
		return 2;
	}

	if (address_mapping_table.find(LBN) != address_mapping_table.end())
	{
		//有对应的物理块
		mapping_block_list mbl = address_mapping_table[LBN];
		//正常映射链表，可能为空，空的时候需要找基本块
		list<position> *mappingPBN = &mbl.mapping_pair.first;
		//延迟擦除的旧替换块,可能为空
		list<pos_delay> delay_rpl_block = mbl.mapping_pair.second;
		//实际映射的物理块数目,最大为3，利用position数组来保存位置

		//先找正常块
		int mappingblocknum = mappingPBN->size();
		//int delayblocknum = delay_rpl_block.size();
		vector<position> pbn;
		//如果有映射表有块
		int i = 0;
		if (mappingblocknum > 0) {
			for (list<position>::iterator l_p = mappingPBN->begin();
				l_p != mappingPBN->end(); l_p++)
				//只复制到pbn[i]
				pbn.push_back(*l_p);
		}
		//读基本块
		if (read1(pbn[0], LPN_in_chip, val))
			return 1;
		//读替换块1，2
		if (mappingblocknum > 1) {
			if (read2(pbn[1], LPN_in_chip, val) == 1)
				return 1;
			if (mappingblocknum > 2)
				if (read2(pbn[1], LPN_in_chip, val) == 1)
					return 1;
		}

		//如果都没有，再找延迟擦除块
		int delay_num = delay_rpl_block.size();
		list<pos_delay>::iterator iter_list = delay_rpl_block.begin();
		if (delay_num == 1)
			if (read1(iter_list->p.first, LPN_in_chip, val))
				return 1;
		if (delay_num == 2) {
			read1(iter_list->p.first, LPN_in_chip, val);
			iter_list++;
			read2(iter_list->p.first, LPN_in_chip, val);
			return 1;
		}

	}
	return 0;
}
	


