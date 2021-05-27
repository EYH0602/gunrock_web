#ifndef _ACCOUNTSERVICE_H_
#define _ACCOUNTSERVICE_H_

#include "HttpService.h"
#include "Database.h"

#include <string>

class AccountService : public HttpService {
 public:
  AccountService();

  virtual void get(HTTPRequest *request, HTTPResponse *response);
  virtual void put(HTTPRequest *request, HTTPResponse *response);

 private:
  void writeHTTPResponse(HTTPResponse *response, User *user); 
};

#endif
