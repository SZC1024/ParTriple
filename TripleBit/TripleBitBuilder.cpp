/*
 * TripleBitBuilder.cpp
 *
 *  Created on: Apr 6, 2010
 *      Author: root
 */

#include "MemoryBuffer.h"
#include "MMapBuffer.h"
#include "BitmapBuffer.h"
#include "TripleBitBuilder.h"
#include "PredicateTable.h"
#include "TripleBit.h"
#include "URITable.h"
#include "Sorter.h"
#include "StatisticsBuffer.h"

#include <string.h>
#include <pthread.h>

static int getCharPos(const char* data, char ch)
{
	const char * p = data;
	int i = 0;
	while(*p != '\0'){
		if( *p == ch)
			return i+1;
		p++;
		i++;
	}

	return -1;
}

TripleBitBuilder::TripleBitBuilder(string _dir) : dir(_dir) {
	preTable = new PredicateTable(dir);
	uriTable = new URITable(dir);
	bitmap = new BitmapBuffer(dir);

	statBuffer[0] = new OneConstantStatisticsBuffer(string(dir + "/subject_statis"), StatisticsBuffer::SUBJECT_STATIS);			//subject statistics buffer;
	statBuffer[1] = new OneConstantStatisticsBuffer(string(dir + "/object_statis"), StatisticsBuffer::OBJECT_STATIS);			//object statistics buffer;
	statBuffer[2] = new TwoConstantStatisticsBuffer(string(dir + "/subjectpredicate_statis"), StatisticsBuffer::SUBJECTPREDICATE_STATIS);	//subject-predicate statistics buffer;
	statBuffer[3] = new TwoConstantStatisticsBuffer(string(dir + "/objectpredicate_statis"), StatisticsBuffer::OBJECTPREDICATE_STATIS);	//object-predicate statistics buffer;

	staReifTable = new StatementReificationTable();
}

TripleBitBuilder::TripleBitBuilder() {
	preTable = NULL;
	uriTable = NULL;
	bitmap = NULL;
	staReifTable = NULL;
}

TripleBitBuilder::~TripleBitBuilder() {
#ifdef TRIPLEBITBUILDER_DEBUG
	cout << "Bit map builder destroyed begin " << endl;
#endif
	//mysql = NULL;
	if( preTable != NULL ) 
		delete preTable;
	preTable = NULL;

	if( uriTable != NULL )
		delete uriTable;
	uriTable = NULL;
	//delete uriStaBuffer;
	if ( staReifTable != NULL )
		delete staReifTable;
	staReifTable = NULL;

	if (bitmap != NULL) {
		delete bitmap;
		bitmap = NULL;
	}

	for(int i = 0; i < 4; i++)
	{
		if(statBuffer[i] != NULL)
			delete statBuffer[i];
		statBuffer[i] = NULL;
	}
}

bool TripleBitBuilder::isStatementReification(const char* object) {
	int pos;

	const char* p;

	if ((pos = getCharPos(object, '#')) != -1) {
		p = object + pos;

		if (strcmp(p, "Statement") == 0 || strcmp(p, "subject") == 0 || strcmp(p, "predicate") == 0 || strcmp(p, "object") == 0) {
			return true;
		}
	}

	return false;
}

//已作废
bool TripleBitBuilder::generateXY(ID& subjectID, ID& objectID)
{
	if (subjectID > objectID)
	{
		ID temp = subjectID;
		subjectID = objectID;
		objectID = temp - objectID;
		return true;
	}
	else {
		objectID = objectID - subjectID;
		return false;
	}
}

void TripleBitBuilder::NTriplesParse(const char* subject,  const char* predicate, const char* object, TempFile& facts) {
	ID subjectID, objectID, predicateID;

	if (isStatementReification(object) == false && isStatementReification(predicate) == false) {

		//得到S，P，O的ID
		if (preTable->getIDByPredicate(predicate, predicateID) == PREDICATE_NOT_BE_FINDED)
			preTable->insertTable(predicate, predicateID);
		if (uriTable->getIdByURI(subject, subjectID) == URI_NOT_FOUND)
			uriTable->insertTable(subject, subjectID);
		if (uriTable->getIdByURI(object, objectID) == URI_NOT_FOUND)
			uriTable->insertTable(object, objectID);

		facts.writeId(subjectID);
		facts.writeId(predicateID);
		facts.writeId(objectID);
	} else {
//		statementFile << subject << " : " << predicate << " : " << object << endl;
	}

}

bool TripleBitBuilder::N3Parse(istream& in, const char* name, TempFile& rawFacts) {
	cerr << "Parsing " << name << "..." << endl;

	TurtleParser parser(in);
	try {
		string subject, predicate, object, source;
		while (true) {
			try {
				if (!parser.parse(subject, predicate, object))
					break;
			} catch (const TurtleParser::Exception& e) {
				while (in.get() != '\n');
				continue;
			}
			//Construct IDs
			//and write the triples
			if (subject.length() && predicate.length() && object.length())
				//也可以在这里转，代替parse函数的转换，将S和O根据情况转换成ID（不做修改），flaot和double



				NTriplesParse((char*)subject.c_str(), (char*)predicate.c_str(), (char*)object.c_str(), rawFacts);
		}
	} catch (const TurtleParser::Exception&) {
		return false;
	}
	return true;
}

const char* TripleBitBuilder::skipIdIdId(const char* reader) {
	return TempFile::skipId(TempFile::skipId(TempFile::skipId(reader)));
}

int TripleBitBuilder::compare213(const char* left, const char* right) {
	ID l1, l2, l3, r1, r2, r3;
	loadTriple(left, l1, l2, l3);
	loadTriple(right, r1, r2, r3);

	return cmpTriples(l2, l1, l3, r2, r1, r3);
}

int TripleBitBuilder::compare231(const char* left, const char* right) {
	ID l1, l2, l3, r1, r2, r3;
	loadTriple(left, l1, l2, l3);
	loadTriple(right, r1, r2, r3);

	return cmpTriples(l2, l3, l1, r2, r3, r1);
}

int TripleBitBuilder::compare123(const char* left, const char* right) {
	ID l1, l2, l3, r1, r2, r3;
	loadTriple(left, l1, l2, l3);
	loadTriple(right, r1, r2, r3);

	return cmpTriples(l1, l2, l3, r1, r2, r3);
}

int TripleBitBuilder::compare321(const char* left, const char* right) {
	ID l1, l2, l3, r1, r2, r3;
	loadTriple(left, l1, l2, l3);
	loadTriple(right, r1, r2, r3);

	return cmpTriples(l3, l2, l1, r3, r2, r1);
}

Status TripleBitBuilder::resolveTriples(TempFile& rawFacts, TempFile& facts) {
	cout<<"Sort by Subject"<<endl;
	ID subjectID, objectID, predicateID;
	ID lastSubject = 0, lastObject = 0, lastPredicate = 0;
	unsigned count0 = 0, count1 = 0;

	TempFile sortedBySubject("./SortByS"), sortedByObject("./SortByO");

	// Created by peng on 2019-04-19 09:09:58.
	// v is objType.
	// ************************************
	unsigned v = 1;
    // ************************************
	Sorter::sort(rawFacts, sortedBySubject, skipIdIdId, compare123);
	//此时sortedBySubject已经有了排序之后的数据，该排序属于保存原序列的排序
	//原文件rawFacts没有任何改变，排序之后的数据存在sortedBySubject

	{
		//insert into chunk
		sortedBySubject.close();
		MemoryMappedFile mappedIn;
		assert(mappedIn.open(sortedBySubject.getFile().c_str()));
		const char* reader = mappedIn.getBegin(), *limit = mappedIn.getEnd();

		loadTriple(reader, subjectID, predicateID, objectID);
		lastSubject = subjectID; lastPredicate = predicateID; lastObject = objectID;
		reader = skipIdIdId(reader);//reader往后跳12个字节

		//s>o ture
		bitmap->insertTriple(predicateID, subjectID, objectID, v, 0);
		count0 = count1 = 1;
		
		while (reader < limit) {
			loadTriple(reader, subjectID, predicateID, objectID);
			if(lastSubject == subjectID && lastPredicate == predicateID && lastObject == objectID) {
				reader = skipIdIdId(reader);
				continue;
			}

			if ( subjectID != lastSubject ) {
				//statBuffer[0]统计的是subject对应的三元组个数,例如subjectID是4的三元组个数有多少个
				((OneConstantStatisticsBuffer*)statBuffer[0])->addStatis(lastSubject, count0);

				//statBuffer[2]统计的是subject-predicate对应的三元组个数,例如subjectID是4且predicateID是1的三元组个数有多少个
				statBuffer[2]->addStatis(lastSubject, lastPredicate, count1);

				lastPredicate = predicateID;
				lastSubject = subjectID;
				count0 = count1 = 1;
			} else if ( predicateID != lastPredicate ) {
				statBuffer[2]->addStatis(lastSubject, lastPredicate, count1);
				lastPredicate = predicateID;
				count0++; count1 = 1;
			}else {
				count0++; count1++;
				lastObject = objectID;
			}
			
			reader = reader + 12;
			//0 indicate the triple is sorted by subjects' id;
			bitmap->insertTriple(predicateID, subjectID, objectID, v, 0);
		}
		mappedIn.close();
	}

	bitmap->flush();

	//sort
	cerr << "Sort by Object" << endl;

	Sorter::sort(rawFacts, sortedByObject, skipIdIdId, compare321);
	//此时sortedByObject已经有了排序之后的数据，该排序属于保存原序列的排序
	//原文件rawFacts没有任何改变，排序之后的数据存在sortedByObject

	{
		//insert into chunk
		sortedByObject.close();
		MemoryMappedFile mappedIn;
		assert(mappedIn.open(sortedByObject.getFile().c_str()));
		const char* reader = mappedIn.getBegin(), *limit = mappedIn.getEnd();

		loadTriple(reader, subjectID, predicateID, objectID);
		lastSubject = subjectID; lastPredicate = predicateID; lastObject = objectID;
		reader = skipIdIdId(reader);

		bitmap->insertTriple(predicateID, objectID, subjectID, v, 1);
		count0 = count1 = 1;

		while (reader < limit) {
			loadTriple(reader, subjectID, predicateID, objectID);
			 if(lastSubject == subjectID && lastPredicate == predicateID && lastObject == objectID) {
				reader = skipIdIdId(reader);
				continue;
			}
			if ( objectID != lastObject ) {
				//statBuffer内存储的内容同上
				((OneConstantStatisticsBuffer*)statBuffer[1])->addStatis(lastObject, count0);//statBuffer[1] object statistics buffer
				statBuffer[3]->addStatis(lastObject, lastPredicate, count1); //statBuffer[3] object-predicate statistics buffer

				lastPredicate = predicateID;
				lastObject = objectID;
				count0 = count1 = 1;
			} else if ( predicateID != lastPredicate ) {
				statBuffer[3]->addStatis(lastObject, lastPredicate, count1);
				lastPredicate = predicateID;
				count0++; count1 = 1;
			} else {
				lastSubject = subjectID;
				count0++; count1++;
			}
			reader = skipIdIdId(reader);
			// 1 indicate the triple is sorted by objects' id;
			bitmap->insertTriple(predicateID, objectID, subjectID, v, 1);
		}
		mappedIn.close();
	}

	bitmap->flush();
	rawFacts.discard();
	sortedByObject.discard();
	sortedBySubject.discard();

	return OK;
}

Status TripleBitBuilder::startBuildN3(string fileName) {
	TempFile rawFacts("./test");//解析之后的原始数据（未排序），里边全都是ID，一个ID接一个ID存储，没有分隔符

	ifstream in((char*)fileName.c_str());
	if (!in.is_open()) {
		cerr << "Unable to open " << fileName << endl;
		return ERROR;
	}


	if (!N3Parse(in, fileName.c_str(), rawFacts)) {//判断条件为一个三元组的解析过程
		in.close();
		return ERROR;
	}

	in.close();
	delete uriTable;
	uriTable = NULL;
	delete preTable;
	preTable = NULL;
	delete staReifTable;
	staReifTable = NULL;

	rawFacts.flush();
	cout<<"over"<<endl;

	//sort by s,o
	TempFile facts(fileName);//fact是最原始的数据，也就是下载的数据集文件
	resolveTriples(rawFacts, facts);//我其实可以在resolve里边再把不需要映射成id的数字和浮点数，映射回来，把原来的ID删掉
	facts.discard();
	return OK;
}

Status TripleBitBuilder::buildIndex() {
	// build hash index;
	MMapBuffer* bitmapIndex;
	cout<<"build hash index for subject"<<endl;
	for ( map<ID,ChunkManager*>::iterator iter = bitmap->predicate_managers[0].begin(); iter != bitmap->predicate_managers[0].end(); iter++ ) {
		if ( iter->second != NULL ) {
			iter->second->buildChunkIndex();
			iter->second->getChunkIndex(1)->save(bitmapIndex);
			iter->second->getChunkIndex(2)->save(bitmapIndex);
		}
	}

	cout<<"build hash index for object"<<endl;
	for ( map<ID, ChunkManager*>::iterator iter = bitmap->predicate_managers[1].begin(); iter != bitmap->predicate_managers[1].end(); iter++ ) {
		if ( iter->second != NULL ) {
			iter->second->buildChunkIndex();
			iter->second->getChunkIndex(1)->save(bitmapIndex);
			iter->second->getChunkIndex(2)->save(bitmapIndex);
		}
	}

	return OK;
}

Status TripleBitBuilder::endBuild() {
	bitmap->save();

	ofstream ofile(string(dir + "/statIndex").c_str());
	MMapBuffer* indexBuffer = NULL;
	((OneConstantStatisticsBuffer*)statBuffer[0])->save(indexBuffer);
	((OneConstantStatisticsBuffer*)statBuffer[1])->save(indexBuffer);
	((TwoConstantStatisticsBuffer*)statBuffer[2])->save(indexBuffer);
	((TwoConstantStatisticsBuffer*)statBuffer[3])->save(indexBuffer);

	delete indexBuffer;
	return OK;
}
