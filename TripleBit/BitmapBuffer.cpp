/*
 * ChunkManager.cpp
 *
 *  Created on: 2010-4-12
 *      Author: root
 */

#include "MemoryBuffer.h"
#include "BitmapBuffer.h"
#include "MMapBuffer.h"
#include "TempFile.h"
#include "TempMMapBuffer.h"

unsigned int ChunkManager::bufferCount = 0;

//#define WORD_ALIGN 1

BitmapBuffer::BitmapBuffer(const string _dir) :
	dir(_dir) {
	startColID = 1;
	string filename(dir);
	filename.append("/temp1");
	// init file size 4MB
	temp1 = new MMapBuffer(filename.c_str(), INIT_PAGE_COUNT * MemoryBuffer::pagesize);

	filename.assign(dir.begin(), dir.end());
	filename.append("/temp2");
	temp2 = new MMapBuffer(filename.c_str(), INIT_PAGE_COUNT * MemoryBuffer::pagesize);

	filename.assign(dir.begin(), dir.end());
	filename.append("/temp3");
	temp3 = new MMapBuffer(filename.c_str(), INIT_PAGE_COUNT * MemoryBuffer::pagesize);

	filename.assign(dir.begin(), dir.end());
	filename.append("/temp4");
	temp4 = new MMapBuffer(filename.c_str(), INIT_PAGE_COUNT * MemoryBuffer::pagesize);

    filename.assign(dir.begin(), dir.end());
    filename.append("/temp5");
    temp5 = new MMapBuffer(filename.c_str(), INIT_PAGE_COUNT * MemoryBuffer::pagesize);

    filename.assign(dir.begin(), dir.end());
    filename.append("/temp6");
    temp6 = new MMapBuffer(filename.c_str(), INIT_PAGE_COUNT * MemoryBuffer::pagesize);

	usedPage1 = usedPage2 = usedPage3 = usedPage4 = usedPage5 = usedPage6 = 0;
}

BitmapBuffer::~BitmapBuffer() {
	for (map<ID, ChunkManager*>::iterator iter = predicate_managers[0].begin(); iter != predicate_managers[0].end(); iter++) {
		if (iter->second != 0) {
			delete iter->second;
			iter->second = NULL;
		}
	}

	for (map<ID, ChunkManager*>::iterator iter = predicate_managers[1].begin(); iter != predicate_managers[1].end(); iter++) {
		if (iter->second != 0) {
			delete iter->second;
			iter->second = NULL;
		}
	}
}

Status BitmapBuffer::insertPredicate(ID id, unsigned char type) {
	predicate_managers[type][id] = new ChunkManager(id, type, this);
	return OK;
}

size_t BitmapBuffer::getTripleCount() {
	size_t tripleCount = 0;
	map<ID, ChunkManager*>::iterator begin, limit;
	for (begin = predicate_managers[0].begin(), limit = predicate_managers[0].end(); begin != limit; begin++) {
		tripleCount = tripleCount + begin->second->getTripleCount();
	}

	tripleCount = 0;
	for (begin = predicate_managers[1].begin(), limit = predicate_managers[1].end(); begin != limit; begin++) {
		tripleCount = tripleCount + begin->second->getTripleCount();
	}

	return tripleCount;
}

/*
 *	@param id: predicate id
 *       type: 0 means so, 1 means os
 *	@param id: the chunk manager id ( predicate id );
 *       type: the predicate_manager type;
 */
ChunkManager* BitmapBuffer::getChunkManager(ID id, unsigned char type) {
	//there is no predicate_managers[id]
	if (!predicate_managers[type].count(id)) {
		//the first time to insert
		insertPredicate(id, type);
	}
	return predicate_managers[type][id];
}

/*
 *	@param f: 0 for triple being sorted by subject; 1 for triple being sorted by object
 *         flag: objType;
 */
// TODO: this function may need to be modified if the way id coded changed
Status BitmapBuffer::insertTriple(ID predicateID, ID xID, Element yID, unsigned objType, unsigned char typeID) {
	unsigned char len;

	len = sizeof(xID);
	switch (objType % objTypeNum){
        case 0:
            len += sizeof(yID.id);
            break;
        case 1:
            len += sizeof(yID.f);
            break;
        default:len += sizeof(yID.d);
	}

	getChunkManager(predicateID, typeID)->insertXY(xID, yID, len, objType);

	//	cout<<getChunkManager(1, 0)->meta->length[0]<<" "<<getChunkManager(1, 0)->meta->tripleCount[0]<<endl;
	return OK;
}

void BitmapBuffer::flush() {
	temp1->flush();
	temp2->flush();
	temp3->flush();
	temp4->flush();
	temp5->flush();
	temp6->flush();
}

void BitmapBuffer::generateXY(ID& subjectID, ID& objectID) {
	ID temp;

	if (subjectID > objectID) {
		temp = subjectID;
		subjectID = objectID;
		objectID = temp - objectID;
	} else {
		objectID = objectID - subjectID;
	}
}

unsigned char BitmapBuffer::getBytes(ID id) {
	if (id <= 0xFF) {
		return 1;
	} else if (id <= 0xFFFF) {
		return 2;
	} else if (id <= 0xFFFFFF) {
		return 3;
	} else if (id <= 0xFFFFFFFF) {
		return 4;
	} else {
		return 0;
	}
}

/**
 * get page number by type and flag
 * @param type triples stored by so or os
 * @param flag objType
 * @param pageNo return by reference
 * @return the first unused page address
 */
 // TODO: temp and usedPage
char* BitmapBuffer::getPage(unsigned char type, unsigned char flag, size_t& pageNo) {
	char* rt;
	bool tempresize = false;

	//cout<<__FUNCTION__<<" begin"<<endl;

	if (type == 0) {  //如果按S排序
		if (flag == 0) { //objType = string
			if (usedPage1 * MemoryBuffer::pagesize >= temp1->getSize()) {
                // expand 1024*pagesize = 4MB when usedPage1 increment to limit
				temp1->resize(INCREMENT_PAGE_COUNT * MemoryBuffer::pagesize);
				tempresize = true;
			}
			pageNo = usedPage1;
			rt = temp1->get_address() + usedPage1 * MemoryBuffer::pagesize;
			usedPage1++;
		}
		else if (flag == 1){ //objType = float
			if (usedPage2 * MemoryBuffer::pagesize >= temp2->getSize()) {
				temp2->resize(INCREMENT_PAGE_COUNT * MemoryBuffer::pagesize);
				tempresize = true;
			}
			pageNo = usedPage2;
			rt = temp2->get_address() + usedPage2 * MemoryBuffer::pagesize;
			usedPage2++;
		} else{ // objType = double
            if (usedPage3 * MemoryBuffer::pagesize >= temp3->getSize()) {
                temp3->resize(INCREMENT_PAGE_COUNT * MemoryBuffer::pagesize);
                tempresize = true;
            }
            pageNo = usedPage3;
            rt = temp3->get_address() + usedPage3 * MemoryBuffer::pagesize;
            usedPage3++;
		}

	} 
	else {   //按O排序
		if (flag == 0) {
            if (usedPage4 * MemoryBuffer::pagesize >= temp4->getSize()) {
                temp4->resize(INCREMENT_PAGE_COUNT * MemoryBuffer::pagesize);
                tempresize = true;
            }
            pageNo = usedPage4;
            rt = temp4->get_address() + usedPage4 * MemoryBuffer::pagesize;
            usedPage4++;
		} else if (flag == 1){
            if (usedPage5 * MemoryBuffer::pagesize >= temp5->getSize()) {
                temp5->resize(INCREMENT_PAGE_COUNT * MemoryBuffer::pagesize);
                tempresize = true;
            }
            pageNo = usedPage5;
            rt = temp5->get_address() + usedPage5 * MemoryBuffer::pagesize;
            usedPage5++;
		} else {
            if (usedPage6 * MemoryBuffer::pagesize >= temp6->getSize()) {
                temp6->resize(INCREMENT_PAGE_COUNT * MemoryBuffer::pagesize);
                tempresize = true;
            }
            pageNo = usedPage6;
            rt = temp6->get_address() + usedPage6 * MemoryBuffer::pagesize;
            usedPage6++;
		};
	}

	if (tempresize == true) {
	    // tempx was expanded and update chunkManger
		if (type == 0) {
            map<ID, ChunkManager*>::iterator iter, limit;
            iter = predicate_managers[0].begin();
            limit = predicate_managers[0].end();
            MMapBuffer *temp = NULL;
            switch (flag % objTypeNum){
                case 0:
                    temp = temp1;
                    break;
                case 1:
                    temp = temp2;
                    break;
                default:temp = temp3;
            }
            for (; iter != limit; iter++) {
                if (iter->second == NULL)
                    continue;
                iter->second->meta = (ChunkManagerMeta*) (temp->get_address()
                        + iter->second->usedPage[flag][0]/*page num where ChunkManagerMeta stored*/
                        * MemoryBuffer::pagesize);
                if (iter->second->usedPage[flag].size() == 1) {
                    iter->second->meta->endPtr[flag] = temp->get_address()
                            + iter->second->usedPage[flag].back() * MemoryBuffer::pagesize
                            + MemoryBuffer::pagesize
                            - (iter->second->meta->length[flag] - iter->second->meta->usedSpace[flag] - sizeof(ChunkManagerMeta));
                } else {
                    iter->second->meta->endPtr[flag] = temp->get_address()
                            + iter->second->usedPage[flag].back() * MemoryBuffer::pagesize
                            + MemoryBuffer::pagesize
                            - (iter->second->meta->length[flag] - iter->second->meta->usedSpace[flag] - sizeof(ChunkManagerMeta));
                }
            }
		} else if (type == 1) {
            map<ID, ChunkManager*>::iterator iter, limit;
            iter = predicate_managers[1].begin();
            limit = predicate_managers[1].end();
            MMapBuffer *temp = NULL;
            switch (flag % objTypeNum){
                case 0:
                    temp = temp4;
                    break;
                case 1:
                    temp = temp5;
                    break;
                default:
                    temp = temp6;
            }
            for (; iter != limit; iter++) {
                if (iter->second == NULL)
                    continue;
                iter->second->meta = (ChunkManagerMeta*) (temp->get_address()
                                                          + iter->second->usedPage[flag][0]/*page num where ChunkManagerMeta stored*/
                                                            * MemoryBuffer::pagesize);
                if (iter->second->usedPage[flag].size() == 1) {
                    iter->second->meta->endPtr[flag] = temp->get_address()
                           + iter->second->usedPage[flag].back() * MemoryBuffer::pagesize
                           + MemoryBuffer::pagesize
                           - (iter->second->meta->length[flag] - iter->second->meta->usedSpace[flag] - sizeof(ChunkManagerMeta));
                } else {
                    iter->second->meta->endPtr[flag] = temp->get_address()
                           + iter->second->usedPage[flag].back() * MemoryBuffer::pagesize
                           + MemoryBuffer::pagesize
                           - (iter->second->meta->length[flag] - iter->second->meta->usedSpace[flag] - sizeof(ChunkManagerMeta));
                }
            }
	    }
	}

	//cout<<__FUNCTION__<<" end"<<endl;

	return rt;
}

// TODO: need to be changed
unsigned char BitmapBuffer::getLen(ID id) {
		return sizeof(id);
}

// TODO: need to be changed
void BitmapBuffer::save() {
	string filename = dir + "/BitmapBuffer";
	string predicateFile(filename.append("_predicate"));

	MMapBuffer *predicateBuffer = new MMapBuffer(predicateFile.c_str(),
	        predicate_managers[0].size() * (sizeof(ID) + sizeof(SOType) + sizeof(size_t) * 2) * 2);

	size_t bufferPageNum = usedPage1 + usedPage2 + usedPage3
	        + usedPage4 + usedPage5 + usedPage6;
    // Created by peng on 2019-04-19 15:25:51.
    // directly alloc all the memory that the buffer need.
    MMapBuffer *buffer = new MMapBuffer(filename.c_str(), bufferPageNum * MemoryBuffer::pagesize);

    char *predicateWriter = predicateBuffer->get_address();
    char *bufferWriter = buffer->get_address();
    size_t chunkManagerOffset = 0;
    for (SOType i = 0; i < 2; ++i) {
        const map<ID, ChunkManager*>& map = predicate_managers[i];
        for (auto& entry: map) {
            // Created by peng on 2019-04-19 15:40:30.
            // firstly save predicate info
            *(ID*)predicateWriter = entry.first;
            predicateWriter += sizeof(ID);
            // i indicate order type(so or os)
            *(SOType*)predicateWriter = i;
            predicateWriter += sizeof(SOType);
            *(size_t*)predicateWriter = chunkManagerOffset;
            predicateWriter += sizeof(size_t) * 2;
            size_t increment = entry.second->save(bufferWriter, i) * MemoryBuffer::pagesize;
            chunkManagerOffset += increment;
            bufferWriter += increment;
        }
    }

	buffer->flush();
	predicateBuffer->flush();

	//这里之前有个疑惑就是temp1-4的buffer在discard之后ChunckManager中的ChunckManagerMeta中startPtr和endPtr
	//的指向问题,也就是ChunckManagerMeta最终指向的内存地址是什么,下面425-428行对指针重新定位
	//以S排序的关联矩阵的metadata计算
	predicateWriter = predicateBuffer->get_address();
	ID id;
    for (int i = 0; i < 2; ++i) {
        const map<ID, ChunkManager*>& map = predicate_managers[i];
        for (auto& entry : map) {
            id = *((ID*) predicateWriter);
            assert(entry.first == id);
            chunkManagerOffset = *(size_t*)(predicateWriter + sizeof(ID) + sizeof(SOType));
            char *base = buffer->get_address() + chunkManagerOffset;
            entry.second->meta = (ChunkManagerMeta*)base;
            ChunkManagerMeta* meta = entry.second->meta;
            for (int j = 0; j < objTypeNum; ++j) {
                meta->startPtr[j] = base + (j==0?sizeof(ChunkManagerMeta):0);
                meta->endPtr[j] = meta->startPtr[j] + meta->usedSpace[j];
                base += meta->length[j];
                MetaData* metaData;
                if (meta->usedSpace[j] + (j==0?sizeof(ChunkManagerMeta):0) <= MemoryBuffer::pagesize){
                    metaData = (MetaData*)meta->startPtr[j];
                    metaData->usedSpace = meta->usedSpace[j];
                } else {
                    size_t usedLastPage = (meta->usedSpace[j] + (j==0?sizeof(ChunkManagerMeta):0))
                            % MemoryBuffer::pagesize;
                    if (usedLastPage == 0){
                        metaData = (MetaData*)(meta->endPtr[j] - MemoryBuffer::pagesize);
                        metaData->usedSpace = MemoryBuffer::pagesize;
                    } else {
                        metaData = (MetaData*)(meta->endPtr[j] - usedLastPage);
                        metaData->usedSpace = usedLastPage;
                    }
                }
            }
        }
    }

	buffer->flush();
    temp1->discard();
    temp2->discard();
	temp3->discard();
	temp4->discard();
	temp5->discard();
	temp6->discard();

	// Created by peng on 2019-04-19 18:18:51.
	// TODO: @youyujie, the code below included in youyujie's work.
	//build index;
	MMapBuffer* bitmapIndex = NULL;
	predicateWriter = predicateBuffer->get_address();
#ifdef MYDEBUG
	cout<<"build hash index for subject"<<endl;
#endif
	//给每个chunckManage后的chunk块创建索引
	for (map<ID, ChunkManager*>::iterator iter = predicate_managers[0].begin(); iter != predicate_managers[0].end(); iter++) {
		if (iter->second) {
#ifdef MYDEBUG
			cout<<iter->first<<endl;
#endif		
			//索引建立2018年11月6日19:27:02
			iter->second->buildChunkIndex();
            chunkManagerOffset = iter->second->getChunkIndex(1)->save(bitmapIndex);
			iter->second->getChunkIndex(2)->save(bitmapIndex);
			predicateWriter = predicateWriter + sizeof(ID) + sizeof(SOType) + sizeof(size_t);
			*((size_t*) predicateWriter) = chunkManagerOffset;
			predicateWriter = predicateWriter + sizeof(size_t);
		}
	}

#ifdef MYDEBUG
	cout<<"build hash index for object"<<endl;
#endif
	for (map<ID, ChunkManager*>::iterator iter = predicate_managers[1].begin(); iter != predicate_managers[1].end(); iter++) {
		if (iter->second) {
#ifdef MYDEBUF
			cout<<iter->first<<endl;
#endif
			iter->second->buildChunkIndex();
            chunkManagerOffset = iter->second->getChunkIndex(1)->save(bitmapIndex);
			iter->second->getChunkIndex(2)->save(bitmapIndex);
			predicateWriter = predicateWriter + sizeof(ID) + sizeof(SOType) + sizeof(size_t);
			*((size_t*) predicateWriter) = chunkManagerOffset;
			predicateWriter = predicateWriter + sizeof(size_t);
		}
	}

	delete bitmapIndex;
	delete buffer;
	delete predicateBuffer;
}

// TODO: need to be changed
BitmapBuffer *BitmapBuffer::load(MMapBuffer* bitmapImage, MMapBuffer*& bitmapIndexImage, MMapBuffer* bitmapPredicateImage) {
	// bitmapImage: file BitmapBuffer
	// bitmapIndexImage: file BitmapBuffer_index
	// bitmapPredicateImage: file BitmapBuffer_predicate
    BitmapBuffer *buffer = new BitmapBuffer();
	char *predicateReader = bitmapPredicateImage->get_address();

	ID id;
	SOType soType;
	size_t offset = 0, indexOffset = 0, predicateOffset = 0;
	size_t sizePredicateBuffer = bitmapPredicateImage->get_length();

	while (predicateOffset < sizePredicateBuffer) {
		id = *((ID*) predicateReader);
		predicateReader += sizeof(ID);
		soType = *((SOType*) predicateReader);
		predicateReader += sizeof(SOType);
		offset = *((size_t*) predicateReader);
		predicateReader += sizeof(size_t);
		//TripleBit/BitmapBuffer.cpp:323: predicateWriter += sizeof(size_t) * 2;
		// second size_t stores indexOffset
		indexOffset = *((size_t*) predicateReader);
		predicateReader += sizeof(size_t);
		// Created by peng on 2019-04-19 18:22:36.
		// TODO: @youyujie, load index is your work.
		if (soType == 0) {
			ChunkManager *manager = ChunkManager::load(id, 0, bitmapImage->get_address(), offset);
			manager->chunkIndex[0] = LineHashIndex::load(*manager, LineHashIndex::SUBJECT_INDEX, LineHashIndex::YBIGTHANX, bitmapIndexImage->get_address(), indexOffset);
			manager->chunkIndex[1] = LineHashIndex::load(*manager, LineHashIndex::SUBJECT_INDEX, LineHashIndex::XBIGTHANY, bitmapIndexImage->get_address(), indexOffset);
			buffer->predicate_managers[0][id] = manager;
		} else if (soType == 1) {
			ChunkManager *manager = ChunkManager::load(id, 1, bitmapImage->get_address(), offset);
			manager->chunkIndex[0] = LineHashIndex::load(*manager, LineHashIndex::OBJECT_INDEX, LineHashIndex::YBIGTHANX, bitmapIndexImage->get_address(), indexOffset);
			manager->chunkIndex[1] = LineHashIndex::load(*manager, LineHashIndex::OBJECT_INDEX, LineHashIndex::XBIGTHANY, bitmapIndexImage->get_address(), indexOffset);
			buffer->predicate_managers[1][id] = manager;
		}
		predicateOffset += sizeof(ID) + sizeof(SOType) + sizeof(size_t) * 2;
	}

	return buffer;
}

// TODO: need to be changed
void BitmapBuffer::endUpdate(MMapBuffer *bitmapPredicateImage, MMapBuffer *bitmapOld) {
	char *predicateReader = bitmapPredicateImage->get_address();

	int offsetId = 0, tableSize = 0;
	char *startPtr, *bufferWriter, *chunkBegin, *chunkManagerBegin, *bufferWriterBegin, *bufferWriterEnd;
	MetaData *metaData = NULL, *metaDataNew = NULL;
	size_t offsetPage = 0, lastoffsetPage = 0;

	ID id = 0;
	SOType soType = 0;
	size_t offset = 0, predicateOffset = 0;
	size_t sizePredicateBuffer = bitmapPredicateImage->get_length();

	string bitmapName = dir + "/BitmapBuffer_Temp";
	MMapBuffer *buffer = new MMapBuffer(bitmapName.c_str(), MemoryBuffer::pagesize);

	while (predicateOffset < sizePredicateBuffer) {
		bufferWriter = buffer->get_address() + offsetPage * MemoryBuffer::pagesize;
		lastoffsetPage = offsetPage;
		bufferWriterBegin = bufferWriter;

		id = *((ID*) predicateReader);
		predicateReader += sizeof(ID);
		soType = *((SOType*) predicateReader);
		predicateReader += sizeof(SOType);
		offset = *((size_t*) predicateReader);
		*((size_t*) predicateReader) = bufferWriterBegin - buffer->get_address();
		predicateReader += sizeof(size_t);
		predicateReader += sizeof(size_t); //skip the indexoffset

		//the part of xyType0
		startPtr = (char*) predicate_managers[soType][id]->getStartPtr(1);
		offsetId = 0;
		tableSize = predicate_managers[soType][id]->getChunkNumber(1);
		metaData = (MetaData*) startPtr;

		chunkBegin = startPtr - sizeof(ChunkManagerMeta);
		chunkManagerBegin = chunkBegin;
		memcpy(bufferWriter, chunkBegin, MemoryBuffer::pagesize);
		offsetPage++;
		metaDataNew = (MetaData*) (bufferWriterBegin + sizeof(ChunkManagerMeta));
		metaDataNew->haveNextPage = false;
		metaDataNew->NextPageNo = 0;

		while (metaData->haveNextPage) {
			chunkBegin = TempMMapBuffer::getInstance().getAddress() + metaData->NextPageNo * MemoryBuffer::pagesize;
			metaData = (MetaData*) chunkBegin;
			if (metaData->usedSpace == sizeof(MetaData))
				break;
			buffer->resize(MemoryBuffer::pagesize);
			bufferWriter = buffer->get_address() + offsetPage * MemoryBuffer::pagesize;
			memcpy(bufferWriter, chunkBegin, MemoryBuffer::pagesize);
			offsetPage++;
			metaDataNew = (MetaData*) bufferWriter;
			metaDataNew->haveNextPage = false;
			metaDataNew->NextPageNo = 0;
		}
		offsetId++;
		while (offsetId < tableSize) {
			buffer->resize(MemoryBuffer::pagesize);
			bufferWriter = buffer->get_address() + offsetPage * MemoryBuffer::pagesize;
			chunkBegin = chunkManagerBegin + offsetId * MemoryBuffer::pagesize;
			metaData = (MetaData*) chunkBegin;
			memcpy(bufferWriter, chunkBegin, MemoryBuffer::pagesize);
			offsetPage++;
			metaDataNew = (MetaData*) bufferWriter;
			metaDataNew->haveNextPage = false;
			metaDataNew->NextPageNo = 0;
			while (metaData->haveNextPage) {
				chunkBegin = TempMMapBuffer::getInstance().getAddress() + metaData->NextPageNo * MemoryBuffer::pagesize;
				metaData = (MetaData*) chunkBegin;
				if (metaData->usedSpace == sizeof(MetaData))
					break;
				buffer->resize(MemoryBuffer::pagesize);
				bufferWriter = buffer->get_address() + offsetPage * MemoryBuffer::pagesize;
				memcpy(bufferWriter, chunkBegin, MemoryBuffer::pagesize);
				offsetPage++;
				metaDataNew = (MetaData*) bufferWriter;
				metaDataNew->haveNextPage = false;
				metaDataNew->NextPageNo = 0;
			}
			offsetId++;
		}

		bufferWriterEnd = buffer->get_address() + offsetPage * MemoryBuffer::pagesize;
		bufferWriterBegin = buffer->get_address() + lastoffsetPage * MemoryBuffer::pagesize;
		if (offsetPage == lastoffsetPage + 1) {
			ChunkManagerMeta *meta = (ChunkManagerMeta*) (bufferWriterBegin);
			MetaData *metaDataTemp = (MetaData*) (bufferWriterBegin + sizeof(ChunkManagerMeta));
			meta->usedSpace[0] = metaDataTemp->usedSpace;
			meta->length[0] = MemoryBuffer::pagesize;
		} else {
			ChunkManagerMeta *meta = (ChunkManagerMeta*) (bufferWriterBegin);
			MetaData *metaDataTemp = (MetaData*) (bufferWriterEnd - MemoryBuffer::pagesize);
			meta->usedSpace[0] = bufferWriterEnd - bufferWriterBegin - sizeof(ChunkManagerMeta) - MemoryBuffer::pagesize + metaDataTemp->usedSpace;
			meta->length[0] = bufferWriterEnd - bufferWriterBegin;
			assert(meta->length[0] % MemoryBuffer::pagesize == 0);
		}
		buffer->flush();

		//the part of xyType1
		buffer->resize(MemoryBuffer::pagesize);
		bufferWriter = buffer->get_address() + offsetPage * MemoryBuffer::pagesize;
		startPtr = (char*) predicate_managers[soType][id]->getStartPtr(2);
		offsetId = 0;
		tableSize = predicate_managers[soType][id]->getChunkNumber(2);
		metaData = (MetaData*) startPtr;

		chunkManagerBegin = chunkBegin = startPtr;
		memcpy(bufferWriter, chunkBegin, MemoryBuffer::pagesize);
		offsetPage++;
		metaDataNew = (MetaData*) bufferWriter;
		metaDataNew->haveNextPage = false;
		metaDataNew->NextPageNo = 0;

		while (metaData->haveNextPage) {
			chunkBegin = TempMMapBuffer::getInstance().getAddress() + metaData->NextPageNo * MemoryBuffer::pagesize;
			metaData = (MetaData*) chunkBegin;
			if (metaData->usedSpace == sizeof(MetaData))
				break;
			buffer->resize(MemoryBuffer::pagesize);
			bufferWriter = buffer->get_address() + offsetPage * MemoryBuffer::pagesize;
			memcpy(bufferWriter, chunkBegin, MemoryBuffer::pagesize);
			offsetPage++;
			metaDataNew = (MetaData*) bufferWriter;
			metaDataNew->haveNextPage = false;
			metaDataNew->NextPageNo = 0;
		}
		offsetId++;
		while (offsetId < tableSize) {
			buffer->resize(MemoryBuffer::pagesize);
			bufferWriter = buffer->get_address() + offsetPage * MemoryBuffer::pagesize;
			chunkBegin = chunkManagerBegin + offsetId * MemoryBuffer::pagesize;
			metaData = (MetaData*) chunkBegin;
			memcpy(bufferWriter, chunkBegin, MemoryBuffer::pagesize);
			offsetPage++;
			metaDataNew = (MetaData*) bufferWriter;
			metaDataNew->haveNextPage = false;
			metaDataNew->NextPageNo = 0;
			while (metaData->haveNextPage) {
				chunkBegin = TempMMapBuffer::getInstance().getAddress() + metaData->NextPageNo * MemoryBuffer::pagesize;
				metaData = (MetaData*) chunkBegin;
				if (metaData->usedSpace == sizeof(MetaData))
					break;
				buffer->resize(MemoryBuffer::pagesize);
				bufferWriter = buffer->get_address() + offsetPage * MemoryBuffer::pagesize;
				memcpy(bufferWriter, chunkBegin, MemoryBuffer::pagesize);
				offsetPage++;
				metaDataNew = (MetaData*) bufferWriter;
				metaDataNew->haveNextPage = false;
				metaDataNew->NextPageNo = 0;
			}
			offsetId++;
		}

		bufferWriterEnd = buffer->get_address() + offsetPage * MemoryBuffer::pagesize;
		bufferWriterBegin = buffer->get_address() + lastoffsetPage * MemoryBuffer::pagesize;
		if (1) {
			ChunkManagerMeta *meta = (ChunkManagerMeta*) (bufferWriterBegin);
			MetaData *metaDataTemp = (MetaData*) (bufferWriterEnd - MemoryBuffer::pagesize);
			meta->length[1] = bufferWriterEnd - bufferWriterBegin - meta->length[0];
			meta->usedSpace[1] = meta->length[1] - MemoryBuffer::pagesize + metaDataTemp->usedSpace;
			//not update the startPtr, endPtr
		}
		buffer->flush();
		//not update the LineHashIndex
		predicateOffset += sizeof(ID) + sizeof(SOType) + sizeof(size_t) * 2;
	}

	predicateOffset = 0;
	predicateReader = bitmapPredicateImage->get_address();
	while (predicateOffset < sizePredicateBuffer) {
		id = *((ID*) predicateReader);
		predicateReader += sizeof(ID);
		soType = *((SOType*) predicateReader);
		predicateReader += sizeof(SOType);
		offset = *((size_t*) predicateReader);
		predicateReader += sizeof(size_t);
		predicateReader += sizeof(size_t);

#ifdef TTDEBUG
		cout << "id:" << id << " soType:" << soType << endl;
		cout << "offset:" << offset << " indexOffset:" << predicateOffset << endl;
#endif

		char *base = buffer->get_address() + offset;
		ChunkManagerMeta *meta = (ChunkManagerMeta*) base;
		meta->startPtr[0] = base + sizeof(ChunkManagerMeta);
		meta->endPtr[0] = meta->startPtr[0] + meta->usedSpace[0];
		meta->startPtr[1] = base + meta->length[0];
		meta->endPtr[1] = meta->startPtr[1] + meta->usedSpace[1];

		predicate_managers[soType][id]->meta = meta;
		predicate_managers[soType][id]->buildChunkIndex();
		predicate_managers[soType][id]->updateChunkIndex();

		predicateOffset += sizeof(ID) + sizeof(SOType) + sizeof(size_t) * 2;
	}
	buffer->flush();

	string bitmapNameOld = dir + "/BitmapBuffer";
//	MMapBuffer *bufferOld = new MMapBuffer(bitmapNameOld.c_str(), 0);
	bitmapOld->discard();
	if (rename(bitmapName.c_str(), bitmapNameOld.c_str()) != 0) {
		perror("rename bitmapName error!");
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void getTempFilename(string& filename, unsigned pid, unsigned _type) {
	filename.clear();
	filename.append(DATABASE_PATH);
	filename.append("temp_");
	char temp[5];
	sprintf(temp, "%d", pid);
	filename.append(temp);
	sprintf(temp, "%d", _type);
	filename.append(temp);
}

ChunkManager::ChunkManager(unsigned pid, unsigned _type, BitmapBuffer* _bitmapBuffer) :
	bitmapBuffer(_bitmapBuffer) {
    for (int i = 0; i < objTypeNum; ++i) {
        usedPage[i].resize(0);
    }
	size_t pageNo = 0;
	meta = NULL;
    for (int i = 0; i < objTypeNum; ++i) {
        ptrs[i] = bitmapBuffer->getPage(_type, i, pageNo);
        usedPage[i].push_back(pageNo);
        meta = (ChunkManagerMeta*) ptrs[i];
        memset((char*) meta, 0, sizeof(ChunkManagerMeta));
        meta->endPtr[i] = meta->startPtr[i] = ptrs[i] + sizeof(ChunkManagerMeta);
        //meta->length[type-1]的初始大小应该是1*MemoryBuffer::pagesize,即4KB
        meta->length[i] = usedPage[i].size() * MemoryBuffer::pagesize;
        meta->usedSpace[i] = 0;
        meta->tripleCount[i] = 0;
        meta->pid = pid;
        meta->type = _type;
    }

	// TODO: @youyujie, the code beneath should be modified by youyujie
	if (meta->type == 0) {
		chunkIndex[0] = new LineHashIndex(*this, LineHashIndex::SUBJECT_INDEX, LineHashIndex::YBIGTHANX);
		chunkIndex[1] = new LineHashIndex(*this, LineHashIndex::SUBJECT_INDEX, LineHashIndex::XBIGTHANY);
	} else {
		chunkIndex[0] = new LineHashIndex(*this, LineHashIndex::OBJECT_INDEX, LineHashIndex::YBIGTHANX);
		chunkIndex[1] = new LineHashIndex(*this, LineHashIndex::OBJECT_INDEX, LineHashIndex::XBIGTHANY);
	}
}

ChunkManager::~ChunkManager() {
	///free the buffer;
    for (int i = 0; i < objTypeNum; ++i) {
        ptrs[i] = NULL;
    }

	if (chunkIndex[0] != NULL)
		delete chunkIndex[0];
	chunkIndex[0] = NULL;
	if (chunkIndex[1] != NULL)
		delete chunkIndex[1];
	chunkIndex[1] = NULL;
}

// TODO: function need to be changed if storage changed
static void getInsertChars(char* temp, unsigned x, unsigned y) {
	char* ptr = temp;

	while (x >= 128) {
		unsigned char c = static_cast<unsigned char> (x & 127);
		*ptr = c;
		ptr++;
		x >>= 7;
	}
	*ptr = static_cast<unsigned char> (x & 127);
	ptr++;

	while (y >= 128) {
		unsigned char c = static_cast<unsigned char> (y | 128);
		*ptr = c;
		ptr++;
		y >>= 7;
	}
	*ptr = static_cast<unsigned char> (y | 128);
	ptr++;
}

// TODO: function need to be changed if storage changed
void ChunkManager::insertXY(unsigned x, Element y, unsigned len, unsigned char type)
//x:xID, y:yID, len:len(xID + yID), (type: objType);
{
	char temp[15];
	// Created by peng on 2019-04-22 09:57:40.
	// I think we don't need it now.
	// origin: 标志位设置,以128为进制单位,分解x,y,最高位为0表示x,1表示y
	// getInsertChars(temp, x, y);
    switch (type){
        case 0:
            memcpy(temp,&x, sizeof(x));
            memcpy(temp+ sizeof(x),&y.id, sizeof(y.id));
            break;
        case 1:
            memcpy(temp,&x, sizeof(x));
            memcpy(temp+ sizeof(x),&y.f, sizeof(y.f));
            break;
        case 2:
            memcpy(temp,&x, sizeof(x));
            memcpy(temp+ sizeof(x),&y.d, sizeof(y.d));
            break;
        case 3:
            memcpy(temp,&y.id, sizeof(y.id));
            memcpy(temp + sizeof(y.id),&x, sizeof(x));
            break;
        case 4:
            memcpy(temp,&y.f, sizeof(y.f));
            memcpy(temp + sizeof(y.f),&x, sizeof(x));
            break;
        case 5:
            memcpy(temp,&y.d, sizeof(y.d));
            memcpy(temp + sizeof(y.d),&x, sizeof(x));
            break;
    }
    type = type % objTypeNum;
	//如果当前空间不够存放新的<x,y>对
	if (isPtrFull(type, len) == true) {
            if (meta->length[type] == MemoryBuffer::pagesize) {//第一个chunk,在第一个chunk被写满(存放不下下一个元组的时候，回溯指针，写metadata的信息)
                //将指针回溯到MetaData(即head区域)写入usedSpace信息
                MetaData *metaData = (MetaData*) (meta->endPtr[type] - meta->usedSpace[type]);
                metaData->usedSpace = meta->usedSpace[type];
            }else {//不是第一个chunk
                //这个usedpage计算最后一个chunk使用了多少字节，length[0]存放的是当前谓词,x<=y的数据链表的已申请buffer大小
                size_t usedPage = MemoryBuffer::pagesize
                        - (meta->length[type] - meta->usedSpace[type] - (type==0?sizeof(ChunkManagerMeta):0));
                //MetaData地址=尾指针-最后一个4KB字节使用的字节，即指向了最后一个4KB字节的首地址，也就是head区域
                MetaData *metaData = (MetaData*) (meta->endPtr[type] - usedPage);
                metaData->usedSpace = usedPage;
            }
            //重新分配大小,修改了meta->length,增加一个4KB,meta->endptr指向下一个4KB的首地址
            resize(type);
            //为下一个4KB创建head信息，下一个chunk的metadata首地址是meta->endPtr[]
            MetaData *metaData = (MetaData*) (meta->endPtr[type]);
            metaData->minID = x;
            metaData->haveNextPage = false;
            metaData->NextPageNo = 0;
            metaData->usedSpace = 0;

            memcpy(meta->endPtr[type] + sizeof(MetaData), temp, len);
            meta->endPtr[type] = meta->endPtr[type] + sizeof(MetaData) + len;
            meta->usedSpace[type] = meta->length[type] - MemoryBuffer::pagesize
                    - (type==0?sizeof(ChunkManagerMeta):0) + sizeof(MetaData) + len;
            tripleCountAdd(type);
	} else if (meta->usedSpace[type] == 0) { //如果usedspace==0，即第一个chunk块，则创建head区域
		MetaData *metaData = (MetaData*) (meta->startPtr[type]);
		memset((char*) metaData, 0, sizeof(MetaData));//将head区域初始化为0
		metaData->minID = x; // x must be the minID
		metaData->haveNextPage = false;
		metaData->NextPageNo = 0;
		metaData->usedSpace = 0;

		memcpy(meta->endPtr[type] + sizeof(MetaData), temp, len); //将数据拷贝到head区域的后面len个字节中去
		meta->endPtr[type] = meta->endPtr[type] + sizeof(MetaData) + len;//重新定位endPtr[type-1]的位置
		meta->usedSpace[type] = sizeof(MetaData) + len; //更新usedSpace的大小,包括MetaData的大小在内。
		tripleCountAdd(type);
	} else { 	//如果不是新的块，则直接将数据拷贝到endPtr[type-1]的后len个字节中去。
		memcpy(meta->endPtr[type], temp, len);

		meta->endPtr[type] = meta->endPtr[type] + len;
		meta->usedSpace[type] = meta->usedSpace[type] + len;
		tripleCountAdd(type);
	}
}

Status ChunkManager::resize(unsigned char type) {
	// TODO
	size_t pageNo = 0;
	ptrs[type] = bitmapBuffer->getPage(meta->type, type, pageNo);
	usedPage[type].push_back(pageNo);
	meta->length[type] = usedPage[type].size() * MemoryBuffer::pagesize;
	meta->endPtr[type] = ptrs[type];

	bufferCount++;
	return OK;
}

/// build the hash index for query;
Status ChunkManager::buildChunkIndex() {
	chunkIndex[0]->buildIndex(1);
	chunkIndex[1]->buildIndex(2);

	return OK;
}
// Created by peng on 2019-04-19 11:58:15.
// TODO: @youyujie, maybe youyujie need to modify the code below
/// update the hash index for Query
Status ChunkManager::updateChunkIndex() {
	chunkIndex[0]->updateLineIndex();
	chunkIndex[1]->updateLineIndex();

	return OK;
}

bool ChunkManager::isPtrFull(unsigned char type, unsigned len) {
    len = len + (type==0?sizeof(ChunkManagerMeta):0);
	return meta->usedSpace[type] + len >= meta->length[type];
}

ID ChunkManager::getChunkNumber(unsigned char type) {
	return (meta->length[type]) / (MemoryBuffer::pagesize);
}

ChunkManager* ChunkManager::load(unsigned pid, unsigned type, char* buffer, size_t& offset) {
	ChunkManagerMeta * meta = (ChunkManagerMeta*) (buffer + offset);
	if (meta->pid != pid || meta->type != type) {
		MessageEngine::showMessage("load chunkmanager error: check meta info", MessageEngine::ERROR);
		cout << meta->pid << ": " << meta->type << endl;
		return NULL;
	}

	ChunkManager* manager = new ChunkManager();
	char* base = buffer + offset;
	manager->meta = meta;

    for (int i = 0; i < objTypeNum; ++i) {
        manager->meta->startPtr[i] = base + (i == 0?sizeof(ChunkManagerMeta):0);
        manager->meta->endPtr[i] = manager->meta->startPtr[i] + manager->meta->usedSpace[i];
        base += manager->meta->length[i];
    }
    offset = base - buffer;
	return manager;
}

size_t ChunkManager::save(char *buffer, SOType type) {
    size_t increment = 0;
    MMapBuffer *temp[3];
    temp[0] = this->bitmapBuffer->temp1;
    temp[1] = this->bitmapBuffer->temp2;
    temp[2] = this->bitmapBuffer->temp3;
    if (type){
        temp[0] = this->bitmapBuffer->temp4;
        temp[1] = this->bitmapBuffer->temp5;
        temp[2] = this->bitmapBuffer->temp6;
    }
    for (int i = 0; i < objTypeNum; ++i) {
        vector<size_t> v = usedPage[i];
        increment += v.size();
        for (int j = 0; j < v.size(); ++j) {
            memcpy(buffer, temp[i]->get_address() + v[j] * MemoryBuffer::pagesize, MemoryBuffer::pagesize);
            buffer += MemoryBuffer::pagesize;
        }
    }
    return increment;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

Chunk::Chunk(unsigned char type, ID xMax, ID xMin, ID yMax, ID yMin, char* startPtr, char* endPtr) {
	this->type = type;
	this->xMax = xMax;
	this->xMin = xMin;
	this->yMax = yMax;
	this->yMin = yMin;
	count = 0;
	this->startPtr = startPtr;
	this->endPtr = endPtr;
}

Chunk::~Chunk() {
	this->startPtr = 0;
	this->endPtr = 0;
}

/*
 *	write x id; set the 7th bit to 0 to indicate it is a x byte;
 */
void Chunk::writeXId(ID id, char*& ptr) {
	// Write a id
	while (id >= 128) {
		unsigned char c = static_cast<unsigned char> (id & 127);
		*ptr = c;
		ptr++;
		id >>= 7;
	}
	*ptr = static_cast<unsigned char> (id & 127);
	ptr++;
}

/*
 *	write y id; set the 7th bit to 1 to indicate it is a y byte;
 */
void Chunk::writeYId(ID id, char*& ptr) {
	while (id >= 128) {
		unsigned char c = static_cast<unsigned char> (id | 128);
		*ptr = c;
		ptr++;
		id >>= 7;
	}
	*ptr = static_cast<unsigned char> (id | 128);
	ptr++;
}

static inline unsigned int readUInt(const uchar* reader) {
	return (reader[0] << 24 | reader[1] << 16 | reader[2] << 8 | reader[3]);
}

const uchar* Chunk::readXId(const uchar* reader, register ID& id) {
#ifdef WORD_ALIGN
	id = 0;
	register unsigned int c = *((unsigned int*)reader);
	register unsigned int flag = c & 0x80808080; /* get the first bit of every byte. */
	switch(flag) {
		case 0: //reads 4 or more bytes;
		id = *reader;
		reader++;
		id = id | ((*reader) << 7);
		reader++;
		id = id | ((*reader) << 14);
		reader++;
		id = id | ((*reader) << 21);
		reader++;
		if(*reader < 128) {
			id = id | ((*reader) << 28);
			reader++;
		}
		break;
		case 0x80000080:
		case 0x808080:
		case 0x800080:
		case 0x80008080:
		case 0x80:
		case 0x8080:
		case 0x80800080:
		case 0x80808080:
		break;

		case 0x80808000://reads 1 byte;
		case 0x808000:
		case 0x8000:
		case 0x80008000:
		id = *reader;
		reader++;
		break;
		case 0x800000: //read 2 bytes;
		case 0x80800000:
		id = *reader;
		reader++;
		id = id | ((*reader) << 7);
		reader++;
		break;
		case 0x80000000: //reads 3 bytes;
		id = *reader;
		reader++;
		id = id | ((*reader) << 7);
		reader++;
		id = id | ((*reader) << 14);
		reader++;
		break;
	}
	return reader;
#else
	// Read an x id
	register unsigned shift = 0;
	id = 0;
	register unsigned int c;

	while (true) {
		c = *reader;
		if (!(c & 128)) {
			id |= (c & 0x7F) << shift;
			shift += 7;
		} else {
			break;
		}
		reader++;
	}
	return reader;

	// register unsigned shift = 0;
	// id = 0;
	// register unsigned int c;

	// while (true) {
	// 	c = *reader;
	// 	if (!(c & 128)) {
	// 		id |= c << shift;
	// 		shift += 7;
	// 	} else {
	// 		break;
	// 	}
	// 	reader++;
	// }
	// return reader;
#endif /* end for WORD_ALIGN */
}

const uchar* Chunk::readXYId(const uchar* reader,register ID& xid,register ID &yid){
	// xid = yid = 0;
	// while((*reader) & 128){
	// 	cout<<"@read xid"<<endl;
	// 	cout<<*reader<<endl;
	// 	xid <<= 7;
	// 	xid |= (*reader & 0x7F);
	// 	reader++;
	// }

	// while(!((*reader) & 128)){
	// 	cout<<"@read yid"<<endl;
	// 	cout<<*reader<<endl;
	// 	yid <<= 7;
	// 	yid |= (*reader & 0x7F);
	// 	reader++;
	// }
	// return reader;
	
	register unsigned shift = 0;
	xid = 0;
	register unsigned int c;

	while (true) {
		c = *reader;
		if (!(c & 128)) {
			xid |= (c & 0x7F) << shift;
			shift += 7;
		} else {
			break;
		}
		reader++;
	}

	shift = 0;
	yid = 0;

	while (true) {
		c = *reader;
		if (c & 128) {
			yid |= (c & 0x7F) << shift;
			shift += 7;
		} else {
			break;
		}
		reader++;
	}
	return reader;
}

const uchar* Chunk::readYId(const uchar* reader, register ID& id) {
	// Read an y id
#ifdef WORD_ALIGN
	id = 0;
	register unsigned int c = *((unsigned int*)reader);
	register unsigned int flag = c & 0x80808080; /* get the first bit of every byte. */
	switch(flag) {
		case 0: //no byte;
		case 0x8000:
		case 0x808000:
		case 0x80008000:
		case 0x80800000:
		case 0x800000:
		case 0x80000000:
		case 0x80808000:
		break;
		case 0x80:
		case 0x80800080:
		case 0x80000080:
		case 0x800080: //one byte
		id = (*reader)& 0x7F;
		reader++;
		break;
		case 0x8080:
		case 0x80008080: // two bytes
		id = (*reader)& 0x7F;
		reader++;
		id = id | (((*reader) & 0x7F) << 7);
		reader++;
		break;
		case 0x808080: //three bytes;
		id = (*reader) & 0x7F;
		reader++;
		id = id | (((*reader) & 0x7F) << 7);
		reader++;
		id = id | (((*reader) & 0x7F) << 14);
		reader++;
		break;
		case 0x80808080: //reads 4 or 5 bytes;
		id = (*reader) & 0x7F;
		reader++;
		id = id | (((*reader) & 0x7F) << 7);
		reader++;
		id = id | (((*reader) & 0x7F) << 14);
		reader++;
		id = id | (((*reader) & 0x7F) << 21);
		reader++;
		if(*reader >= 128) {
			id = id | (((*reader) & 0x7F) << 28);
			reader++;
		}
		break;
	}
	return reader;
#else



	register unsigned shift = 0;
	id = 0;
	register unsigned int c;


	while (true) {
		c = *reader;
		if (c & 128) {
			id |= (c & 0x7F) << shift;
			shift += 7;
		} else {
			break;
		}
		reader++;
	}
	return reader;
#endif /* END FOR WORD_ALIGN */
}

uchar* Chunk::deleteXId(uchar* reader)
/// Delete a subject id (just set the id to 0)
{
	register unsigned int c;

	while (true) {
		c = *reader;
		if (!(c & 128))
			(*reader) = 0;
		else
			break;
		reader++;
	}
	return reader;
}

uchar* Chunk::deleteYId(uchar* reader)
/// Delete an object id (just set the id to 0)
{
	register unsigned int c;

	while (true) {
		c = *reader;
		if (c & 128)
			(*reader) = (*reader) & 0x80;
		else
			break;
		reader++;
	}
	return reader;
}

const uchar* Chunk::skipId(const uchar* reader, unsigned char flag) {
	// Skip an id
	if (flag == 1) {
		while ((*reader) & 128)
			++reader;
	} else {
		while (!((*reader) & 128))
			++reader;
	}

	return reader;
}

const uchar* Chunk::skipForward(const uchar* reader) {
	// skip a x,y forward;
	return skipId(skipId(reader, 0), 1);
}

const uchar* Chunk::skipBackward(const uchar* reader) {
	// skip backward to the last x,y;
	while ((*reader) == 0)
		--reader;
	while ((*reader) & 128)
		--reader;
	while (!((*reader) & 128))
		--reader;
	return ++reader;
}

const uchar* Chunk::skipBackward(const uchar* reader, const uchar* begin, unsigned type) {
	//if is the begin of One Chunk
	if (type == 1) {
		if ((reader - begin - sizeof(MetaData) + sizeof(ChunkManagerMeta)) % MemoryBuffer::pagesize == 0 || (reader + 1 - begin - sizeof(MetaData) + sizeof(ChunkManagerMeta)) % MemoryBuffer::pagesize
				== 0) {
			if ((reader - begin - sizeof(MetaData) + sizeof(ChunkManagerMeta)) == MemoryBuffer::pagesize || (reader + 1 - begin - sizeof(MetaData) + sizeof(ChunkManagerMeta))
					== MemoryBuffer::pagesize)
			// if is the second Chunk
			{
				reader = begin;
				MetaData* metaData = (MetaData*) reader;
				reader = reader + metaData->usedSpace;
				--reader;
				return skipBackward(reader);
			}
			reader = begin - sizeof(ChunkManagerMeta) + MemoryBuffer::pagesize * ((reader - begin + sizeof(ChunkManagerMeta)) / MemoryBuffer::pagesize - 1);
			MetaData* metaData = (MetaData*) reader;
			reader = reader + metaData->usedSpace;
			--reader;
			return skipBackward(reader);
		} else if (reader <= begin + sizeof(MetaData)) {
			return begin - 1;
		} else {
			//if is not the begin of one Chunk
			return skipBackward(reader);
		}
	}
	if (type == 2) {
		if ((reader - begin - sizeof(MetaData)) % MemoryBuffer::pagesize == 0 || (reader + 1 - begin - sizeof(MetaData)) % MemoryBuffer::pagesize == 0) {
			reader = begin + MemoryBuffer::pagesize * ((reader - begin) / MemoryBuffer::pagesize - 1);
			MetaData* metaData = (MetaData*) reader;
			reader = reader + metaData->usedSpace;
			--reader;
			return skipBackward(reader);
		} else if (reader <= begin + sizeof(MetaData)) {
			return begin - 1;
		} else {
			//if is not the begin of one Chunk
			return skipBackward(reader);
		}
	}
}
