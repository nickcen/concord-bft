// Concord
//
// Copyright (c) 2018 VMware, Inc. All Rights Reserved.
//
// This product is licensed to you under the Apache 2.0 license (the "License").
// You may not use this product except in compliance with the Apache 2.0
// License.
//
// This product may include a number of subcomponents with separate copyright
// notices and license terms. Your use of these subcomponents is subject to the
// terms and conditions of the subcomponent's license, as noted in the LICENSE
// file.

#include "simpleKVBCTests.h"
#include <inttypes.h>
#include <map>
#include <set>
#include <list>
#include <chrono>
#include <string>
#ifndef _WIN32
#include <unistd.h>
#endif

using std::list;
using std::map;
using std::set;


#define KV_LEN (21)
#define NUMBER_OF_KEYS (200) 
#define CONFLICT_DISTANCE (49)
#define MAX_WRITES_IN_REQ (7)
#define MAX_READ_SET_SIZE_IN_REQ (10)
#define MAX_READS_IN_REQ (7)


namespace BasicRandomTests
{
	namespace Internal
	{
		void CHECK(bool cond, const char* msg) 
		{
			if (!cond) // TODO(GG): use a standard approach
			{
				printf("\nTest failed: %s", msg);
				assert(cond && msg);
			}
		}

		struct SimpleKV
		{
			char key[KV_LEN];
			char val[KV_LEN];
		};

		struct SimpleKey
		{
			char key[KV_LEN];
		};

		struct SimpleVal
		{
			char v[KV_LEN];
		};

		struct SimpleBlock
		{
			BlockId id;
			size_t numberOfItems;
			SimpleKV items[1];

			static SimpleBlock* alloc(size_t items)
			{
				size_t size = sizeof(SimpleBlock) + sizeof(SimpleKV) * (items - 1);
				char* pBuf = new char[size];
				memset(pBuf, 0, size);
				return (SimpleBlock*)(pBuf);
			}

			static void free(SimpleBlock* p)
			{
				char* p1 = (char*)p;
				delete[] p1;
			}

			//static void print(SimpleBlock* block)
			//{
			//	printf("\nBlockId=%" PRId64 " Items=%zu", block->id, block->numberOfItems);
			//	for (size_t i = 0; i < block->numberOfItems; i++)
			//	{
			//		printf("\n");
			//		printf("Block id %" PRId64 " item %3zu key=", block->id, i);
			//		for (int k = 0; k < KV_LEN; k++) printf("%02X", block->items[i].key[k]);
			//		printf("   val=");
			//		for (int k = 0; k < KV_LEN; k++) printf("%02X", block->items[i].val[k]);
			//	}
			//}
		};

		struct SimpleRequestHeader
		{
			char type; // 1 == conditional write , 2 == read, 3 == get last block

			static void free(SimpleRequestHeader* p)
			{
				char* p1 = (char*)p;
				delete[] p1;
			}
		};

		struct SimpleConditionalWriteHeader
		{
			SimpleRequestHeader h; // TODO: this is ugly ....
			BlockId readVerion;
			size_t numberOfKeysInReadSet;
			size_t numberOfWrites;
			// followed by SimpleKey[numberOfKeysInReadSet]
			// followed by SimpleKV[numberOfWrites]

			static SimpleConditionalWriteHeader* alloc(size_t numOfKeysInReadSet, size_t numOfWrites)
			{
				size_t s = size(numOfKeysInReadSet, numOfWrites);
				char* pBuf = new char[s];
				memset(pBuf, 0, s);
				return (SimpleConditionalWriteHeader*)(pBuf);
			}

			static void free(SimpleConditionalWriteHeader* p)
			{
				char* p1 = (char*)p;
				delete[] p1;
			}

			static size_t size(size_t numOfKeysInReadSet, size_t numOfWrites)
			{
				return sizeof(SimpleConditionalWriteHeader) + numOfKeysInReadSet * sizeof(SimpleKey) + numOfWrites * sizeof(SimpleKV);
			}

			size_t size()
			{
				return size(numberOfKeysInReadSet, numberOfWrites);
			}

			SimpleKey* readSetArray()
			{
				return (SimpleKey*)(((char*)this) + sizeof(SimpleConditionalWriteHeader));
			}

			SimpleKV* keyValArray()
			{
				return (SimpleKV*)(((char*)this) + sizeof(SimpleConditionalWriteHeader) + numberOfKeysInReadSet * sizeof(SimpleKey));
			}
		};

		struct SimpleReadHeader
		{
			SimpleRequestHeader h;
			BlockId readVerion; // if 0, then read from the latest version
			size_t numberOfKeysToRead;
			SimpleKey keys[1];

			static SimpleReadHeader* alloc(size_t numOfKeysToRead)
			{
				size_t size = sizeof(SimpleReadHeader) + (sizeof(SimpleKey) * (numOfKeysToRead - 1));
				char* pBuf = new char[size];
				memset(pBuf, 0, size);
				return (SimpleReadHeader*)(pBuf);
			}

			static void free(SimpleReadHeader* p)
			{
				char* p1 = (char*)p;
				delete[] p1;
			}

			static size_t size(size_t numOfKeysToRead)
			{
				return sizeof(SimpleReadHeader) + (numOfKeysToRead - 1) * sizeof(SimpleKey);
			}

			size_t size()
			{
				return size(numberOfKeysToRead);
			}

			SimpleKey* keysArray()
			{
				return ((SimpleKey*)((char*)keys));
			}


		};


		struct SimpleGetLastBlockHeader
		{
			static SimpleGetLastBlockHeader* alloc()
			{
				size_t size = sizeof(SimpleGetLastBlockHeader);
				char* pBuf = new char[size];
				memset(pBuf, 0, size);
				return (SimpleGetLastBlockHeader*)(pBuf);
			}

			static void free(SimpleGetLastBlockHeader* p)
			{
				char* p1 = (char*)p;
				delete[] p1;
			}

			static size_t size()
			{
				return sizeof(SimpleGetLastBlockHeader);
			}

			SimpleRequestHeader h;
		};




		struct SimpleReplyHeader
		{
			char type; // 1 == conditional write , 2 == read, 3 == get last block

			static void free(SimpleReplyHeader* p)
			{
				char* p1 = (char*)p;
				delete[] p1;
			}

		};



		struct SimpleReplyHeader_ConditionalWrite
		{
			static SimpleReplyHeader_ConditionalWrite* alloc()
			{
				size_t s = sizeof(SimpleReplyHeader_ConditionalWrite);
				char* pBuf = new char[s];
				memset(pBuf, 0, s);
				return (SimpleReplyHeader_ConditionalWrite*)(pBuf);
			}

			static void free(SimpleReplyHeader_ConditionalWrite* p)
			{
				char* p1 = (char*)p;
				delete[] p1;
			}


			SimpleReplyHeader h;
			bool succ;
			BlockId latestBlock;
		};

		struct SimpleReplyHeader_Read
		{
			SimpleReplyHeader h;
			size_t numberOfElements;
			SimpleKV elements[1];

			static size_t size(size_t numOfElements)
			{
				size_t size = sizeof(SimpleReplyHeader_Read) + (sizeof(SimpleKV) * (numOfElements - 1));
				return size;
			}

			size_t size()
			{
				return size(numberOfElements);
			}

			static SimpleReplyHeader_Read* alloc(size_t numOfElements)
			{
				size_t size = sizeof(SimpleReplyHeader_Read) + (sizeof(SimpleKV) * (numOfElements - 1));
				char* pBuf = new char[size];
				memset(pBuf, 0, size);
				return (SimpleReplyHeader_Read*)(pBuf);
			}

			static void free(SimpleReplyHeader_Read* p)
			{
				char* p1 = (char*)p;
				delete[] p1;
			}
		};

		struct SimpleReplyHeader_GetLastBlockHeader
		{
			static SimpleReplyHeader_GetLastBlockHeader* alloc()
			{
				size_t s = sizeof(SimpleReplyHeader_GetLastBlockHeader);
				char* pBuf = new char[s];
				memset(pBuf, 0, s);
				return (SimpleReplyHeader_GetLastBlockHeader*)(pBuf);
			}

			static void free(SimpleReplyHeader_GetLastBlockHeader* p)
			{
				char* p1 = (char*)p;
				delete[] p1;
			}

			SimpleReplyHeader h;
			BlockId latestBlock;
		};

		// internal types
		class SimpleKIDPair // represents <key,blockId>
		{
		public:
			const SimpleKey key;
			const BlockId blockId;

			SimpleKIDPair(SimpleKey s, BlockId i) : key(s), blockId(i)
			{
			}

			bool operator<(const SimpleKIDPair& k) const
			{
				int c = memcmp((char*)&this->key, (char*)&k.key, sizeof(SimpleKey));
				if (c == 0)
					return this->blockId > k.blockId;
				else
					return c < 0;
			}

			bool operator==(const SimpleKIDPair& k) const
			{
				if (this->blockId != k.blockId) return false;
				int c = memcmp((char*)&this->key, (char*)&k.key, sizeof(SimpleKey));
				return (c == 0);
			}
		};

		static void print(SimpleRequestHeader* r)
		{
			if (r->type == 1)
			{
				SimpleConditionalWriteHeader* p = (SimpleConditionalWriteHeader*)r;
				printf("\n");
				printf("Write: version=%" PRId64 " numberOfWrites=%zu\n", p->readVerion, p->numberOfWrites);
				SimpleKV* pWritesKVArray = p->keyValArray();

				for (size_t i = 0; i < p->numberOfWrites; i++)
				{
					printf("%4s", " ");
					printf("< ");
					for (int k = 0; k < KV_LEN; k++){
						printf("%02X", pWritesKVArray[i].key[k]);
					}
					printf(" ; ");
					for (int k = 0; k < KV_LEN; k++){ 
						printf("%02X", pWritesKVArray[i].val[k]);
					}
					printf(" >");
					printf("\n");
				}
			}
			else if (r->type == 2)
			{
				SimpleReadHeader* p = (SimpleReadHeader*)r;
				printf("\n");
				printf("Read: version=%" PRId64 " numberOfKeysToRead=%zu\n", p->readVerion, p->numberOfKeysToRead);

				for (size_t i = 0; i < p->numberOfKeysToRead; i++)
				{
					printf("%4s", " ");
					printf("< ");
					for (int k = 0; k < KV_LEN; k++){
						printf("%02X", p->keys[i].key[k]);
					}
					printf(" ; ");
					printf(" >");
					printf("\n");
				}
			}
			else if (r->type == 3)
			{

			}
			else
			{
				assert(0);
			}		
			printf("\n");
		}

		static void print(SimpleReplyHeader* r)
		{			
			if (r->type == 1)
			{
				SimpleReplyHeader_ConditionalWrite* p = (SimpleReplyHeader_ConditionalWrite*) r;
				printf("\n");
				printf("Write reply: latestBlock=%zu\n", p->latestBlock);
			}
			else if (r->type == 2)
			{
				SimpleReplyHeader_Read* p = (SimpleReplyHeader_Read*)r;
				printf("\n");
				printf("Read reply: numOfelements=%zu\n", p->numberOfElements);
				for (size_t i = 0; i < p->numberOfElements; i++)
				{
					printf("%4s", " ");
					printf("< ");
					for (int k = 0; k < KV_LEN; k++){
						printf("%02X", p->elements[i].key[k]);
					}
					printf(" ; ");
					for (int k = 0; k < KV_LEN; k++){ 
						printf("%02X", p->elements[i].val[k]);
					}
					printf(" >");
					printf("\n");
				}
			}
			else if (r->type == 3)
			{

			}
			else
			{
				assert(0);
			}
			printf("\n");
		}

		class InternalTestsBuilder
		{
			friend void BasicRandomTests::run(IClient* client);

			static InternalTestsBuilder createRandomTest(IClient* client)
			{
				InternalTestsBuilder t(1);
				t.write(client, "k1", "v1");
				t.write(client, "k2", "v2");
				t.read(client, "k1");
				t.read(client, "k2");
				t.create(client);

				for (map<BlockId, SimpleBlock*>::iterator it = t.m_internalBlockchain.begin(); it != t.m_internalBlockchain.end(); it++)
					SimpleBlock::free(it->second);

				return t;
			}

			static void free(std::list<SimpleRequestHeader*>& outRequests, std::list<SimpleReplyHeader*>& outReplies)
			{
				for (list<SimpleRequestHeader*>::iterator it = outRequests.begin(); it != outRequests.end(); it++)
					SimpleRequestHeader::free(*it);

				for (list<SimpleReplyHeader*>::iterator it = outReplies.begin(); it != outReplies.end(); it++)
					SimpleReplyHeader::free(*it);
			}

			std::list<SimpleRequestHeader*> m_requests;
			std::list<SimpleReplyHeader*> m_replies;

			std::map<BlockId, SimpleBlock*> m_internalBlockchain;
			std::map<SimpleKIDPair, SimpleVal> m_map;

			BlockId m_lastBlockId;

			InternalTestsBuilder(int64_t testPrefix) //: m_testPrefix(testPrefix)
			{
				m_lastBlockId = 0;
			}

			void create(IClient* client)
			{
				for (std::map<BlockId, SimpleBlock*>::iterator it = m_internalBlockchain.begin();
					it != m_internalBlockchain.end(); it++)
				{
					BlockId bId = it->first;
					SimpleBlock* block = it->second;
					(void)bId;
					(void)block;

					assert(bId == block->id);
				}
			}

			void write(IClient* client, std::string k, std::string v)
			{
				// Create request

				size_t numberOfWrites = 1;
				size_t numberOfKeysInReadSet = 0;

				SimpleConditionalWriteHeader* pHeader = SimpleConditionalWriteHeader::alloc(numberOfKeysInReadSet, numberOfWrites);

				// fill request
				pHeader->h.type = 1;
				pHeader->readVerion = 0;
				pHeader->numberOfKeysInReadSet = numberOfKeysInReadSet;
				pHeader->numberOfWrites = numberOfWrites;
				SimpleKey* pReadKeysArray = pHeader->readSetArray();
				SimpleKV*  pWritesKVArray = pHeader->keyValArray();

				for (size_t i = 0; i < numberOfKeysInReadSet; i++)
				{
					size_t k = rand() % NUMBER_OF_KEYS;
					memcpy(pReadKeysArray[i].key /*+ sizeof(int64_t)*/, &k, sizeof(size_t));
				}

				strcpy(pWritesKVArray[0].key, k.c_str());
				strcpy(pWritesKVArray[0].val, v.c_str());

				print((SimpleRequestHeader*)pHeader);
				Slice command((const char*)pHeader, sizeOfReq((SimpleRequestHeader*)pHeader));
				Slice reply;

				printf("--- invokeCommandSynch ===\n");
				client->invokeCommandSynch(command, false, reply);
				print((SimpleReplyHeader*)reply.data);
				printf("==== invokeCommandSynch ===\n");
				client->release(reply);

				// // look for conflicts
				// bool foundConflict = false;
				// for (BlockId i = readVer + 1; (i <= m_lastBlockId) && !foundConflict; i++)
				// {
				// 	SimpleBlock* currBlock = m_internalBlockchain[i];

				// 	for (size_t a = 0; (a < numberOfKeysInReadSet) && !foundConflict; a++)
				// 		for (size_t b = 0; (b < currBlock->numberOfItems) && !foundConflict; b++)
				// 		{
				// 			if (memcmp(pReadKeysArray[a].key, currBlock->items[b].key, KV_LEN) == 0)
				// 				foundConflict = true;
				// 		}
				// 	}

				// // if needed, add new block into the blockchain
				// 	if (!foundConflict)
				// 	{
				// 		m_lastBlockId++;

				// 		const size_t N = pHeader->numberOfWrites;

				// 		SimpleBlock* pNewBlock = SimpleBlock::alloc(N);

				// 		pNewBlock->id = m_lastBlockId;
				// 		pNewBlock->numberOfItems = N;

				// 		for (size_t i = 0; i < N; i++)
				// 		{
				// 			pNewBlock->items[i] = pWritesKVArray[i];

				// 			SimpleKey sk;
				// 			memcpy(sk.key, pWritesKVArray[i].key, KV_LEN);

				// 			SimpleVal sv;
				// 			memcpy(sv.v, pWritesKVArray[i].val, KV_LEN);

				// 			SimpleKIDPair kiPair(sk, m_lastBlockId);
				// 			m_map[kiPair] = sv;
				// 		}

				// 		m_internalBlockchain[m_lastBlockId] = pNewBlock;
				// 	}

				// 	return pHeader;
			}

			void read(IClient* client, std::string k)
			{
				// Create request
				size_t numberOfReads = 1;

				SimpleReadHeader* pHeader = SimpleReadHeader::alloc(numberOfReads);

				// fill request
				pHeader->h.type = 2;
				pHeader->readVerion = 10;
				pHeader->numberOfKeysToRead = numberOfReads;

				strcpy(pHeader->keys[0].key, k.c_str());

				// add request to m_requests
					// m_requests.push_back((SimpleRequestHeader*)pHeader);
				print((SimpleRequestHeader*)pHeader);
				Slice command((const char*)pHeader, sizeOfReq((SimpleRequestHeader*)pHeader));
				Slice reply;

				printf("--- invokeCommandSynch ===\n");
				client->invokeCommandSynch(command, true, reply);
				print((SimpleReplyHeader*)reply.data);
				printf("==== invokeCommandSynch ===\n");
				client->release(reply);
			}

			size_t sizeOfReq(SimpleRequestHeader* req)
			{
				if (req->type == 1)
				{
					SimpleConditionalWriteHeader* p = (SimpleConditionalWriteHeader*)req;
					return p->size();
				}
				else if (req->type == 2)
				{
					SimpleReadHeader* p = (SimpleReadHeader*)req;
					return p->size();
				}
				else if (req->type == 3)
				{
					return SimpleGetLastBlockHeader::size();
				}
				assert(0); 
				return 0;
			}
		};

		class InternalCommandsHandler : public ICommandsHandler
		{
		public:

			virtual bool executeCommand(const Slice command,
				const ILocalKeyValueStorageReadOnly& roStorage,
				IBlocksAppender& blockAppender,
				const size_t maxReplySize,
				char* outReply, size_t& outReplySize) const
			{
				printf("Got message of size %zu\n",command.size);

				printf("==== executeCommand\n");

				//DEBUG_RNAME("InternalCommandsHandler::executeCommand");
				if (command.size < sizeof(SimpleRequestHeader))
				{
					CHECK(false, "small message");
					return false;
				}
				SimpleRequestHeader* p = (SimpleRequestHeader*)command.data;
				if (p->type != 1) return executeReadOnlyCommand(command, roStorage, maxReplySize, outReply, outReplySize);

				// conditional write

				if (command.size < sizeof(SimpleConditionalWriteHeader))
				{
					CHECK(false, "small message");
					return false;
				}
				SimpleConditionalWriteHeader* pCondWrite = (SimpleConditionalWriteHeader*)command.data;

				print((SimpleRequestHeader*)pCondWrite);

				if (command.size < pCondWrite->size())
				{
					CHECK(false, "small message");
					return false;
				}

				SimpleKey* readSetArray = pCondWrite->readSetArray();

				BlockId currBlock = roStorage.getLastBlock();

				// look for conflicts
				bool hasConflict = false;
				for (size_t i = 0; !hasConflict && i < pCondWrite->numberOfKeysInReadSet; i++)
				{
					Slice key(readSetArray[i].key, KV_LEN);
					roStorage.mayHaveConflictBetween(key, pCondWrite->readVerion + 1, currBlock, hasConflict);
				}

				if (!hasConflict)
				{
					SimpleKV* keyValArray = pCondWrite->keyValArray();
					SetOfKeyValuePairs updates;

					//printf("\nAdding BlockId=%" PRId64 " ", currBlock + 1);

					for (size_t i = 0; i < pCondWrite->numberOfWrites; i++)
					{
						Slice key(keyValArray[i].key, KV_LEN);
						Slice val(keyValArray[i].val, KV_LEN);
						KeyValuePair kv(key, val);
						updates.insert(kv);
						//printf("\n");
						//for (int k = 0; k < sizeof(size_t); k++)
						//	printf("%02X", key.data()[k]);
						//printf("%04s", " ");
						//for (int k = 0; k < sizeof(size_t); k++)
						//	printf("%02X", val.data()[k]);

					}
					//printf("\n\n");
					BlockId newBlockId = 0;
					Status addSucc = blockAppender.addBlock(updates, newBlockId);
					assert(addSucc.ok());
					assert(newBlockId == currBlock + 1);
				}

				assert(sizeof(SimpleReplyHeader_ConditionalWrite) <= maxReplySize);
				SimpleReplyHeader_ConditionalWrite* pReply = (SimpleReplyHeader_ConditionalWrite*)outReply;
				memset(pReply, 0, sizeof(SimpleReplyHeader_ConditionalWrite));
				pReply->h.type = 1;
				pReply->succ = (!hasConflict);
				if (!hasConflict)
					pReply->latestBlock = currBlock + 1;
				else
					pReply->latestBlock = currBlock;

				outReplySize = sizeof(SimpleReplyHeader_ConditionalWrite);
				return true;
			}

			virtual bool executeReadOnlyCommand(const Slice command,
				const ILocalKeyValueStorageReadOnly& roStorage,
				const size_t maxReplySize,
				char* outReply, size_t& outReplySize) const
			{
				printf("==== executeReadOnlyCommand\n");
				if (command.size < sizeof(SimpleRequestHeader))
				{
					CHECK(false, "small message");
					return false;
				}
				SimpleRequestHeader* p = (SimpleRequestHeader*)command.data;
				if (p->type == 2)
				{
					// read
					if (command.size < sizeof(SimpleReadHeader))
					{
						CHECK(false, "small message");
						return false;
					}
					SimpleReadHeader* pRead = (SimpleReadHeader*)command.data;
					print((SimpleRequestHeader*)pRead);

					if (command.size < pRead->size())
					{
						CHECK(false, "small message");
						return false;
					}
					size_t numOfElements = pRead->numberOfKeysToRead;
					size_t replySize = SimpleReplyHeader_Read::size(numOfElements);

					if (maxReplySize < replySize)
					{
						CHECK(false, "small message");
						return false;
					}

					SimpleReplyHeader_Read* pReply = (SimpleReplyHeader_Read*)(outReply);
					outReplySize = replySize;
					memset(pReply, 0, replySize);
					pReply->h.type = 2;
					pReply->numberOfElements = numOfElements;

					SimpleKey* keysArray = pRead->keysArray();
					for (size_t i = 0; i < numOfElements; i++)
					{
						memcpy(pReply->elements[i].key, keysArray[i].key, KV_LEN);
						Slice val;
						Slice k(keysArray[i].key, KV_LEN);
						BlockId outBlock = 0;
						roStorage.get(pRead->readVerion, k, val, outBlock);
						if (val.size > 0)
							memcpy(pReply->elements[i].val, val.data, KV_LEN);
						else
							memset(pReply->elements[i].val, 0, KV_LEN);
					}

					return true;


				}
				else if (p->type == 3)
				{
					// read
					if (command.size < sizeof(SimpleGetLastBlockHeader))
					{
						CHECK(false, "small message");
						return false;
					}

					if (maxReplySize < sizeof(SimpleReplyHeader_GetLastBlockHeader))
					{
						CHECK(false, "small message");
						return false;
					}

					SimpleReplyHeader_GetLastBlockHeader* pReply = (SimpleReplyHeader_GetLastBlockHeader*)(outReply);
					outReplySize = sizeof(SimpleReplyHeader_GetLastBlockHeader);
					memset(pReply, 0, sizeof(SimpleReplyHeader_GetLastBlockHeader));
					pReply->h.type = 3;
					pReply->latestBlock = roStorage.getLastBlock();

					return true;
				}
				else
				{
					outReplySize = 0;
					CHECK(false, "illegal message");
					return false;
				}
			}
		};

		static size_t sizeOfRep(SimpleReplyHeader* rep)
		{
			if (rep->type == 1)
			{
				return sizeof(SimpleReplyHeader_ConditionalWrite);
			}
			else if (rep->type == 2)
			{
				SimpleReplyHeader_Read* p = (SimpleReplyHeader_Read*)rep;
				return p->size();
			}
			else if (rep->type == 3)
			{
				return sizeof(SimpleReplyHeader_GetLastBlockHeader);
			}
			assert(0); 
			return 0;
		}


		static void verifyEmptyBlockchain(IClient* client)
		{
			SimpleGetLastBlockHeader* p = SimpleGetLastBlockHeader::alloc();
			p->h.type = 3;
			Slice command((const char*)p, sizeof(SimpleGetLastBlockHeader));
			Slice reply;

			client->invokeCommandSynch(command, true, reply);

			assert(reply.size == sizeof(SimpleReplyHeader_GetLastBlockHeader));

			SimpleReplyHeader_GetLastBlockHeader* pReplyData = (SimpleReplyHeader_GetLastBlockHeader*)reply.data;
			(void)pReplyData;

			assert(pReplyData->h.type == 3);
			assert(pReplyData->latestBlock == 0);

			client->release(reply);
		}
	}

	InternalTestsBuilder run(IClient* client)
	{
		assert(!client->isRunning());

		client->start();

		InternalTestsBuilder t = Internal::InternalTestsBuilder::createRandomTest(client);

		// client->stop();

		// getchar();
		return t;
	}


	ICommandsHandler* commandsHandler()
	{
		return new Internal::InternalCommandsHandler();
	}
}
