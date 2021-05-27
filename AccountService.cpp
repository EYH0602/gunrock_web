#define RAPIDJSON_HAS_STDSTRING 1

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "AccountService.h"
#include "ClientError.h"
#include "StringUtils.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"

using namespace std;
using namespace rapidjson;

AccountService::AccountService() : HttpService("/users") {
  
}

void AccountService::writeHTTPResponse(HTTPResponse *response, User *user) {
  // example JSON API usage from doc
  Document document;
  Document::AllocatorType& a = document.GetAllocator();
  Value o;
  o.SetObject();
  o.AddMember("balance", user->balance, a);
  o.AddMember("email", user->email, a);

  // now some rapidjson boilerplate for converting the JSON object to a string
  document.Swap(o);
  StringBuffer buffer;
  PrettyWriter<StringBuffer> writer(buffer);
  document.Accept(writer);

  // set the return object
  response->setContentType("application/json");
  response->setBody(buffer.GetString() + string("\n"));
}

void AccountService::get(HTTPRequest *request, HTTPResponse *response) {
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
  string username = this->m_db->auth_tokens[auth_token]->username;
  User *user = this->m_db->users[username];

  this->writeHTTPResponse(response, user);

  #ifdef _TESTING_
  cout << "Set Email: " << endl;
  cout << "    user id: " << user->user_id << endl;
  cout << "    auth token: " << auth_token << endl;
  cout << "    email: " << user->email << endl;
  #endif

}

void AccountService::put(HTTPRequest *request, HTTPResponse *response) {
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
  string username = this->m_db->auth_tokens[auth_token]->username;
  User *user = this->m_db->users[username];
  vector<string> info = string_util.split(request->getBody(), '=');
  if (info[0] == "email") {
    user->email = info[1];
  }

  this->writeHTTPResponse(response, user);

  #ifdef _TESTING_
  cout << "Set Email: " << endl;
  cout << "    user id: " << user->user_id << endl;
  cout << "    auth token: " << auth_token << endl;
  cout << "    email: " << user->email << endl;
  #endif

}
