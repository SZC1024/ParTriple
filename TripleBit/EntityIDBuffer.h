/*
 * EntityIDBuffer.h
 *
 *  Created on: May 21, 2010
 *      Author: liupu
 */

#ifndef ENTITYIDBUFFER_H_
#define ENTITYIDBUFFER_H_

class MemoryBuffer;
class EntityIDBuffer;

#include "TripleBit.h"
#include "ThreadPool.h"

class SortTask
{
public:
	SortTask() {}
	virtual ~SortTask() {}
	static int Run(ID* p, size_t length, int sortKey, int IDCount);
	static int qcompareInt(const void* a, const void* b);
	static int qcompareLongByFirst32(const void* a, const void* b);
	static int qcompareLongBySecond32(const void* a, const void* b);

	static bool compareInt(ID a, ID b);
	static bool compareLongByFirst32(int64_t a, int64_t b);
	static bool compareLongBySecond32(int64_t a, int64_t b);
};

class EntityIDBuffer {
public:
    pair<ID,ID> min_max[2];
	int	IDCount;		//模式中变量的个数,也就是一行有多少列数据
	ID* buffer;
	ID* p;
	int	pos;
	size_t usedSize;	//已经存储的ID个数
	size_t totalSize;	//当前页数*sizePerPage
	int sizePerPage;	//每一个page能存放的ID数量

	//used to sort
    // 表示按照第一个ID还是第二个ID排序
    // 即按照subject还是object排序
	int sortKey;
	bool sorted;
	bool firstTime;
	bool unique;
	//used to hash join;
	vector<int> hashBucket;
	vector<int> prefixSum;
public:
	friend class SortTask;
	friend class HashJoin;
	friend class SortMergeJoin;
	friend class ScanMemory;

	bool contains(ID id){
		size_t total = getSize();
		size_t i;

		ID* p = buffer;

		int k;

		for( i = 0 ; i != total; i++)
		{
			for( k = 0; k < IDCount; k++)
			{
				// cout<<p[i * IDCount + k]<<" ";
				if(k == sortKey){
					if(p[i * IDCount + k] == id)
					return true;
				}
				// other->insertID(p[i * IDCount + k]);
			}
			// cout<<endl;
		}
		return false;


	}

	Status insertID(ID id);
	Status getID(ID& id, size_t _pos);
	void   empty() {
		hashBucket.clear();
		prefixSum.clear();

		sorted = true;
		firstTime = true;

		p = buffer;
		usedSize = 0;
		pos = 0;
	}

	void print2ID(EntityIDBuffer *other){
		size_t total = getSize();
		size_t i;

		ID* p = buffer;

		int k;

		for( i = 0 ; i != total; i++)
		{
			for( k = 0; k < IDCount; k++)
			{
				// cout<<p[i * IDCount + k]<<" ";
				other->insertID(p[i * IDCount + k]);
			}
			// cout<<endl;
		}


	}

	


	void sig(){
		size_t total = getSize();
		cout<<"---buffer sig---"<<endl;
		cout<<"rows : "<<getSize()<<endl;
		cout<<"cols : "<<IDCount<<endl;
		cout<<"sorted : "<<sorted<<endl;
		cout<<"sortKey : "<<sortKey<<endl;
		int k;
		if(total!=0){
			cout<<"first row : ";
			for( k = 0; k < IDCount; k++)
			{
				cout<<buffer[0 * IDCount + k]<<" ";
			}
			cout<<endl;
			cout<<"last row : ";
			for( k = 0; k < IDCount; k++)
			{
				cout<<buffer[(total-1) * IDCount + k]<<" ";
			}
			cout<<endl;
		}
	}

	void   setSize(size_t size) {
		usedSize = size * IDCount;
	}

	size_t getUsedSize() const  { return usedSize; }

	void   setIDCount(int c) { IDCount = c;}
	size_t	   getSize() const {
		if(IDCount != 0) return usedSize / IDCount;
		else return 0;
	}

	size_t    getCapacity() const {
		return totalSize;
	}

	int	   getTotalPerChunk() const {
		return sizePerPage  / IDCount;
	}

	int	   getIDCount() const { return IDCount; }

	Status sort(int _sortKey) {
		if ( ( _sortKey - 1) == sortKey && sorted == true ){
			return OK;
		}

		sortKey = _sortKey - 1;
		return sort();
	}
	Status sort();

	size_t    getEntityIDPos(ID id);
	ID* getBuffer() const {
		return buffer;
	}

	Status mergeIntersection( EntityIDBuffer* entBuffer, char* flags, ID joinKey);
	Status intersection( EntityIDBuffer* entBuffer, char* flags, ID joinKey1, ID joinKey2);
	Status mergeBuffer(EntityIDBuffer* XTemp, EntityIDBuffer* XYTemp);
	static Status mergeSingleBuffer(EntityIDBuffer* entbuffer , ID* buffer1, ID* buffer, size_t size1, size_t size2);
	static Status mergeSingleBuffer(EntityIDBuffer*  ,EntityIDBuffer* ,EntityIDBuffer* );
	static Status mergeSingleBuffer(EntityIDBuffer*  ,EntityIDBuffer*);
//	void   uniqe();
	Status mergeBuffer(ID* destBuffer, ID* buffer1, ID* buffer, size_t size1, size_t size2, int IDCount, int key);
	Status modifyByFlag( char* flags, int no);
	void   getMinMax(ID& min, ID& max);
	void resize(size_t size);
	void resizeForSortMergeJoin(size_t totalSize);
	inline void   setSortKey(int _sortKey) {
		if ( firstTime == true) {
			sortKey = _sortKey;
			firstTime = false;
			return;
		}
		sortKey = _sortKey;
		sorted = false;
	}
	int    getSortKey() const { return sortKey; }
	bool   isInBuffer(ID res[]);
	void   print();
	ID    getMaxID();
	void initialize(int pageCount = 1);
	EntityIDBuffer();
	virtual ~EntityIDBuffer();

	ID& operator[] (const size_t );
	Status appendBuffer(const ID* buffer, size_t size);
	Status appendBuffer1(const ID* buffer, size_t size);
	Status appendBuffer(const EntityIDBuffer* otherBuffer);
	static Status swapBuffer(EntityIDBuffer* &buffer1,EntityIDBuffer* &buffer2);
private:
	int    partition(ID* p, int first, int last);
	void   quickSort(ID*p, int first, int last);
	void   quickSort1(ID*p, int first, int last);
	void   quickSort(ID* p, int size);
	void   merge(int start1, int end1, int start2, int end2, ID* tempBuffer);
	void   swapBuffer(ID*& tempBuffer);
	EntityIDBuffer(const EntityIDBuffer* otherBuffer);
	EntityIDBuffer* operator=(const EntityIDBuffer* otherBuffer);
};

#endif /* ENTITYIDBUFFER_H_ */
