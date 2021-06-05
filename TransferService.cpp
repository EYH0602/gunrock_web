#define RAPIDJSON_HAS_STDSTRING 1

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include <iostream>
#include <map>
#include <string>
#include <sstream>

#include "TransferService.h"
#include "ClientError.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"

using namespace rapidjson;
using namespace std;

TransferService::TransferService() : HttpService("/transfers") { }


void TransferService::post(HTTPRequest *request, HTTPResponse *response) {
  // check for auth token
  User *user;
  try {
    user = this->getAuthenticatedUser(request);
  } catch (ClientError &ce) {
    throw ce;
  } catch (...) {}
  string auth_token = request->getAuthToken();

  // check if tr->from has sufficient balance
  if (user->balance <= 0) {
    throw ClientError::methodNotAllowed();
  }

  string username = user->username;

  // parse request body
  StringUtils string_util;
  vector<string> info_list = string_util.split(request->getBody(), '&');

  // init a Transfer object for use
  Transfer *tr = new Transfer();
  tr->to = NULL;
  tr->from = user;
  tr->amount = 0;

  for (string info: info_list) {
    vector<string> kv = string_util.split(info, '=');
    if (kv[0] == "amount") {
      int amount = atoi(kv[1].c_str());
      // make sure the sender have enough money
      if (tr->from->balance < amount) {
        throw ClientError::badRequest();
      }
      tr->amount = amount;
    } else if (kv[0] == "to") {
      // check if the tr->to is in db
      if (this->m_db->users.count(kv[1]) == 0) {
        throw ClientError::notFound();
      }
      tr->to = this->m_db->users[kv[1]];
    } else {
      throw ClientError::badRequest();
    }
  }

  // make change in balance for both account (by refence)
  tr->to->balance += tr->amount;
  tr->from->balance -= tr->amount;

  // save transfer record to db
  this->m_db->transfers.push_back(tr);

  // construct response
  Document document;
  Document::AllocatorType& a = document.GetAllocator();
  Value o;
  o.SetObject();
  o.AddMember("balance", user->balance, a);

  // create an array
  Value array;
  array.SetArray();

  // add an object to our array
  for (Transfer *record: this->m_db->transfers) {
    if (record->from->username != username) {
      continue;
    }
    Value to;
    to.SetObject();
    to.AddMember("from", username, a);
    to.AddMember("to", record->to->username, a);
    to.AddMember("amount", record->amount, a);
    array.PushBack(to, a);
  }
  // and add the array to our return object
  o.AddMember("transfers", array, a);

  // now some rapidjson boilerplate for converting the JSON object to a string
  document.Swap(o);
  StringBuffer buffer;
  PrettyWriter<StringBuffer> writer(buffer);
  document.Accept(writer);

  // set the return object
  response->setContentType("application/json");
  response->setBody(buffer.GetString() + string("\n"));

}
