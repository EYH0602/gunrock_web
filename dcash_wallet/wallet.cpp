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

void apply_function(vector<string> argv) {
  int balance;
  if (argv[0] == "auth") {
    if (argv.size() != 4) {
      exit(1);
    }
    balance = auth(argv[1], argv[2], argv[3]);
  }
  cout << "Balance: $" << balance << ".00" << endl;
}

/**
 * Display the prompt at begining of line.
 * @param prompt Your prompt with ending charactor '\0'.
 */
#define SHOW_PROMPT(prompt) write(STDOUT_FILENO, prompt, strlen(prompt))

/* Command parser and Function caller */
void start_cli() {
  string prompt = "D$> ";
  size_t cap = 0;
  ssize_t len;
  char* buff = NULL;
  string command;
  vector<string> argv;

  // read from file/STDIN line by line
  while (SHOW_PROMPT(prompt.c_str()), (len = getline(&buff, &cap, stdin)) > 0) {
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
    cout << "could not open config.json" << endl;
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

  start_cli();

  return 0;
}
