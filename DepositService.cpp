#define RAPIDJSON_HAS_STDSTRING 1

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include <iostream>
#include <map>
#include <string>
#include <sstream>

#include "DepositService.h"
#include "Database.h"
#include "ClientError.h"
#include "HTTPClientResponse.h"
#include "HttpClient.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"

using namespace rapidjson;
using namespace std;

DepositService::DepositService() : HttpService("/deposits") { }

void DepositService::post(HTTPRequest *request, HTTPResponse *response) {
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

  // parse request body
  StringUtils string_util;
  vector<string> info_list = string_util.split(request->getBody(), '&');

  Deposit *dp = new Deposit();
  dp->to = this->m_db->auth_tokens[auth_token];
  dp->amount = 0;
  dp->stripe_charge_id = "";
  string stripe_token = "";

  for (string info: info_list) {
    vector<string> kv = string_util.split(info, '=');
    if (kv[0] == "amount") {
      dp->amount = atoi(kv[1].c_str());
    } else if (kv[0] == "stripe_token") {
      stripe_token = kv[1];
    } else {
      throw ClientError::badRequest;
    }
  }

  string username = this->m_db->auth_tokens[auth_token]->username;

  // from the gunrock server to Stripe
  HttpClient client("api.stripe.com", 443, true);
  client.set_basic_auth(m_db->stripe_secret_key, "");
  WwwFormEncodedDict body;
  body.set("amount", dp->amount);
  body.set("currency", "usd");
  body.set("source", stripe_token);
  string encoded_body = body.encode();
  HTTPClientResponse *client_response = client.post("/v1/charges", encoded_body);

  Document *d = client_response->jsonBody();
  dp->stripe_charge_id = (*d)["id"].GetString();
  delete d;

  #ifdef _TESTING
  cout << "Stripe Charge ID: " << dp->stripe_charge_id << endl;
  #endif

  this->m_db->deposits.push_back(dp);
  this->m_db->users[username]->balance += dp->amount;


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
  for (Deposit *record: this->m_db->deposits) {
    if (record->to->username != username) {
      continue;
    }
    Value to;
    to.SetObject();
    to.AddMember("to", username, a);
    to.AddMember("amount", record->amount, a);
    to.AddMember("stripe_charge_id", record->stripe_charge_id, a);
    array.PushBack(to, a);
  }
  // and add the array to our return object
  o.AddMember("deposits", array, a);

  // now some rapidjson boilerplate for converting the JSON object to a string
  document.Swap(o);
  StringBuffer buffer;
  PrettyWriter<StringBuffer> writer(buffer);
  document.Accept(writer);

  // set the return object
  response->setContentType("application/json");
  response->setBody(buffer.GetString() + string("\n"));
}

void DepositService::get(HTTPRequest *request, HTTPResponse *response) {

}
