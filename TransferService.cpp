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
  string auth_token;
  if (request->hasAuthToken()) {
    auth_token = request->getAuthToken();
  } else {
    throw ClientError::badRequest;
  }

  // checkout if auth token is in data base
  if (this->m_db->auth_tokens.count(auth_token) == 0) {
    throw ClientError::notFound;
  }

  string username = this->m_db->auth_tokens[auth_token]->username;

  // parse request body
  StringUtils string_util;
  vector<string> info_list = string_util.split(request->getBody(), '&');

  Transfer *tr = new Transfer();

  tr->to = NULL;
  tr->from = this->m_db->users[username];
  tr->amount = 0;

  for (string info: info_list) {
    vector<string> kv = string_util.split(info, '=');
    if (kv[0] == "amount") {
      tr->amount = atoi(kv[1].c_str());
    } else if (kv[0] == "to") {
      tr->to = this->m_db->users[kv[1]];
    } else {
      throw ClientError::badRequest;
    }
  }

  // make sure the sender have enough money
  if (tr->from->balance < tr->amount) {
    throw ClientError::methodNotAllowed;
  }

  tr->to->balance += tr->amount;
  tr->from->balance -= tr->amount;

  // save to db
  this->m_db->transfers.push_back(tr);

  // construct response
  Document document;
  Document::AllocatorType& a = document.GetAllocator();
  Value o;
  o.SetObject();
  o.AddMember("balance", this->m_db->users[username]->balance, a);

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
