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

 /**
  * @brief Write info of a user to HTTP response in Account service standard.
  * Account Service Standard Response Body:
  * {
  *   "balance": <int>,
  *   "email": <str>
  * }
  * 
  * @param response HTTP Response object to write in.
  * @param user  User object providing info.
  */
  void writeHTTPResponse(HTTPResponse *response, User *user); 
};

#endif
