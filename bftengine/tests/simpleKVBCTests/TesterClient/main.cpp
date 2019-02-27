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

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>

#include "KVBCInterfaces.h"
#include "simpleKVBCTests.h"
#include "CommFactory.hpp"
#include "TestDefs.h"

#ifndef _WIN32
#include <sys/param.h>
#include <unistd.h>
#else
#include "winUtils.h"
#endif

#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>

#include "concord.grpc.pb.h"

using namespace SimpleKVBC;

using std::string;

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using msgs::GetRequest;
using msgs::GetReply;
using msgs::SetRequest;
using msgs::SetReply;
using msgs::DeleteRequest;
using msgs::DeleteReply;
using msgs::InitRequest;
using msgs::InitReply;
using msgs::Concord;

// Logic and data behind the server's behavior.
class ConcordServiceImpl final : public Concord::Service {
private: 
  IClient* c;
  
public: 

  ConcordServiceImpl(IClient* rc) 
  { 
    c = rc;
  } 
  grpc::Status Get(ServerContext* context, const GetRequest* request,
    GetReply* reply) override {

    // const char *k = request->key().c_str();

    // redisReply *pRedisReply = (redisReply*)redisCommand(c, "GET %s", k);

    // if(pRedisReply->len > 0){
    //   std::cout << "received Get request [" << request->key() << ":" << pRedisReply->str << "]" << std::endl;
    //   reply->set_value(std::string(pRedisReply->str, pRedisReply->len));
    // }

    // freeReplyObject(pRedisReply); 

    return grpc::Status::OK;
  }

  grpc::Status Set(ServerContext* context, const SetRequest* request,
    SetReply* reply) override {
    std::cout << "received Set request [" << request->key() << ":" << request->value() << "]" << std::endl;

    // const char *k = request->key().c_str();
    // const char *v = request->value().c_str();
    // redisReply *pRedisReply = (redisReply*)redisCommand(c, "SET %s %b", k, v, request->value().length());
    
    // freeReplyObject(pRedisReply); 

    return grpc::Status::OK;
  }

  grpc::Status Delete(ServerContext* context, const DeleteRequest* request,
    DeleteReply* reply) override {

    std::cout << "received Delete request [" << request->key() << "]" << std::endl;

    // const char *k = request->key().c_str();
    // redisReply *pRedisReply = (redisReply*)redisCommand(c, "DEL %s", k);
    
    // freeReplyObject(pRedisReply); 

    return grpc::Status::OK;
  }

  grpc::Status Init(ServerContext* context, const InitRequest* request,
    InitReply* reply) override {

    // std::cout << "received Init request " << std::endl;
    
    // redisReply *pRedisReply = (redisReply*)redisCommand(c, "flushall");
    
    // freeReplyObject(pRedisReply); 

    return grpc::Status::OK;
  }
};

void RunServer(IClient* c) {
  std::string server_address("0.0.0.0:50051");

  ConcordServiceImpl service(c);

  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
  BasicRandomTests::Stub stub(c);
}

																						 
int main(int argc, char **argv) {
#if defined(_WIN32)
	initWinSock();
#endif

	char argTempBuffer[PATH_MAX + 10];

	uint16_t clientId = UINT16_MAX;
	uint16_t fVal = UINT16_MAX;
	uint16_t cVal = UINT16_MAX;
	uint32_t numOfOps = UINT32_MAX;

	int o = 0;
	while ((o = getopt(argc, argv, "i:f:c:p:")) != EOF) {
		switch (o) {
		case 'i':
		{
			strncpy(argTempBuffer, optarg, sizeof(argTempBuffer) - 1);
			argTempBuffer[sizeof(argTempBuffer) - 1] = 0;
			string idStr = argTempBuffer;
			int tempId = std::stoi(idStr);
			if (tempId >= 0 && tempId < UINT16_MAX)
				clientId = (uint16_t)tempId;
			// TODO: check clientId
		}
		break;

		case 'f':
		{
			strncpy(argTempBuffer, optarg, sizeof(argTempBuffer) - 1);
			argTempBuffer[sizeof(argTempBuffer) - 1] = 0;
			string fStr = argTempBuffer;
			int tempfVal = std::stoi(fStr);
			if (tempfVal >= 1 && tempfVal < UINT16_MAX)
				fVal = (uint16_t)tempfVal;
			// TODO: check fVal
		}
		break;

		case 'c':
		{
			strncpy(argTempBuffer, optarg, sizeof(argTempBuffer) - 1);
			argTempBuffer[sizeof(argTempBuffer) - 1] = 0;
			string cStr = argTempBuffer;
			int tempcVal = std::stoi(cStr);
			if (tempcVal >= 0 && tempcVal < UINT16_MAX)
				cVal = (uint16_t)tempcVal;
			// TODO: check cVal
		}
		break;

		case 'p':
		{
			strncpy(argTempBuffer, optarg, sizeof(argTempBuffer) - 1);
			argTempBuffer[sizeof(argTempBuffer) - 1] = 0;
			string numOfOpsStr = argTempBuffer;
			int tempfVal = std::stoi(numOfOpsStr);
			if (tempfVal >= 1 && tempfVal < UINT32_MAX)
				numOfOps = (uint32_t)tempfVal;
			// TODO: check numOfOps
		}
		break;


		default:
			// nop
			break;
		}
	}

	if (clientId == UINT16_MAX ||
		fVal == UINT16_MAX ||
		cVal == UINT16_MAX	||
		numOfOps == UINT32_MAX
		)
	{
		fprintf(stderr, "%s -f F -c C -p NUM_OPS -i ID", argv[0]);
		exit(-1);
	}

	// TODO: check arguments 

	const uint16_t numOfReplicas = 3 * fVal + 2 * cVal + 1;
	const uint16_t port = basePort + clientId * 2;	 

	std::unordered_map <NodeNum, NodeInfo> nodes;
	for (int i = 0; i < (numOfReplicas + numOfClientProxies); i++) {					
		nodes.insert({
		i,
		NodeInfo{ ipAddress, (uint16_t)(basePort + i * 2), i < numOfReplicas } });
	}

	bftEngine::PlainUdpConfig commConfig(ipAddress, port, maxMsgSize, nodes, clientId);
	bftEngine::ICommunication* comm = bftEngine::CommFactory::create(commConfig);

	ClientConfig config;

	config.clientId = clientId;
	config.fVal = fVal;
	config.cVal = cVal;
	config.maxReplySize = maxMsgSize;

	IClient* c = createClient(config, comm);	

	RunServer(c);	 

	// BasicRandomTests::run(c, numOfOps);
}