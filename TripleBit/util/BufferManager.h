/*
 * BufferManager.h
 *
 *  Created on: 2011-4-8
 *      Author: liupu
 */

#ifndef BUFFERMANAGER_H_
#define BUFFERMANAGER_H_

#define INIT_BUFFERS 30
#define INCREASE_BUFFERS 30

class EntityIDBuffer;

#include "../TripleBit.h"

class BufferManager {
private:
    // bufferPool contains all EntityIDBuffer
    // usedBuffer.size + cleanBuffer.size should be bufferPool.size
        vector<EntityIDBuffer*> bufferPool;
        vector<EntityIDBuffer*> usedBuffer;
        vector<EntityIDBuffer*> cleanBuffer;
    // bufferPool.size
        int bufferCnt;
        boost::mutex bufferMutex;

protected:
    // singleton
        static BufferManager* instance;
        BufferManager();
        bool expandBuffer();
public:
        virtual ~BufferManager();
        EntityIDBuffer* getNewBuffer();
        Status freeBuffer(EntityIDBuffer* buffer);
        Status reserveBuffer();
        void destroyBuffers();
public:
        static BufferManager* getInstance();
};


#endif /* BUFFERMANAGER_H_ */
