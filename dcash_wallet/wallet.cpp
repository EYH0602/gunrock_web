#define RAPIDJSON_HAS_STDSTRING 1

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "WwwFormEncodedDict.h"
#include "HttpClient.h"
#include "StringUtils.h"

#include "rapidjson/document.h"

using namespace std;
using namespace rapidjson;

int API_SERVER_PORT = 8080;
string API_SERVER_HOST = "localhost";
string PUBLISHABLE_KEY = "";

string auth_token;
string user_id;

StringUtils string_util;

int update_email(string email) {

  HttpClient *client = new HttpClient(API_SERVER_HOST.c_str(), API_SERVER_PORT);
  WwwFormEncodedDict encoder;

  HTTPClientResponse *response;
  encoder.set("email", email);
  string body = encoder.encode();
  string path = "/users/" + user_id;
  client->set_header("x-auth-token", auth_token);
  response = client->put(path, body);
  string rsp = response->body();

  delete response;
  delete client;

  Document d;
  d.Parse(rsp);
  assert(d["balance"].IsInt());

  return d["balance"].GetInt();
}

int auth(string username, string password, string email) {

  HttpClient *client = new HttpClient(API_SERVER_HOST.c_str(), API_SERVER_PORT);
  client->set_basic_auth(username, password);
  WwwFormEncodedDict encoder = WwwFormEncodedDict();
  encoder.set("username", username); 
  encoder.set("password", password);
  string body = encoder.encode();

  HTTPClientResponse *response;
  response = client->post("/auth-tokens", body);
  string rsp = response->body();
  delete response;

  Document d;
  d.Parse(rsp);
  assert(d["auth_token"].IsString());
  assert(d["user_id"].IsString());
  
  auth_token = d["auth_token"].GetString();
  user_id = d["user_id"].GetString();
  delete client;

  return update_email(email);
}

int get_balance() {

  // make a API request
  HttpClient *client = new HttpClient(API_SERVER_HOST.c_str(), API_SERVER_PORT);
  string path = "/users/" + user_id;
  client->set_header("x-auth-token", auth_token);
  HTTPClientResponse *response = client->get(path);
  string rsp = response->body();
  delete client;
  delete response;

  Document d;
  d.Parse(rsp);
  assert(d["balance"].IsInt());
  return d["balance"].GetInt();
}

int deposit(string amount, string card, string year, string month, string cvc) {
  // API call to string to get token
  // from the dcash wallet to Stripe
  HttpClient *client = new HttpClient("api.stripe.com", 443, true);
  client->set_header("Authorization", string("Bearer ") + PUBLISHABLE_KEY);
  client->set_basic_auth(PUBLISHABLE_KEY, "");

  WwwFormEncodedDict encoder;
  encoder.set("card[number]", card.c_str());
  encoder.set("card[exp_month]", atoi(month.c_str()));
  encoder.set("card[exp_year]", atoi(year.c_str()));
  encoder.set("card[cvc]", atoi(cvc.c_str()));
  string body = encoder.encode();

  HTTPClientResponse *client_response = client->post("/v1/tokens", body);
  // This method converts the HTTP body into a rapidjson document
  Document *d = client_response->jsonBody();
  string token = (*d)["id"].GetString();
  delete d;
  delete client;
  delete client_response;

  // send amount and token to API server
  client = new HttpClient(API_SERVER_HOST.c_str(), API_SERVER_PORT);
  encoder = WwwFormEncodedDict();
  encoder.set("stripe_token", token); 
  encoder.set("amount", atoi(amount.c_str()));
  body = encoder.encode();
  client->set_header("x-auth-token", auth_token);
  client_response = client->post("/deposits", body);

  d = client_response->jsonBody();
  assert((*d)["balance"].IsInt());
  return (*d)["balance"].GetInt();
}

void logout() {
  HttpClient *client = new HttpClient(API_SERVER_HOST.c_str(), API_SERVER_PORT);
  string path = "/auth-tokens/" + user_id;
  client->set_header("x-auth-token", auth_token);
  HTTPClientResponse *client_response = client->del(path);

  delete client;
  delete client_response;

  // the server show returns nothing
  exit(0);  // exit the program
}

void throw_error() {
  char error_message[30] = "Error\n";
  write(STDERR_FILENO, error_message, strlen(error_message));
}

int apply_function(vector<string> argv) {
  int balance = 0;
  if (argv[0] == "auth") {
    if (argv.size() != 4) {
      throw_error();
      return 1;
    }
    balance = auth(argv[1], argv[2], argv[3]);
  } else if (argv[0] == "balance") {
    if (argv.size() != 1) {
      throw_error();
      return 1;
    }
    balance = get_balance();
  } else if (argv[0] == "deposit") {
    if (argv.size() != 6) {
      throw_error();
      return 1;
    }
    balance = deposit(argv[1], argv[2], argv[3], argv[4], argv[5]);
  } else if (argv[0] == "logout") {
    if (argv.size() != 1) {
      throw_error();
      return 1;
    }
    logout();
  }

  cout << "Balance: $" << balance << ".00" << endl;
  return 0;
}

/**
 * Display the prompt at begining of line.
 * @param prompt Your prompt with ending charactor '\0'.
 */
#define SHOW_PROMPT(prompt) write(STDOUT_FILENO, prompt, strlen(prompt))

/* Command parser and Function caller */
void start_cli(FILE* fp) {
  string prompt = (fp == stdin) ? "D$> " : "";
  size_t cap = 0;
  ssize_t len;
  char* buff = NULL;
  string command;
  vector<string> argv;

  // read from file/STDIN line by line
  while (SHOW_PROMPT(prompt.c_str()), (len = getline(&buff, &cap, fp)) > 0) {
    if (len == 1 && buff[len] == '\0') {
      continue;
    }
    buff[len-1] = '\0';
    command = string(buff);
    argv = string_util.split(command, ' ');
    apply_function(argv);
  }
}


int main(int argc, char *argv[]) {
  stringstream config;
  int fd = open("config.json", O_RDONLY);
  if (fd < 0) {
    cerr << "could not open config.json" << endl;
    exit(1);
  }
  int ret;
  char buffer[4096];
  while ((ret = read(fd, buffer, sizeof(buffer))) > 0) {
    config << string(buffer, ret);
  }
  Document d;
  d.Parse(config.str());
  API_SERVER_PORT = d["api_server_port"].GetInt();
  API_SERVER_HOST = d["api_server_host"].GetString();
  PUBLISHABLE_KEY = d["stripe_publishable_key"].GetString();

  if (argc > 2) {
    cerr << "Usage: ./dcash [batch file]" << endl;
    exit(1);
  }

  FILE* fp = (argc == 1) ? stdin : fopen(argv[1], "r");
  if (!fp) {
    cerr << "batch file invaliad." << endl;
    exit(1);
  }

  start_cli(fp);

  return 0;
}
