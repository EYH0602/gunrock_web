#define RAPIDJSON_HAS_STDSTRING 1

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "AuthService.h"
#include "StringUtils.h"
#include "ClientError.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"

using namespace std;
using namespace rapidjson;


AuthService::AuthService() : HttpService("/auth-tokens") {
  
}

void AuthService::post(HTTPRequest *request, HTTPResponse *response) {
  // parse the request body
  User *user;
  try {
    user = this->getAuthenticatedUser(request);
  } catch (ClientError &ce) {
    response->setStatus(ce.status_code);
  } catch (...) {

  }

  StringUtils string_util;
  string auth_token;

  // If the username doesn't exist this call will create a new user
  if (this->m_db->users.count(user->username) == 0) {
    this->m_db->users[user->username] = user;
    this->m_db->auth_tokens[auth_token] = user;
  }
  // if the password matches, log in the user
  if (this->m_db->users[user->username]->password == user->password){
    auth_token = string_util.createAuthToken();
    this->m_db->auth_tokens[auth_token] = user;
  } else {
    throw ClientError::notFound;
  }

  // example JSON API usage from doc
  Document document;
  Document::AllocatorType& a = document.GetAllocator();
  Value o;
  o.SetObject();
  o.AddMember("auth_token", auth_token, a);
  o.AddMember("user_id", user->user_id, a);

  // now some rapidjson boilerplate for converting the JSON object to a string
  document.Swap(o);
  StringBuffer buffer;
  PrettyWriter<StringBuffer> writer(buffer);
  document.Accept(writer);

  // set the return object
  response->setContentType("application/json");
  response->setBody(buffer.GetString() + string("\n"));

  #ifdef _TESTING_
  cout << "Login: " << user->username << endl;
  cout << "    user id: " << user->user_id << endl;
  cout << "    token: " << auth_token << endl;
  #endif
}

void AuthService::del(HTTPRequest *request, HTTPResponse *response) {
  string auth_token;
  if (request->hasAuthToken()) {
    auth_token = request->getAuthToken();
  } else {
    throw ClientError::notFound;
  }
  this->m_db->auth_tokens.erase(auth_token);

  #ifdef _TESTING_
  cout << "Logout: " << auth_token << endl;
  #endif
}
