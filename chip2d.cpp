#include "global.h"

//因为页是一样大的，LPN是统一的，3D上的一页，转移到2D上，也是同样的一页。
int chip_2d::read1(position PBNX, int LPN, Info_Page &val)
{
	int OFFSET = LPN % page_per_block_2d;

	if (info_block_2d[PBNX.layer][PBNX.x][PBNX.y].ip[OFFSET].status == 1)
	{
		change_time('r');
		val = info_block_2d[PBNX.layer][PBNX.x][PBNX.y].ip[OFFSET];
		return 1;
	}
	return 0;
}

int chip_2d::read2(position PBNY, int LPN, Info_Page &val)
{
	// 一定是读最新的那一个，所以是从后往前的第一个
	for (int i = page_per_block_2d - 1; i >= 0; i--)
	{
		if (info_block_2d[PBNY.layer][PBNY.x][PBNY.y].ip[i].LPN == LPN)
		{
			val = info_block_2d[PBNY.layer][PBNY.x][PBNY.y].ip[i];
			change_time('r');
			return 1;
		}
	}
	return 0;
}

int chip_2d::read_2dflash(int LPN, Info_Page &val)
{
	int LBN = LPN / page_per_block_2d;
	// 根据逻辑块号去进行查询
	/*
	当数据出现时，它返回数据所在位置的迭代器，
	如果map中没有要查找的数据，
	它返回的迭代器等于end函数返回的迭代器
	*/

	if (address_mapping_table.find(LBN) != address_mapping_table.end())
	{
		pair<position, position> mappingPBN = address_mapping_table[LBN];

		position PBNX = mappingPBN.first;
		position PBNY = mappingPBN.second;

		if (read1(PBNX, LPN, val) == 1)
			return 1;
		else // 如果不在PBNX里面，就一定在PBNY里面，因为映射表存在，表明一定能找到这样的一个数据
		{
			if (PBNY.layer == -1) return 0;
			read2(PBNY, LPN, val);
			return 1;
		}
	}
	// 找不到映射，读数据失败的时候返回值就是0
	return 0;
}

int chip_2d::write1(position PBN, int LPN, int val)
{
	int layer = PBN.layer;
	int x = PBN.x;
	int y = PBN.y;

	int OFFSET = LPN % page_per_block_2d;

	if (info_block_2d[layer][x][y].ip[OFFSET].status == 0)
	{
		info_block_2d[layer][x][y].ip[OFFSET].data = val;
		info_block_2d[layer][x][y].ip[OFFSET].status = 1;
		info_block_2d[layer][x][y].ip[OFFSET].LPN = LPN;
		change_time('w');
		return 1;
	}
	else
	{
		// 写不成功记得改回来
		info_block_2d[layer][x][y].ip[OFFSET].status = -1;
		return 0;
	}
}

int chip_2d::write2(position PBN, int LPN, int val)
{
	int layer = PBN.layer;
	int x = PBN.x;
	int y = PBN.y;

	for (int i = 0; i<page_per_block_2d; i++)
	{
		if (info_block_2d[layer][x][y].ip[i].status == 1 &&
			info_block_2d[layer][x][y].ip[i].LPN == LPN)
		{
			info_block_2d[layer][x][y].ip[i].status = -1;
		}
		if (info_block_2d[layer][x][y].ip[i].status == 0)
		{
			change_time('w');
			info_block_2d[layer][x][y].ip[i].status = 1;
			info_block_2d[layer][x][y].ip[i].data = val;
			info_block_2d[layer][x][y].ip[i].LPN = LPN;
			return 1;
		}
	}

	return 0;
}

int chip_2d::write_2dflash(cache_cell cell)
{
	//cell的LPN是全局的，这里应该是全局的
	int LPN = cell.LPN;
	int val = cell.data;

	int LBN = LPN / page_per_block_2d;

	if (address_mapping_table.find(LBN) != address_mapping_table.end())
	{
		pair<position, position> mappingPBN = address_mapping_table[LBN];
		position PBNX = mappingPBN.first;
		position PBNY = mappingPBN.second;

		if (write1(PBNX, LPN, val) == 1)
		{
			return 1;
		}
		else
		{
			if (PBNY.layer != -1)
			{
				if (write2(PBNY, LPN, val) == 1)
					return 1;
				else
				{
					// find_free_block 的时候就已经将物理块置1了
					position PBNZ = find_free_block();
					write1(PBNZ, LPN, val);

					pair<position, position> new_mapping = make_pair(PBNZ, position(-1, -1, -1));
					address_mapping_table[LPN / page_per_block_2d] = new_mapping;

					// 在garbage_collection 那里就将PBNX和PBNY回收到空白块链表了
					garbage_collection(PBNX, PBNY, PBNZ);

					return 1;
					// 更新地址表
				}
			}
			else
			{
				position PBNY = find_free_block();
				write2(PBNY, LPN, val);
				pair<position, position> new_mapping = make_pair(PBNX, PBNY);
				address_mapping_table[LBN] = new_mapping;
				return 1;
			}
		}
	}
	// 到这里还没有return的话，说明映射表里面没有这个东西
	else
	{
		// find_free_block 里面要删掉已经被用掉的
		position PBNX = find_free_block();
		write1(PBNX, LPN, val);
		pair<position, position> new_mapping = make_pair(PBNX, position(-1, -1, -1));
		address_mapping_table[LBN] = new_mapping;
		return 1;
	}
}


void change_time(char op) {
	double val_time;

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

	return;
}


position chip_2d::find_free_block()
{
	list<position>::iterator lt;
	map<int, pair<position, position> >::iterator mt;

	int tot_block = row_num_2d * column_num_2d;
	int least_free_block = tot_block * (1 - FB_threshold);


	for (mt = address_mapping_table.begin(); mt != address_mapping_table.end(); mt++)
	{
		if (free_block.size() > least_free_block) break;

		int LBN = mt->first;
		pair<position, position> PBN = mt->second;

		// 如果映射的地址还有一个空间没有用，那就没有回收的必要，跳过下一个
		if (PBN.second.layer == -1) continue;

		else
		{
			/**
			data, LPN, ecc, status
			*/
			position newblock_3d = free_block.front();
			lt = free_block.begin();
			free_block.erase(lt);

			position PBNX = PBN.first;
			position PBNY = PBN.second;

			garbage_collection(PBNX, PBNY, newblock_3d);

			//            garbage_collection里面没有更新映射表，这里要更新
			pair<position, position> new_mapping = make_pair(newblock_3d, position(-1, -1, -1));
			address_mapping_table[LBN] = new_mapping;
		}
	}


	position newblock_3d = free_block.front();
	lt = free_block.begin();
	free_block.erase(lt);
	return newblock_3d;
}


int chip_2d::garbage_collection(position PBNX, position PBNY, position PBNZ)
{
	int i;
	// 同一块上面不可能有同一个offset的
	for (i = 0; i<page_per_block_2d; i++)
	{
		if (info_block_2d[PBNX.layer][PBNX.x][PBNX.y].ip[i].status == 1)
		{
			change_time('r');
			change_time('w');
			info_block_2d[PBNZ.layer][PBNZ.x][PBNZ.y].ip[i] = info_block_2d[PBNX.layer][PBNX.x][PBNX.y].ip[i];
		}
	}
	int cnt = 0;
	for (i = page_per_block_2d - 1; i >= 0; i--)
	{
		if (info_block_2d[PBNY.layer][PBNY.x][PBNY.y].ip[i].status == 1)
		{
			change_time('r');
			change_time('w');
			int OFFSET = info_block_2d[PBNY.layer][PBNY.x][PBNY.y].ip[i].LPN % page_per_block_2d;
			info_block_2d[PBNZ.layer][PBNZ.x][PBNZ.y].ip[OFFSET] = info_block_2d[PBNY.layer][PBNY.x][PBNY.y].ip[i];
		}
	}

	for (i = 0; i<page_per_block_2d; i++)
	{
		info_block_2d[PBNX.layer][PBNX.x][PBNX.y].ip[i].clean();
		info_block_2d[PBNY.layer][PBNY.x][PBNY.y].ip[i].clean();
	}

	change_time('e');
	change_time('e');

	info_block_2d[PBNX.layer][PBNX.x][PBNX.y].erase_count++;
	info_block_2d[PBNY.layer][PBNY.x][PBNY.y].erase_count++;

	block_err[PBNX.layer][PBNX.x][PBNX.y] = 0;
	block_err[PBNY.layer][PBNY.x][PBNY.y] = 0;

	// 垃圾回收之后要记得清空将原来的块加入空白块链表中去
	free_block.push_back(PBNX);
	free_block.push_back(PBNY);
	return 1;
}



//处理热数据
//处理热数据的读操作
int chip_2d::read_hotdata_2dflash(Info_Page& val, int lpn, int lcn) {
	//先查地址映射表
	int lbn = lpn / page_per_block;
	int offset = lpn % page_per_block;
	vector<pair<int, list<position> > >::iterator iter_b = chip2d->hot_data_address_mapping_table[lcn].begin();
	vector<pair<int, list<position> > >::iterator iter_e = chip2d->hot_data_address_mapping_table[lcn].end();
	vector<position> pbn_vec;
	while (iter_b != iter_e) {
		//找到
		if (iter_b->first == lbn) {
			for (list<position>::iterator iter_l = (iter_b->second).begin(); iter_l != (iter_b->second).end(); iter_l++) {
				pbn_vec.push_back(*iter_l);
			}
			break;
		}
		iter_b++;
	}


	//在物理块上找写入的位置
	//4个基本块,采用offset的方式
	if (pbn_vec.size() == 4) {
		int block_num = offset / page_per_block_2d;
		int page_num = offset % page_per_block_2d;
		position p = pbn_vec[block_num];
		if (chip2d->info_block_2d[p.layer][p.x][p.y].ip[page_num].status == VALID) {
			//读到val，返回
			val = chip2d->info_block_2d[p.layer][p.x][p.y].ip[page_num];

			return 1;
		}
	}

	//4个替换块,一个一个找，先把有效页设为无效，再找可用页
	for (int i = 4; i< 8; i++) {
		position p = pbn_vec[i];
		for (int j = 0; j < page_per_block_2d; j++) {
			if (lpn == chip2d->info_block_2d[p.layer][p.x][p.y].ip[j].LPN
				&&chip2d->info_block_2d[p.layer][p.x][p.y].ip[j].status == VALID) {
				val = chip2d->info_block_2d[p.layer][p.x][p.y].ip[j];
				return 1;
			}
		}
	}
	return 1;
}

//写操作

int chip_2d::write_hotdata_2dflash(int data, int lpn, int lcn) {
	//先查地址映射表
	int lbn = lpn / page_per_block;
	int offset = lpn % page_per_block;
	vector<pair<int, list<position> > >::iterator iter_b = chip2d->hot_data_address_mapping_table[lcn].begin();
	vector<pair<int, list<position> > >::iterator iter_e = chip2d->hot_data_address_mapping_table[lcn].end();
	vector<position> pbn_vec;
	while (iter_b != iter_e) {
		//找到
		if (iter_b->first == lbn) {
			for (list<position>::iterator iter_l = (iter_b->second).begin(); iter_l != (iter_b->second).end(); iter_l++) {
				pbn_vec.push_back(*iter_l);
			}
			break;
		}
		iter_b++;
	}


	//在物理块上找写入的位置
	//4个基本块,采用offset的方式
	if (pbn_vec.size() == 4) {
		int block_num = offset / page_per_block_2d;
		int page_num = offset % page_per_block_2d;
		position p = pbn_vec[block_num];
		if (chip2d->info_block_2d[p.layer][p.x][p.y].ip[page_num].status == FREE) {
			//写到对应基本块，返回
			chip2d->info_block_2d[p.layer][p.x][p.y].ip[page_num].data = data;
			chip2d->info_block_2d[p.layer][p.x][p.y].ip[page_num].status = VALID;
			chip2d->info_block_2d[p.layer][p.x][p.y].ip[page_num].LPN = lpn;
			return 1;
		}
	}

	//4个替换块,一个一个找，先把有效页设为无效，再找可用页
	for (int i = 4; i< 8; i++) {
		position p = pbn_vec[i];
		for (int j = 0; j < page_per_block_2d; j++) {
			if (lpn == chip2d->info_block_2d[p.layer][p.x][p.y].ip[j].LPN
				&&chip2d->info_block_2d[p.layer][p.x][p.y].ip[j].status == VALID) {
				chip2d->info_block_2d[p.layer][p.x][p.y].ip[j].status = INVALID;
			}
			if (chip2d->info_block_2d[p.layer][p.x][p.y].ip[j].status == FREE) {
				chip2d->info_block_2d[p.layer][p.x][p.y].ip[j].data = data;
				chip2d->info_block_2d[p.layer][p.x][p.y].ip[j].status = VALID;
				chip2d->info_block_2d[p.layer][p.x][p.y].ip[j].LPN = lpn;
				return 1;
			}
		}
	}
	vector<position> pbnz;
	for (int i = 0; i < 4; i++) {
		position p = chip2d->find_free_block();
		pbnz.push_back(p);
	}
	vector<position> pbnx, pbny;
	int i;
	for (i = 0; i < 4; i++) {
		pbnx.push_back(pbn_vec[i]);
	}
	for (; i<8; i++) {
		pbny.push_back(pbn_vec[i]);
	}
	//如果这个时候还没返回，则意味着需要垃圾回收
	garbage_collection_hotdata(pbnx, pbny, pbnz);

	//再修改地址映射表
	vector<position>::iterator iter_pbnz = pbnz.begin();
	while (iter_pbnz != pbnz.end())
	{
		(iter_b->second).push_back(*iter_pbnz);
	}
	return 1;
}



void chip_2d::copyValidDataFrom2Dto3D(int lcn, position newblock_3d, list<position> old_list_2d) {
	//根据块的类型来,大于4块代表有基本块
	if (old_list_2d.size() == 4) {
		for (list<position>::iterator list_iter; list_iter != old_list_2d.end(); list_iter++) {
			position p = *list_iter;
			Info_Block_2D *block_2d = &info_block_2d[1][p.x][p.y];

			for (int i = 0, j = 1; i < page_per_block; i++) {
				//将2D上的基本块复制到3D大块
				if (block_2d->ip[i % page_per_block_2d].status == VALID)
					chips3d[lcn]->info_block[newblock_3d.layer][newblock_3d.x][newblock_3d.y].ip[i].data = block_2d->ip[i % page_per_block_2d].data;
				if (i % page_per_block_2d == page_per_block_2d - 1)break;
			}
		}
	}
	if (old_list_2d.size()>4) {
		//
		for (list<position>::iterator list_iter; list_iter != old_list_2d.end(); list_iter++) {
			position p = *list_iter;
			Info_Block_2D *block_2d = &info_block_2d[1][p.x][p.y];

			for (int i = 0; i < page_per_block; i++) {
				if (block_2d->ip[i % page_per_block_2d].status == VALID) {
					//计算偏移量
					int offset = block_2d->ip[i % page_per_block_2d].LPN % page_per_block;
					chips3d[lcn]->info_block[newblock_3d.layer][newblock_3d.x][newblock_3d.y].ip[offset].data = block_2d->ip[i % page_per_block_2d].data;
				}
				if (i % page_per_block_2d == page_per_block_2d - 1)break;
			}
		}
	}

}



void chip_2d::garbage_collection_2dto3d(int cn, int min_lba) {
	//当2D上热数据空间不足时，需要回收热数据块到3d上，腾出空间
	//需要修改3D上记录迁移热数据块的链表，修改2D上的映射表
	//应该用在findFreeblock里面，找新热块的时候，发现没有块可用，就得把不符合要求的
	//迁移回去，这个需要迁回的块直接从排序结果上找，写入频率最低的热块迁回所属的芯片
	//先对四个芯片的热数据映射表对逻辑块的写入次数进行排序
	//3d芯片中有这个vector<int> hotdata_to_2d，可以利用这个在garbage_collection_3dto2d
	//中调用garbage_collection_2dto3d
	//先从3d上找一块
	position new_p_3d = chips3d[cn]->find_free_block(PRIMARYBLOCK);

	list<position> old_list_2d;
	vector<  pair<int, list<position> > >::iterator iter = hot_data_address_mapping_table[cn].begin();
	//找到擦除次数最小的块的位置
	while (iter != hot_data_address_mapping_table[cn].end()) {
		if (iter->first == min_lba) {
			old_list_2d = iter->second;
			break;
		}
	}

	//复制有效数据到新块new_p,先复制基本块
	copyValidDataFrom2Dto3D(cn, new_p_3d, old_list_2d);
	//擦除旧块

	//修改映射表,先修改2D上的
	hot_data_address_mapping_table[cn].erase(iter);
	//再修改3D上的
	vector<int>::iterator iter_vec = find(chips3d[cn]->hotdata_to_2d.begin(), chips3d[cn]->hotdata_to_2d.end(), min_lba);
	chips3d[cn]->hotdata_to_2d.erase(iter_vec);
	//要迁回的芯片映射表
	//新建正常映射链表
	mapping_block_list mbl;
	//正常映射链表，可能为空，空的时候需要找基本块
	mbl.mapping_pair.first.push_back(new_p_3d);
	mbl.mapping_pair.second.clear();
	// find_free_block 里面要删掉已经被用掉的
	chips3d[cn]->address_mapping_table[min_lba] = mbl;
}
