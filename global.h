#define _CRT_SECURE_NO_WARNINGS
#include <map>
#include <set>
#include <list>
#include <cmath>
#include <queue>
#include <vector>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <memory>
#include <assert.h>
#include <sstream>
#include<numeric>


using namespace std;
//一个逻辑块对应物理块的数量
const int loglength = 3;
//free block阈值
const double FB_threshold = 0.85;
//cache大小
const int cache_threshold = 1;
//页或者块状态
const int FREE=0;
const int USED=1;
const int INVALID = -1;
const int VALID = 1;

const int PRIMARYBLOCK = -1;
const int REPLACEBLOCK = -2;

const int chip_num = 1;
//16层，64行，128列，64页，4k，单片总大小32G
const int layer_num = 16;
const int row_num = 64;
const int column_num = 128;
const int page_per_block = 64;
const int least_free_block = layer_num * row_num * column_num * (1-FB_threshold);
const int free_page_in_block_limit = 32;
//256行256列，16页，每页1k，单片总大小4G，这样总容量就相当于3d芯片的1/8
const int row_num_2d = 256;
const int column_num_2d = 256;
//先把它改成和3D一样的块大小
const int page_per_block_2d = 16;
//重要数据约占4%，要控制2D芯片重要数据的比例在一半以下。这样才能有空间给来自3D的数据。
//const int important_data_limit = layer_num*row_num*column_num*page_per_block*0.8*0.05;
const int rec_threshold_3d = layer_num * row_num * column_num * 0.10 ;
const int rec_threshold_2d =  row_num_2d * column_num_2d * 0.10;
//热数据在2D上最多占一半
const int hotblock_in2d_limit = row_num_2d*column_num_2d * 0.5;
//根据LPN页数来计算最多能放下这么多来自3D的block
const int hotblock_in2d_num = hotblock_in2d_limit / 64 - 2;
//这样重要数据只能占所有块的40%，也就是lbn从0到
const int important_data_limit = row_num_2d*column_num_2d *1/4*page_per_block_2d;
//读写擦除带来的温度改变，读0.1度，写0.3度
const double read_tmp = 0.1;
const double write_tmp = 0.3;
const double erase_tmp = 12.0;
//读写擦除时间
const double read_time = 0.04;
const double write_time = 0.1;
const double erase_time = 1;
//散热的设置
const double percent_of_zy_dissipation = 0.08;
const double percent_of_sx_dissipation = 0.02;
const double percent_of_qh_dissipation = 0.04;
//散热间隔
const double dissipation_temp_time = 4000;
//错误率时间设置
const double make_errors_time = 900;
const double correct_time = 920;
const double scan_temp_time = 17000;
//全局变量，控制所有芯片的散热时间，错误率信息，扫描时间，在initial.cpp中初始化
extern double now_dissipation_temp_time;
extern double now_make_errors_time;
extern double now_correct_time;
extern double now_scan_time;
extern double total_Time;
extern int scan_count;
extern int scan_temperature_gap;
extern int cnt_temperature_gap;
extern int scan_time_gap;
extern int cnt_time_gap;


//温度，错误率相关参数，initial.cpp中初始化
extern int _pow[1000];
extern double temp_mapping_temp[26] ;
extern int block_err[layer_num][row_num][column_num];
//纠错相关参数
const int ecc_strength = 40;
const int temp_error_length = 26;
const double temp_gap = 4.0;
const int temp_mapping_error[26] = {0,0,4,4,9,10,18,28,30,40,48,55,68,74,92,95,110,125,136,148,160,183,200,216,231,250};


//每次扫描需要保存的相关参数
extern vector<int> biterrors;
extern vector<int> errors_record;
extern vector<double> peak_temperature;
extern vector<double> average_temperture;


extern char infile[100];
extern char outfile[100];
extern char prefix[100];


//控制迁移的量
const int GC_count_gap_limit = 100;

#ifndef _INFO_PAGE
#define _INFO_PAGE
struct Info_Page
{
	//LPN是芯片内LPN
    int LPN;
    //保存数据，写入1，读取时，如果数据不是1证明错误，页无效数据改为-1，擦除后为0
    int data;
    //页的状态，invalid: -1 free: 0 valid: 1
    int status;
    //记录错误数目
    int ecc; 

	//LBN是局部的,后面需要把大块打碎，同一块内LPN可能是不连续的。
    int LBN;
    /*块的类型
        unused = -1 
        primary_block = 1 
        replaced_block = 0
    */
    int isprimary;  

    Info_Page(int d,int l,int e,int s):data(d),LPN(l),ecc(e),status(s) {}
    Info_Page()
    {
        LPN = data = -1;
        LBN = isprimary = -1;
        ecc = 100;      // ecc 是可以改变的, 为了统一， 要改成赋值为ecc_strength
        status = 0;
    }
    void clean()
    {
        LPN = data = -1;
        LBN = isprimary = -1;
        ecc=100, status=0;
    }
	Info_Page& operator =(const Info_Page& ip)
	{
		this->LPN = ip.LPN;
		this->data = ip.data;
		this->ecc = ip.data;
		this->status = ip.status;
		return *this;
	}
};
#endif // _INFO_PAGE

#ifndef _INFO_BLOCK
#define _INFO_BLOCK
struct Info_Block
{
    Info_Page ip[page_per_block];
	//计算块的ls值，范围是0到n
	int ls_val;
    int status ;  // status = 0 free -- status = 1 used
    int erase_count ;//擦除次数
    /*这个迁移和空间不足选择回收块的时候用
    
    */
    int free_page_num ;
    int valid_page_num ;
    //有效页数量，擦除的时候用，基本块如果有效页使用不超过50%，则加入reused块链表
    //再分配后当作替换块使用。

    Info_Block()
    {
        free_page_num = 1024;
        valid_page_num = 0 ;
        erase_count = 0 ;
        status = 0 ;
		ls_val = -1 ;
    }

};
#endif // _INFO_BLOCK


#ifndef _INFO_BLOCK_2D
#define _INFO_BLOCK_2D
struct Info_Block_2D
{
    Info_Page ip[page_per_block_2d];
	//计算块的ls值，范围是0到n
    int status ;  // status = 0 free -- status = 1 used
    int erase_count ;//擦除次数
    /*这个迁移和空间不足选择回收块的时候用
    
    */

    //有效页数量，擦除的时候用，基本块如果有效页使用不超过50%，则加入reused块链表
    //再分配后当作替换块使用。

    Info_Block_2D()
    {

        erase_count = 0 ;
        status = 0 ;
    }

};
#endif // _INFO_BLOCK

#ifndef LOGIC_BLOCK
#define LOGIC_BLOCK
struct Logic_Block{
    int lba;
    int update_count;


    Logic_Block(){
        lba = 0;
        update_count = 0;

    }
};
#endif

#ifndef _CACHE_CELL
#define _CACHE_CELL
struct cache_cell
{
    //芯片内的LBN（局部）
    int data;
    //总的LPN
    int LPN;

    cache_cell(int LPN,int data,int visit_count=1)
    {
        this->LPN = LPN;
        this->data = data;

    }
	cache_cell(int LPN) {
		this->LPN = LPN;
	}
    cache_cell()
    {
        data = -1;
		LPN = -1;
    }
    bool operator==(const cache_cell tmp) const
    {
        if( LPN==tmp.LPN)
            return 1;
        return 0;
    }
};
#endif // _CACHE_CELL
extern list<cache_cell> wr_cache;

#ifndef _POSITION
#define _POSITION
struct position
{
    int layer, x, y;
    position()
    {
        layer = x = y = -1;
    }
    position(int ll,int xx,int yy):layer(ll),x(xx),y(yy) {}

    bool operator<(const position b) const
    {
        return (layer<b.layer) || (layer==b.layer && x<b.x) || (layer==b.layer && x==b.x && y<b.y);
    }
};
#endif // _POSITION

#ifndef _POS_DELAY
#define _POS_DELAY
struct pos_delay{
    //存放延迟擦除的替换块-新块位置对
    pair<position,position> p;
	//LBN存放延迟擦除替换块的逻辑地址，以便在垃圾回收的时候擦除延迟块后，修改对应的地址映射表中第二个延迟映射
	int LBN;
    pos_delay(){
        p.first = p.second = position(-1,-1,-1);
		LBN = -1;
    }

	bool operator == (const pos_delay& p_d)const {
		if (p.first.x == p_d.p.first.x) {
			return 1;
		}
		else
			return 0;
	}
};
#endif

struct mapping_block_list{
    //第一个是正常映射，第二个是延迟块映射
    pair<list<position>,list<pos_delay> > mapping_pair;
 
    mapping_block_list(){
        mapping_pair.first.clear();
        mapping_pair.second.clear();
    }

};
/*
#ifndef _BLOCK_LIST
#define _BLOCK_LIST
struct block_list
{
    //管理块的一种数据结构
    list<position> mapping_blocks;
    block_list()
    {
        mapping_blocks.clear();
    }

    block_list(const block_list& tmp)
    {
    }

    int size(){
        return mapping_blocks.size();
    }
};
#endif // _BLOCK_LIST
*/
struct ls {
	position pos;
	int flag;
	ls() {
		flag = 0;
	}
};

struct Mols {
	//遍历用的指针,ls_p是二维数组，包含ls个数*层数个指针，每个指针指向单个ls在每一层第一行的位置
	ls **ls_p;
	//统计各个ls_val数目
	int *count;

	int ls_val, max_ls_val;

	Mols() {
		ls_val = 0;
		max_ls_val = 0;
	}
};

class chip {
public:
	//用来记录芯片号
	char LCN[1];
	//可用块的数目
	int free_block_num;
    int used_block_num;
	
};


class chip_3d:public chip{
	//继承chip的成员
public:
    //统计GC擦除次数，为迁移做准备
    int GC_count;
    //控制间隔
    int GC_count_gap;
	//mols遍历指针
	Mols mols;
    /*
	  可用LBN到PBN映射,这里一个LBN如果被打碎，则分散到不同的PBN，
      需要用block_list来记录不同PBN的位置
      将基本块和替换块都放到blocklist，block中的标记位来决定该块是
        基本块
        替换块1
        替换块2
        新块
    */
    //映射表：pair第一个是正常的映射，第二个是延迟的块映射
	map<int, mapping_block_list > address_mapping_table;
	//第一个是擦除的逻辑快lba，第二个int是对应的擦除次数
	map<int, int> lba_gctimes_map;
    //基本块如果可以重新利用则加入该链表，暂时不擦除，找基本块新块时从该链表返回
    list<position> reused_primary_block;
	//延迟擦除的块，按照一定顺序延迟擦除该链表的块
    list<pos_delay> delay_copy_block;
	//遍历延迟擦除块的指针
    list<pos_delay>::iterator d_erase_p;
	//配合延迟擦除链表指针使用，记录延迟擦除块的数目，到达一定数目指针回到链表头
    int d_erase_count;
    //要延迟GC的基本块和替换块加入该链表
    //list<position> gc_block; 
    //记录迁移到2D上的热数据逻辑地址，这个用2D上的不太方便，因为2D记录热数据用了chip_num个链表，所以如果2d和3d之间有转移的话，需要同步
	//更新这个链表
    vector<int> hotdata_to_2d;

	//分别记录块的管理信息，错误信息，温度信息。
	Info_Block info_block[layer_num][row_num][column_num];
	int block_err[layer_num][row_num][column_num];
	double block_temp[layer_num][row_num][column_num];
	//记录芯片的温度信息
	//记录芯片的错误位
	vector<int> biterrors;
	vector<int> errors_record;
	vector<double> peak_temperature;
	vector<double> average_temperture;
	
public:
	//定义在globa.cpp
	chip_3d();
    //写入基本块，写入成功返回1
	int write1(position PBN, int LPN, int val);
	//写入替换块，成功返回1
	int write2(position PBN, int LPN, int val);
    int Write3dFlash(int LPN,cache_cell val);
	//返回lba，失败返回-1
    int computeVar(map<int,mapping_block_list> addr_map_table);
	//清理最近一段时间的逻辑块lba更新次数
    void clearLBA_count(map<int,mapping_block_list> addr_map_table);
	//返回垃圾回收要擦除的lba
	int find_erase_lba();
	//读基本块
	int read1(position PBNX, int LPN, Info_Page &val);
	//读替换块
	int read2(position PBNX, int LPN, Info_Page &val);
    int Read3dFlash(int LPN_in_chip, Info_Page &val);

	//如果要找的是替换块，可以先看看可重用的基本块链表
    position find_free_block(int p_or_r);
	//mols的实现，返回新块位置
	position find_free_block1();

    void scan_avg_peak_temp();
    void scan_and_correct();
    int find_err_num(double temp_val);
    void make_errors();
	//改变块的温度
	void change_temperature(char op, position aim);

    int check_border(int lay, int x, int y);
    int equ(double val);
    void temp_spread(int cl, int cx, int cy, int ty);
    void dissipation_temp ();
	void build_mols(Mols &mols);
    //垃圾回收
    int checkPrimaryBlock(position p);
    int copyValidData(char block_class,position oldblock,position newblock);
    int garbage_collection(int LBN,mapping_block_list* mbl, position newblock);
    int eraseBlock(position p);
    void checkVDinDelayBlock(int LPN_in_chip, list<pos_delay> delay_rpl_block);
    int findAndErase(list<pos_delay> *d_c_b,pos_delay p_d);
    int garbage_collection_3dto2d(mapping_block_list* mbl,list<position> p_list,int c_num,int lbn);

    //迁移
    void copyValidDataFrom3Dto2D(char p_or_r,position r_old,list<position> new_p_list);

};

class chip_2d:public chip {
	//继承chip的成员
public:
	//添加自己的成员
	Info_Block_2D info_block_2d[1][row_num_2d][column_num_2d];
	int block_err[1][row_num_2d][column_num_2d];
	list<position> free_block;
    //地址映射表统一，对于重要数据，一对二映射
    //对于来自3D的热数据，一对4或者8映射,每个芯片都有对应的映射表，分别管理
    vector<pair<int,list<position> > >  hot_data_address_mapping_table[chip_num];
	map<int, pair<position, position> > address_mapping_table;
    

public:
	chip_2d() {
		free_block_num = row_num_2d * column_num_2d;
		int i, j, k;
		for (i = 0; i<1; i++)
		{
			for (j = 0; j<row_num_2d; j++)
			{
				for (k = 0; k<column_num_2d; k++)
				{
					//block_temp[i][j][k] = 25.0;
					free_block.push_back(position(i, j, k));
				}
			}
		}
	}
	int write1(position PBN, int LPN, int val);
	int write2(position PBN, int LPN, int val);
	int write_2dflash(cache_cell val);
    int write_hotdata_2dflash(int data,int LBN,int chip_n);
    void garbage_collection_hotdata(vector<position> pbnx,vector<position > pbny,vector<position > pbnz);
    void copyValidDataFrom2Dto3D(int cn,position newblock_3d,list<position> old_list_2d);
    void garbage_collection_2dto3d(int cn,int lbn);

	int read1(position PBNX, int LPN, Info_Page &val);
	int read2(position PBNX, int LPN, Info_Page &val);
	int read_2dflash(int LPN, Info_Page &val);
	int read_hotdata_2dflash(Info_Page& val, int LPN_in_chip, int LCN);

	position find_free_block();
	int garbage_collection(position PBNX, position PBNY, position PBNZ);
	//与change_temperature作用类似

};
void change_time(char op);
//定义在global.h的全局变量
extern chip_2d* chip2d;
extern chip_3d* chips3d[chip_num];
int read_cache(int LPN,Info_Page &val);
int write_cache(int LPN, int val);
void initial();
int first_prime(int m);
int isPrime(int n);
void output_overall(chip_3d* chips);