#include "global.h"
//写到基本块

bool cmp(const pair<int, int> &p1, const pair<int, int> &p2)
{
	//从大到小进行排序
	return p1.second > p2.second;
}


int chip_3d::write1 (position PBN, int LPN, int val)
{
    int layer = PBN.layer;
    int x = PBN.x;
    int y = PBN.y; 

    int OFFSET = LPN % page_per_block ;
    //如果偏移页可用，写到对应的页上
    if( info_block[layer][x][y].ip[OFFSET].status == 0)
    {
		info_block[layer][x][y].ip[OFFSET].data = val;
		info_block[layer][x][y].ip[OFFSET].status = 1;
		info_block[layer][x][y].ip[OFFSET].LPN = LPN;
        change_temperature('w',PBN);

        return 1;
    }
    else
    {
        // 如果不可用，都设为无效
		info_block[layer][x][y].ip[OFFSET].status = -1;
        return 0;
    }
}
//写到替换块
int chip_3d::write2(position PBN, int LPN, int val)
{
    int layer = PBN.layer;
    int x = PBN.x;
    int y = PBN.y;

    for(int i=0; i<page_per_block; i++)
    {
        //如果是该页，并为有效，则设为无效
        if(info_block[layer][x][y].ip[i].status ==1 &&
			info_block[layer][x][y].ip[i].LPN == LPN )
        {   
			info_block[layer][x][y].ip[i].status = -1;
        }
        //如果该页可用，则写到该页，并设为有效
        if(info_block[layer][x][y].ip[i].status == 0 )
        {
            change_temperature('w',PBN);
			info_block[layer][x][y].ip[i].status = 1;
			info_block[layer][x][y].ip[i].data = val;
			info_block[layer][x][y].ip[i].LPN = LPN;
            return 1;
        }
    }

    return 0;
}
//检查延迟块的有效数据，如果有，则因为更新设为无效
void chip_3d::checkVDinDelayBlock(int LPN_in_chip, list<pos_delay> delay_rpl_block){
    //延迟擦除的旧替换块数目，最多两块
    if( delay_rpl_block.size() == 0) return;

    for(list<pos_delay>::iterator l_p = delay_rpl_block.begin();
        l_p!=delay_rpl_block.end();l_p++){
        position r = l_p->p.first;
		//对每个块，都要检查一遍
        for(int i = 0;i < page_per_block; i++){
            if(info_block[r.layer][r.x][r.y].ip[i].LPN == LPN_in_chip && info_block[r.layer][r.x][r.y].ip[i].status == VALID)
                info_block[r.layer][r.x][r.y].ip[i].status = INVALID;
        }
    }
    
}
//
//void chip_3d::clearLBA_count(map<int,mapping_block_list> addr_map_table){
//    map<int,mapping_block_list>::iterator m_iter = addr_map_table.begin();
//    //将所有的计数清零，重新计数
//    while(m_iter != addr_map_table.end()){
//        m_iter->second.lba_update_count = 0;
//    }
//}
//
//int chip_3d::computeVar(map<int,mapping_block_list> addr_map_table){
//    long int total_gc_count = 0;
//    int lba_num = 0;
//    map<int,mapping_block_list>::iterator m_iter = addr_map_table.begin();
//    int max_gc_count = 0;
//    int max_gc_lba;
//    while(m_iter != addr_map_table.end()){
//        lba_num ++;
//        total_gc_count+=m_iter->second.lba_update_count;
//        if(m_iter->second.lba_update_count > max_gc_count){
//            max_gc_count = m_iter->second.lba_update_count;
//            max_gc_lba = m_iter->first;
//        }
//    }
//    
//    if(max_gc_count / total_gc_count > 0.4)
//        return lba_num;
//    return -1;
//}

void chip_3d::copyValidDataFrom3Dto2D(char p_or_r,position oldblock,list<position> new_p_list){
    //根据块的类型来
    if(p_or_r == 'p'){
        for(list<position>::iterator list_iter; list_iter!=new_p_list.end();list_iter++){
        position p = *list_iter;
        for(int i = 0;i < page_per_block;i++){
            //将大块的数据一个一个复制给小块
            Info_Block_2D *block_2d = &(chip2d->info_block_2d[1][p.x][p.y]);
            if(info_block[oldblock.layer][oldblock.x][oldblock.y].ip[i].status == VALID)
                block_2d->ip[i % page_per_block_2d].data = info_block[oldblock.layer][oldblock.x][oldblock.y].ip[i].data;
            if(i % page_per_block_2d == page_per_block_2d -1)break;
        }
    }
    }
    else{
        //
        for(list<position>::iterator list_iter; list_iter!=new_p_list.end();list_iter++){
        position p = *list_iter;
        for(int i = 0;i < page_per_block;i++){
            //将大块的数据一个一个复制给小块
            int OFFSET = info_block[oldblock.layer][oldblock.x][oldblock.y].ip[i].LPN % page_per_block;
            //找对应的块
            int num = OFFSET % page_per_block_2d;

            Info_Block_2D *block_2d = &(chip2d->info_block_2d[1][p.x][p.y]);
            if(info_block[oldblock.layer][oldblock.x][oldblock.y].ip[i].status == VALID)
                block_2d->ip[i % page_per_block_2d].data = info_block[oldblock.layer][oldblock.x][oldblock.y].ip[i].data;
            if(i % page_per_block_2d == page_per_block_2d -1)break;
        }
    }
    }


}


int chip_3d::Write3dFlash(int LPN_in_chip, cache_cell val)
{
    int i;
	//分到芯片，芯片内逻辑页号，芯片内逻辑块号，页偏移

	int LBN = LPN_in_chip / page_per_block;
	int OFFSET = LPN_in_chip % page_per_block;
    //得先看这个LBN是否迁移到2D上了,find第二个参数应该包括结束位置的元素，end指的是最后一个元素下一个位置
	
    auto result = find(hotdata_to_2d.begin(),hotdata_to_2d.end(),LBN);
    if(result != hotdata_to_2d.end()){
            //再计算芯片号
            int LCN = val.LPN % chip_num;
            chip2d->write_hotdata_2dflash(val.data,LPN_in_chip,LCN);
            //返回2意味着写入2D,不往下弄了
            return 2;
    }
        
    if(address_mapping_table.find(LBN) != address_mapping_table.end())
    {
        //有对应的物理块
        mapping_block_list *mbl = &address_mapping_table[LBN];
        //正常映射链表，可能为空，空的时候需要找基本块
        list<position> *mappingPBN = &mbl->mapping_pair.first;
        //延迟擦除的旧替换块,可能为空
        list<pos_delay> delay_rpl_block = mbl->mapping_pair.second;
        //实际映射的物理块数目,最大为3，利用position数组来保存位置
        
        int mappingblocknum = mappingPBN->size();
        //int delayblocknum = delay_rpl_block.size();
		vector<position> pbn;
        //如果有映射表有块
        int i = 0;
        if(mappingblocknum>0){
            for(list<position>::iterator l_p = mappingPBN->begin();
                l_p != mappingPBN->end(); l_p++)
                //只复制到pbn[i]
                pbn.push_back(*l_p);
        }
        //对正常映射表块的处理，写入基本块
        if(write1(pbn[0], LPN_in_chip, val.data) == 1 )
        {
			//写入数据与2个延迟的替换块相关的操作：将延迟的替换块对应数据设为无效
			//查看延迟擦除的两个替换块有没有该页数据，如果有，则设为无效
			//这里的检验操作不会影响效率，可以先写入数据，后面再做检查
			checkVDinDelayBlock(LPN_in_chip, delay_rpl_block);
			return 1;
        }
        //基本块没有可用页，写入替换块1
        else
        {
            //如果对应的块有替换块1
            if( mappingblocknum > 1 )
            {
                //成功写入替换块1
                if( write2(pbn[1], LPN_in_chip, val.data) == 1 )
                {
                    //查看延迟擦除的两个替换块有没有该页数据，如果有，则设为无效
                    checkVDinDelayBlock(LPN_in_chip,delay_rpl_block);
                    return 1;
                }
                else
                {
                    //如果对应的块有替换块2(映射块数目为3)
                    if(mappingblocknum > 2){
                        if(write2(pbn[2],LPN_in_chip,val.data) == 1){
                            //查看延迟擦除的两个替换块有没有该页数据，如果有，则设为无效
                            checkVDinDelayBlock(LPN_in_chip,delay_rpl_block);
                            return 1;
                        }
                        else{
							//搞一个lba到擦除次数的映射，只从擦除的逻辑块里找比较快
							lba_gctimes_map[LBN]++;
							//当擦除块数目到达100的倍数时，排序判断是否迁移，比如200的情况是当到达100时没有触发迁移条件
							if (lba_gctimes_map.size() % 100 == 0 ) {
								//辅助排序的临时变量
								vector<pair<int, int > > vec_map;
								for (int i = 0; i < lba_gctimes_map.size(); i++) {
									pair<int, int > p;
									p.first = i;
									p.second = lba_gctimes_map[i];
									vec_map.push_back(p);
								}
								//不能直接对map排序，转换成对vector排序,cmp定义在最上面
								sort(vec_map.begin(), vec_map.end(),cmp);
								//排序后判断是否需要迁移，迁移条件是擦除最多的块远大于平均的擦除次数，每个块都擦除一次，或者相差不到
								//2次，比如5 5 5 5 4 4 4 4 3 3 3 1 1 ，这样就没必要迁移，先设为最大的占30%，
								
								int sum = 0;
								//求和，所有的擦除次数
								vector<pair<int, int > >::iterator iter_vec = vec_map.begin();
								while (iter_vec != vec_map.end()) {
									sum += iter_vec->second;
								}
								//vec_map第一个元素就是最大的元素
								if ((vec_map.end()--)->second / sum >= 0.3) {
									//开始迁移，先找4个块给3D移动过去的块用
									list<position> PBNZ;
									for (int i = 0; i < 4; i++) {
										position p = chip2d->find_free_block();
										PBNZ.push_back(p);
									}

									int LCN = val.LPN % chip_num;
									int LBN_in_chip = LPN_in_chip % page_per_block;
									//主要是把数据复制到新块，然后修改地址映射表
									//未验证GC
									garbage_collection_3dto2d(mbl, PBNZ, LCN, LBN_in_chip);
									//清空map
									lba_gctimes_map.clear();
									return 1;
								}
								//如果不迁移，不清空map
								
							}



							////每当总的擦除次数到达100次，逻辑块更新重新计数
       //                     GC_count_gap++;
       //                     if(GC_count_gap >= GC_count_gap_limit)
       //                         //将逻辑块更新清零
       //                         clearLBA_count(address_mapping_table);
       //                     //一个逻辑块写满三个物理块，需要进行擦除，逻辑更新次数加3
       //                     mbl.lba_update_count +=3;
       //                     GC_count = GC_count_gap % 20;
							//
       //                     //擦除总次数达到20*20次
       //                     if(GC_count == 19){
       //                         //计算方差,方差过大返回1，并找出更新最多的那个逻辑块
       //                         int move_lba = 0;
       //                         if((move_lba = computeVar(address_mapping_table)) != -1){
       //                             /*排序找出要迁移的块
							//		排序要加上已经迁移过去的热数据，如果排序结果还是2d上的最热，则不动，如果有变化则迁入。
       //                             把该块迁移到2d,要做一系列对应改变，比如不用找的新块了，映射表也不改变
       //                             跨越2d芯片和3D芯片的gc，先从2D找新块,再擦除3D旧块
       //                             找块find_free_block没有不同之处，gc的过程特别，因为一个3D的页要对应4个2d的页
       //                             */
       //                             //找4个块给3D移动过去的块用
       //                             list<position> PBNZ ; 
       //                             for(int i = 0; i < 4; i++){
       //                                 position p = chip2d->find_free_block();
       //                                 PBNZ.push_back(p);
       //                             }
       //                             
       //                             int LCN = val.LPN % chip_num;
       //                             int LBN_in_chip = LPN_in_chip % page_per_block;
							//		//有变化
       //                             garbage_collection_3dto2d(&mbl,PBNZ,LCN,LBN_in_chip);
       //                             return 1;
       //                         }
       //                     }
       //                     
                            //到这里三个替换块都写入失败，需要找新块作为基本块
							//如果不迁移，则在3D内找新块做垃圾回收
                            position PBN = find_free_block(PRIMARYBLOCK);

                            // 在garbage_collection ,回收替换块1和2到空白块链表，在里面判断基本块
                            // 使用率，使用率低加入重利用链表,基本块先不擦除
							//gc里面修改地址映射表，未验证
                            garbage_collection(LBN,mbl,PBN);
							//gc里面把旧块上的数据都迁移到新块上，应该在gc里面也把引起垃圾回收的lpn更新
							//把对应的那一页直接更新
							int layer = PBN.layer;
							int x = PBN.x;
							int y = PBN.y;

							int OFFSET = LPN_in_chip % page_per_block;
							
							info_block[layer][x][y].ip[OFFSET].data = val.data;
							info_block[layer][x][y].ip[OFFSET].status = 1;
							info_block[layer][x][y].ip[OFFSET].LPN = LPN_in_chip;
							change_temperature('w', PBN);

                            
                        }
                    
                    }
                    else{
                        //如果没有替换块2，则找新块
                        position rpbn2 = find_free_block(REPLACEBLOCK);
                        write2(rpbn2, LPN_in_chip,val.data);
                        mappingPBN->push_back(rpbn2);
                        return 1;
                    }
                    
                }
            }
            else
            {   //如果没有替换块1，则找新块
                position rpbn1 = find_free_block(REPLACEBLOCK);
                write2(rpbn1, LPN_in_chip,val.data);
                mappingPBN->push_back(rpbn1);
                return 1;
            }
        }
    }
    // 到这里还没有return的话，说明映射表里面没有这个东西，则要找基本块
    else
    {
        position ppbn = find_free_block(PRIMARYBLOCK);
        write1(ppbn, LPN_in_chip, val.data);

        //修改映射对第一个正常映射
        address_mapping_table[LBN].mapping_pair.first.push_back(ppbn);
        return 1;
    }
}
