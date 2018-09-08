#include "global.h"

//��Ϊҳ��һ����ģ�LPN��ͳһ�ģ�3D�ϵ�һҳ��ת�Ƶ�2D�ϣ�Ҳ��ͬ����һҳ��
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
	// һ���Ƕ����µ���һ���������ǴӺ���ǰ�ĵ�һ��
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
	// �����߼����ȥ���в�ѯ
	/*
	�����ݳ���ʱ����������������λ�õĵ�������
	���map��û��Ҫ���ҵ����ݣ�
	�����صĵ���������end�������صĵ�����
	*/

	if (address_mapping_table.find(LBN) != address_mapping_table.end())
	{
		pair<position, position> mappingPBN = address_mapping_table[LBN];

		position PBNX = mappingPBN.first;
		position PBNY = mappingPBN.second;

		if (read1(PBNX, LPN, val) == 1)
			return 1;
		else // �������PBNX���棬��һ����PBNY���棬��Ϊӳ�����ڣ�����һ�����ҵ�������һ������
		{
			if (PBNY.layer == -1) return 0;
			read2(PBNY, LPN, val);
			return 1;
		}
	}
	// �Ҳ���ӳ�䣬������ʧ�ܵ�ʱ�򷵻�ֵ����0
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
		// д���ɹ��ǵøĻ���
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
	//cell��LPN��ȫ�ֵģ�����Ӧ����ȫ�ֵ�
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
					// find_free_block ��ʱ����Ѿ����������1��
					position PBNZ = find_free_block();
					write1(PBNZ, LPN, val);

					pair<position, position> new_mapping = make_pair(PBNZ, position(-1, -1, -1));
					address_mapping_table[LPN / page_per_block_2d] = new_mapping;

					// ��garbage_collection ����ͽ�PBNX��PBNY���յ��հ׿�������
					garbage_collection(PBNX, PBNY, PBNZ);

					return 1;
					// ���µ�ַ��
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
	// �����ﻹû��return�Ļ���˵��ӳ�������û���������
	else
	{
		// find_free_block ����Ҫɾ���Ѿ����õ���
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

		// ���ӳ��ĵ�ַ����һ���ռ�û���ã��Ǿ�û�л��յı�Ҫ��������һ��
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

			//            garbage_collection����û�и���ӳ�������Ҫ����
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
	// ͬһ�����治������ͬһ��offset��
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

	// ��������֮��Ҫ�ǵ���ս�ԭ���Ŀ����հ׿�������ȥ
	free_block.push_back(PBNX);
	free_block.push_back(PBNY);
	return 1;
}



//����������
//���������ݵĶ�����
int chip_2d::read_hotdata_2dflash(Info_Page& val, int lpn, int lcn) {
	//�Ȳ��ַӳ���
	int lbn = lpn / page_per_block;
	int offset = lpn % page_per_block;
	vector<pair<int, list<position> > >::iterator iter_b = chip2d->hot_data_address_mapping_table[lcn].begin();
	vector<pair<int, list<position> > >::iterator iter_e = chip2d->hot_data_address_mapping_table[lcn].end();
	vector<position> pbn_vec;
	while (iter_b != iter_e) {
		//�ҵ�
		if (iter_b->first == lbn) {
			for (list<position>::iterator iter_l = (iter_b->second).begin(); iter_l != (iter_b->second).end(); iter_l++) {
				pbn_vec.push_back(*iter_l);
			}
			break;
		}
		iter_b++;
	}


	//�����������д���λ��
	//4��������,����offset�ķ�ʽ
	if (pbn_vec.size() == 4) {
		int block_num = offset / page_per_block_2d;
		int page_num = offset % page_per_block_2d;
		position p = pbn_vec[block_num];
		if (chip2d->info_block_2d[p.layer][p.x][p.y].ip[page_num].status == VALID) {
			//����val������
			val = chip2d->info_block_2d[p.layer][p.x][p.y].ip[page_num];

			return 1;
		}
	}

	//4���滻��,һ��һ���ң��Ȱ���Чҳ��Ϊ��Ч�����ҿ���ҳ
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

//д����

int chip_2d::write_hotdata_2dflash(int data, int lpn, int lcn) {
	//�Ȳ��ַӳ���
	int lbn = lpn / page_per_block;
	int offset = lpn % page_per_block;
	vector<pair<int, list<position> > >::iterator iter_b = chip2d->hot_data_address_mapping_table[lcn].begin();
	vector<pair<int, list<position> > >::iterator iter_e = chip2d->hot_data_address_mapping_table[lcn].end();
	vector<position> pbn_vec;
	while (iter_b != iter_e) {
		//�ҵ�
		if (iter_b->first == lbn) {
			for (list<position>::iterator iter_l = (iter_b->second).begin(); iter_l != (iter_b->second).end(); iter_l++) {
				pbn_vec.push_back(*iter_l);
			}
			break;
		}
		iter_b++;
	}


	//�����������д���λ��
	//4��������,����offset�ķ�ʽ
	if (pbn_vec.size() == 4) {
		int block_num = offset / page_per_block_2d;
		int page_num = offset % page_per_block_2d;
		position p = pbn_vec[block_num];
		if (chip2d->info_block_2d[p.layer][p.x][p.y].ip[page_num].status == FREE) {
			//д����Ӧ�����飬����
			chip2d->info_block_2d[p.layer][p.x][p.y].ip[page_num].data = data;
			chip2d->info_block_2d[p.layer][p.x][p.y].ip[page_num].status = VALID;
			chip2d->info_block_2d[p.layer][p.x][p.y].ip[page_num].LPN = lpn;
			return 1;
		}
	}

	//4���滻��,һ��һ���ң��Ȱ���Чҳ��Ϊ��Ч�����ҿ���ҳ
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
	//������ʱ��û���أ�����ζ����Ҫ��������
	garbage_collection_hotdata(pbnx, pbny, pbnz);

	//���޸ĵ�ַӳ���
	vector<position>::iterator iter_pbnz = pbnz.begin();
	while (iter_pbnz != pbnz.end())
	{
		(iter_b->second).push_back(*iter_pbnz);
	}
	return 1;
}



void chip_2d::copyValidDataFrom2Dto3D(int lcn, position newblock_3d, list<position> old_list_2d) {
	//���ݿ��������,����4������л�����
	if (old_list_2d.size() == 4) {
		for (list<position>::iterator list_iter; list_iter != old_list_2d.end(); list_iter++) {
			position p = *list_iter;
			Info_Block_2D *block_2d = &info_block_2d[1][p.x][p.y];

			for (int i = 0, j = 1; i < page_per_block; i++) {
				//��2D�ϵĻ����鸴�Ƶ�3D���
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
					//����ƫ����
					int offset = block_2d->ip[i % page_per_block_2d].LPN % page_per_block;
					chips3d[lcn]->info_block[newblock_3d.layer][newblock_3d.x][newblock_3d.y].ip[offset].data = block_2d->ip[i % page_per_block_2d].data;
				}
				if (i % page_per_block_2d == page_per_block_2d - 1)break;
			}
		}
	}

}



void chip_2d::garbage_collection_2dto3d(int cn, int min_lba) {
	//��2D�������ݿռ䲻��ʱ����Ҫ���������ݿ鵽3d�ϣ��ڳ��ռ�
	//��Ҫ�޸�3D�ϼ�¼Ǩ�������ݿ�������޸�2D�ϵ�ӳ���
	//Ӧ������findFreeblock���棬�����ȿ��ʱ�򣬷���û�п���ã��͵ðѲ�����Ҫ���
	//Ǩ�ƻ�ȥ�������ҪǨ�صĿ�ֱ�Ӵ����������ң�д��Ƶ����͵��ȿ�Ǩ��������оƬ
	//�ȶ��ĸ�оƬ��������ӳ�����߼����д�������������
	//3dоƬ�������vector<int> hotdata_to_2d���������������garbage_collection_3dto2d
	//�е���garbage_collection_2dto3d
	//�ȴ�3d����һ��
	position new_p_3d = chips3d[cn]->find_free_block(PRIMARYBLOCK);

	list<position> old_list_2d;
	vector<  pair<int, list<position> > >::iterator iter = hot_data_address_mapping_table[cn].begin();
	//�ҵ�����������С�Ŀ��λ��
	while (iter != hot_data_address_mapping_table[cn].end()) {
		if (iter->first == min_lba) {
			old_list_2d = iter->second;
			break;
		}
	}

	//������Ч���ݵ��¿�new_p,�ȸ��ƻ�����
	copyValidDataFrom2Dto3D(cn, new_p_3d, old_list_2d);
	//�����ɿ�

	//�޸�ӳ���,���޸�2D�ϵ�
	hot_data_address_mapping_table[cn].erase(iter);
	//���޸�3D�ϵ�
	vector<int>::iterator iter_vec = find(chips3d[cn]->hotdata_to_2d.begin(), chips3d[cn]->hotdata_to_2d.end(), min_lba);
	chips3d[cn]->hotdata_to_2d.erase(iter_vec);
	//ҪǨ�ص�оƬӳ���
	//�½�����ӳ������
	mapping_block_list mbl;
	//����ӳ����������Ϊ�գ��յ�ʱ����Ҫ�һ�����
	mbl.mapping_pair.first.push_back(new_p_3d);
	mbl.mapping_pair.second.clear();
	// find_free_block ����Ҫɾ���Ѿ����õ���
	chips3d[cn]->address_mapping_table[min_lba] = mbl;
}
