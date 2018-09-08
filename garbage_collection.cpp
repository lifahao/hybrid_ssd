#include "global.h"

int chip_3d::checkPrimaryBlock(position p){
    int freepagenum = 0;
    //统计
    for(int i = 0;i<page_per_block; i++){
        if(info_block[p.layer][p.x][p.y].ip[i].status == FREE)
            freepagenum++;
    }
    //判断,超过一半的页是可用的
    if(freepagenum > free_page_in_block_limit)
        return 1;
    else
        return 0;
}

int chip_3d::copyValidData(char block_class,position oldblock,position newblock){
    if(block_class == 'p'){
        for(int i=0; i<page_per_block; i++)
        {
            if(info_block[oldblock.layer][oldblock.x][oldblock.y].ip[i].status == 1 )
            {
                //读取基本块
                change_temperature('r', oldblock);
                //写入新块
                change_temperature('w', newblock);
                info_block[newblock.layer][newblock.x][newblock.y].ip[i] = info_block[oldblock.layer][oldblock.x][oldblock.y].ip[i];
            }
        }
    }
    else{
        for(int i=page_per_block-1; i>=0 ; i--)
        {
            if(info_block[oldblock.layer][oldblock.x][oldblock.y].ip[i].status == 1 )
            {   //读取替换块1
                change_temperature('r', oldblock);
                //写入新块
                change_temperature('w', newblock);
                int OFFSET = info_block[oldblock.layer][oldblock.x][oldblock.y].ip[i].LPN % page_per_block;
                info_block[newblock.layer][newblock.x][newblock.y].ip[OFFSET] = info_block[oldblock.layer][oldblock.x][oldblock.y].ip[i];
            }
        }
    }
	return 1;
}

int chip_3d::findAndErase(list<pos_delay> *d_c_b,pos_delay p_d){
    list<pos_delay>::iterator iter = find(d_c_b->begin(),d_c_b->end(),p_d);
    if(iter == d_c_b->end())
        return 0;
    d_c_b->erase(iter);
    return 1;
}

int chip_3d::eraseBlock(position PBNX ){
    //清理每一页数据
    for(int i=0; i<page_per_block; i++)
    {
		info_block[PBNX.layer][PBNX.x][PBNX.y].ip[i].clean();
    }
    //标记页状态为可用
    info_block[PBNX.layer][PBNX.x][PBNX.y].status = FREE;
    change_temperature( 'e',PBNX);
    block_err[PBNX.layer][PBNX.x][PBNX.y] = 0;
    info_block[PBNX.layer][PBNX.x][PBNX.y].erase_count++;
	return 1;
}
/*
三个块不一起复制到新块，否则温度会很高
先把替换块复制到新块
方案一：利用延迟复制擦除链表，该表的长度是定值，满了之后交替复制擦除
来减少一次性大量复制给新块带来的热量
第一次复制擦除基本块，如果使用率不高，则加入到只复制不擦除，
，加入到重用链表，把两个替换块加入延迟复制擦除链表表尾，
方案二：根据总的空间利用情况来调整复制擦除策略，如果空间紧张，则
都擦除，如果空间不紧张，则采用方案一
后面把基本块复制到新块，把基本块加入到待复制块（什么时候复制？）
*/
//LBN主要是修改mapping_block_list的延迟链表时用
int chip_3d::garbage_collection(int LBN,mapping_block_list* mbl,position n_PBN)
{
    
    list<position>* mappingPBN = &mbl->mapping_pair.first;
    //延迟擦除的旧替换块,可能为空
    list<pos_delay> *delay_rpl_block = &mbl->mapping_pair.second;
    //先处理正常映射
    int mappingblocknum = mappingPBN->size();
    //int delayblocknum = delay_rpl_block.size();
    vector<position> pbn;
    //如果有映射表有块
    int i = 0;
    if(mappingblocknum>0){
        for(list<position>::iterator l_p = mappingPBN->begin();
            l_p != mappingPBN->end(); l_p++)
            //只复制到pbn[i]
            pbn.push_back( *l_p);
    }
    //如果正常链表的块需要回收，延迟的块还没有处理，先处理延迟的，再把正常块加入到延迟
    //链表
    int size_rpl = delay_rpl_block->size();
    for(int i = 0; i < size_rpl; i++){
        pos_delay p_d = delay_rpl_block->front();
        delay_rpl_block->pop_front();
        //不要把数据放到延迟块该放的基本块了，因为这时候基本块也满了
        //放到新选择的基本块
        position r_old = p_d.p.first;
        copyValidData('r',r_old,n_PBN);
        eraseBlock(r_old);
        //对应的从延迟链表中删除
        if(findAndErase(&delay_copy_block,p_d) == 0)
            cout<<"delay_copy_block no this block"<<endl;
    }
    //清空延迟映射链表
    delay_rpl_block->clear();
    

    //先复制基本块的有效数据到新块，替换块先不用，等到擦除的时候再复制过去
    copyValidData('p',pbn[0],n_PBN);
    //检验基本块可用页数目，符合回收条件加入回收利用链表
    if(checkPrimaryBlock(pbn[0])){
        //加入回收利用链表
        reused_primary_block.push_back(pbn[0]);
    }
    else{
        //不符合，则直接擦除
        eraseBlock(pbn[0]);
    }
    //因为替换块都是成对加入的，如果延迟复制擦除链表满了，则先处理链表前两个
    if(delay_copy_block.size() > 100){
        pos_delay r1,r3;
        //r1为第一块,类推
        
        if(d_erase_count > 10){
            d_erase_p = delay_copy_block.begin();
        }
        r1 = *d_erase_p;
        delay_copy_block.erase(d_erase_p);
        ++d_erase_p++;
        r3 = *d_erase_p;
        delay_copy_block.erase(d_erase_p);
        ++d_erase_p++;

        //处理r1和r3
        position r1_old = r1.p.first;
        position r1_new = r1.p.second;
        position r3_old = r3.p.first;
        position r3_new = r3.p.second;
        //复制旧块到新块
        copyValidData('r',r1_old,r1_new);
        copyValidData('r',r3_old,r3_new);
        //擦除链表的头部两块，eraseBlock主要是擦除块上的数据，上面已经从delay_copy_block移除了
        eraseBlock(r1_old);
        eraseBlock(r3_old);

        //除了处理总的映射表delay_copy_block外，还要处理单个映射的delay_rpl_block,修改对应的mapping_block_list,这里需要LBN以找到
		//对应的擦除了的mapping_block_list,要找r1,r3的LPN
		mapping_block_list *m1 = &address_mapping_table[r1.LBN],*m3 = &address_mapping_table[r3.LBN];
		//先修改m1第二个映射对，删除
		list<pos_delay> *list1 = &m1->mapping_pair.second;
		list<pos_delay>::iterator iter_list = list1->begin();
		while (iter_list != list1->end()) {
			if (*iter_list == r1)
				list1->erase(iter_list);
		}
		//再修改m3
		list<pos_delay> *list3 = &m3->mapping_pair.second;
		iter_list = list3->begin();
		while (iter_list != list3->end()) {
			if (*iter_list == r3)
				list3->erase(iter_list);
		}

    }
    //处理旧块，将两个替换块加入到延迟擦除链表，pos_delay中只有一个pair<position,position>第一个代表旧块位置，第二个代表新块位置
    pos_delay r1,r2;
	//这里保存LBN，方便上面擦除的时候修改对应的映射单元
    r1.p.first = pbn[1];
    r2.p.first = pbn[2];
	r1.LBN = r2.LBN = LBN;
    r1.p.second = r2.p.second = n_PBN;

    //加入到地址映射表中第二个，延迟链表
    delay_copy_block.push_back(r1);
    delay_copy_block.push_back(r2);

    //修改地址映射表
    //再修改映射对中第二个延迟映射对，因为旧的数据都在上面，所以后面再更新的时候需要修改上面旧的数据，设为无效
	//同样，读取的时候也要考虑旧块上的有效数据
    delay_rpl_block->push_back(r1);
    delay_rpl_block->push_back(r2);

    //再修改正常映射表
    mappingPBN->clear();
    mappingPBN->push_back(n_PBN);

    
    return 1;
}


//一个3D块需要4个2D的块(假设页一样大，页数目是1/4），地址映射表是特殊的，单独一个
int chip_3d::garbage_collection_3dto2d(mapping_block_list *mbl, list<position> new_p_list, int c_num, int lbn) {

	//这里需要考虑分配给热数据的空间不足的情况，如果空间不足，需要将最不热的数据迁移回去，腾出地方给新来的
	//分配给热数据块数是固定的。要不先假设2d和3d上的块一样大，页数一样多，这样好实现。对于地址映射也好弄。
	//先对3d处理，包括擦除三个旧块，修改地址映射表，读取数据写入到2D
	list<position>* mappingPBN = &mbl->mapping_pair.first;
	//延迟擦除的旧替换块,可能为空
	list<pos_delay> *delay_rpl_block = &mbl->mapping_pair.second;
	//先处理延迟映射块，把旧块有效数据复制到2D上的新块
	int size_rpl = delay_rpl_block->size();
	for (int i = 0; i < size_rpl; i++) {
		pos_delay p_d = delay_rpl_block->front();
		delay_rpl_block->pop_front();
		//不要把数据放到延迟块该放的基本块了，因为这时候基本块也满了
		//放到新选择的基本块
		position r_old = p_d.p.first;
		copyValidDataFrom3Dto2D('r', r_old, new_p_list);
		eraseBlock(r_old);
		//对应的从延迟链表中删除
		if (findAndErase(&delay_copy_block, p_d) == 0)
			cout << "delay_copy_block no this block" << endl;
	}
	//清空延迟映射链表
	delay_rpl_block->clear();
	//再处理正常映射块
	int mappingblocknum = mappingPBN->size();
	int i = 0;
	//如果有映射表有块
	if (mappingblocknum>0) {
		for (list<position>::iterator l_p = mappingPBN->begin();
			l_p != mappingPBN->end(); l_p++) {
			//只复制到pbn[i]
			position old_p = *l_p;
			if (i == 0)
				copyValidDataFrom3Dto2D('p', old_p, new_p_list);
			else
				copyValidDataFrom3Dto2D('r', old_p, new_p_list);
		}
		i++;
	}
	//再对2d进行处理，先修改地址映射表，再写入数据
	//热数据的地址映射管理有些特殊，因为是总的LPN，数目也少，所以采用链表管理
	pair<int, list<position> > pair_map;
	pair_map.first = lbn;
	pair_map.second = new_p_list;
	chip2d->hot_data_address_mapping_table[c_num].push_back(pair_map);
	return 1;
}
void chip_2d::garbage_collection_hotdata(vector<position> pbnx, vector<position > pbny, vector<position > pbnz) {
	//复制8个旧块到有效数据到4个新块
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < page_per_block_2d; j++) {
			if (info_block_2d[pbnx[i].layer][pbnx[i].x][pbnx[i].y].ip[j].status == VALID)
			{
				//基本块上有效页复制到对应的新块页上
				info_block_2d[pbnz[i].layer][pbnz[i].x][pbnz[i].y].ip[j] = info_block_2d[pbnx[i].layer][pbnx[i].x][pbnx[i].y].ip[j];
			}
		}
	}
	for (int i = 4; i < 8; i++) {
		for (int j = 0; j < page_per_block_2d; j++) {
			if (info_block_2d[pbnx[i].layer][pbnx[i].x][pbnx[i].y].ip[j].status == VALID)
			{
				//基本块上有效页复制到对应的新块页上
				int block_num = info_block_2d[pbnx[i].layer][pbnx[i].x][pbnx[i].y].ip[j].LPN % 4;
				int offset = info_block_2d[pbnx[i].layer][pbnx[i].x][pbnx[i].y].ip[j].LPN / 4;
				info_block_2d[pbnz[block_num].layer][pbnz[block_num].x][pbnz[block_num].y].ip[offset] = info_block_2d[pbnx[i].layer][pbnx[i].x][pbnx[i].y].ip[j];

			}
		}
	}
	//回收4个基本块4个替换块
	for (int i = 0; i < 4; i++) {
		info_block_2d[pbnx[i].layer][pbnx[i].x][pbnx[i].y].ip[i].clean();
		info_block_2d[pbny[i].layer][pbny[i].x][pbny[i].y].ip[i].clean();
		free_block.push_back(pbnx[i]);
		free_block.push_back(pbny[i]);

	}
}

