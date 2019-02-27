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

#pragma once 

#include "KVBCInterfaces.h"
#include <string>

using namespace SimpleKVBC;
using std::string;

namespace BasicRandomTests
{
  class IStub;

  class IStub
  {
  public:
    virtual std::string read(IClient* client, std::string k) = 0;
    virtual void write(IClient* client, std::string k, std::string v) = 0;
  };

  ICommandsHandler* commandsHandler();
  IStub* newStub();
}