#include <iostream>

#include <stdlib.h>
#include <stdio.h>

#include "HttpService.h"
#include "ClientError.h"
#include "StringUtils.h"

using namespace std;

HttpService::HttpService(string pathPrefix) {
  this->m_pathPrefix = pathPrefix;
}

User *HttpService::getAuthenticatedUser(HTTPRequest *request)  {
  // check for auth token
  string auth_token;
  if (request->hasAuthToken()) {
    auth_token = request->getAuthToken();
  } else {
    throw ClientError::unauthorized();
  }

  // if this auth_token is not in db, throw an not found error
  if (this->m_db->auth_tokens.count(auth_token) == 0) {
    #ifdef _TESTING_
    cout << "here: " << auth_token << endl;
    this->check_db();
    #endif
    throw ClientError::notFound();
  }

  return this->m_db->auth_tokens[auth_token];
}

void HttpService::checkUserID(HTTPRequest *request) {
  // check if this token belongs to the user
  string user_id;
  try {
    vector<string> path = request->getPathComponents();
    if (path.size() != 2) {
      throw ClientError::forbidden();
    }
    user_id = request->getPathComponents()[1];
  } catch (...) {
    throw ClientError::forbidden();
  }
  if (user_id != this->m_db->auth_tokens[request->getAuthToken()]->user_id) {
    throw ClientError::forbidden();
  }
}

string HttpService::pathPrefix() {
  return m_pathPrefix;
}

void HttpService::head(HTTPRequest *request, HTTPResponse *response) {
  cout << "HEAD " << request->getPath() << endl;
  throw ClientError::methodNotAllowed();
}

void HttpService::get(HTTPRequest *request, HTTPResponse *response) {
  cout << "GET " << request->getPath() << endl;
  throw ClientError::methodNotAllowed();
}

void HttpService::put(HTTPRequest *request, HTTPResponse *response) {
  cout << "PUT " << request->getPath() << endl;
  throw ClientError::methodNotAllowed();
}

void HttpService::post(HTTPRequest *request, HTTPResponse *response) {
  cout << "POST " << request->getPath() << endl;
  throw ClientError::methodNotAllowed();
}

void HttpService::del(HTTPRequest *request, HTTPResponse *response) {
  cout << "DELETE " << request->getPath() << endl;
  throw ClientError::methodNotAllowed();
}

