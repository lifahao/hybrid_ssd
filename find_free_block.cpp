#include "global.h"
position chip_3d::find_free_block1() {
	position newblock;
	//每次返回一个特定ls值的位置
	while (mols.ls_val < mols.max_ls_val + 1) {
		for (int i = 0; i < layer_num; i++) {
			position *p = &mols.ls_p[mols.ls_val][i].pos;
			//找可用位置
			while (info_block[p->layer][p->x][p->y].status == USED) {
				if (p->x == row_num - 1) {
					p->x = (p->x + 2) % row_num;
					p->y = (p->y + mols.max_ls_val - 2) % mols.max_ls_val;
					//从头找没有可用的，再回到首行，跳出循环
					break;
				}
				else {
					p->x = (p->x + 1) % row_num;
					p->y = (p->y + mols.max_ls_val - 1 - i) % mols.max_ls_val;
				}
				while (p->y > column_num - 1) {
					p->y = (p->y + mols.max_ls_val - 1 - i) % mols.max_ls_val;
					p->y = (p->y + 1) % row_num;
				}
			}
			while (info_block[p->layer][p->x][p->y].status == FREE && mols.ls_val == info_block[p->layer][p->x][p->y].ls_val) {
				if (p->y < column_num) {
					mols.count[mols.ls_val]++;
					newblock.layer = p->layer;
					newblock.x = p->x;
					newblock.y = p->y;
					info_block[p->layer][p->x][p->y].status = USED;
					return newblock;
				}
				//更改特定ls值的初始位置，方便下一次使用
				if (p->x == row_num - 1) {
					p->x = (p->x + 2) % row_num;
					p->y = (p->y + mols.max_ls_val - 2) % mols.max_ls_val;
				}
				else {
					p->x = (p->x + 1) % row_num;
					p->y = (p->y + mols.max_ls_val - 1 - i) % mols.max_ls_val;
				}
				while (p->y > column_num - 1) {
					p->y = (p->y + mols.max_ls_val - 1 - i) % mols.max_ls_val;
					p->y = (p->y + 1) % row_num;
				}
			}
		}
		mols.ls_val = (mols.ls_val + 1) % mols.max_ls_val;
	}
	return newblock;

}
int chip_3d::find_erase_lba(){
    //应该找符合条件的块擦除，找写的页最多的块进行擦除
	int lba_num = 0;
	map<int, mapping_block_list>::iterator m_iter = address_mapping_table.begin();
	int max_lba = 0;
	int max_up_count, min_up_count;
	//这里不用lba_update_count，用lba_gctimes_nums代替
	/*max_up_count = min_up_count = m_iter->second.lba_update_count;
	while (m_iter != address_mapping_table.end()) {
		lba_num++;
		if (m_iter->second.lba_update_count > max_up_count) {
			max_up_count = m_iter->second.lba_update_count;
			max_lba = m_iter->first;
		}
	}*/
	if (address_mapping_table[max_lba].mapping_pair.first.size() >= 2)
		return max_lba;
	else
		return -1;
}

position chip_3d::find_free_block(int p_or_r)
{
    /*
    如果是找替换块，先从可重用链表找，如果链表为空
    则直接按照MOLS顺序找
    */
    position newblock;
	//如果要找的是替换块，并且可重用链表有可用块，则先从可重用链表中找替换块
    if(p_or_r == REPLACEBLOCK && reused_primary_block.size() > 0){
			newblock = reused_primary_block.front();
			reused_primary_block.pop_front();
			return newblock;
    }

    //如果空间不足,小于阈值
    if(free_block_num < least_free_block){
        //怎样找擦除的块？通过一个逻辑块管理链表，这个链表也是决定迁移的条件
		int erase_lba = 0;
		erase_lba = find_erase_lba();
		if(erase_lba == -1)
			cout<<"find_free_block error\n";
		//int garbage_collection(mapping_block_list* mbl, position newblock);
		position p = find_free_block1();
		garbage_collection(erase_lba,&address_mapping_table[erase_lba], p);
    }
	return find_free_block1();

}
