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
bool is_loged_in = false;

StringUtils string_util;

int get_input_amount(string str) {
  vector<string> amounts = string_util.split(str, '.');
  int left = atoi(amounts[0].c_str()) * 100;
  if (amounts.size() == 1) {
    return left;
  }
  return left + atoi(amounts[1].c_str());
}

string get_output_amount(int n) {
  string str = to_string(n);

  if (n < 10) {
    return "0.0" + str;
  }
  if (n < 100) {
    return "0." + str;
  }

  string left = str.substr(0, str.length()-2);
  string right = str.substr(str.length()-2);
  return left + "." + right;
}

string update_email(string email) {

  HttpClient *client = new HttpClient(API_SERVER_HOST.c_str(), API_SERVER_PORT);
  WwwFormEncodedDict encoder;

  HTTPClientResponse *response;
  encoder.set("email", email);
  string body = encoder.encode();
  string path = "/users/" + user_id;
  client->set_header("x-auth-token", auth_token);
  response = client->put(path, body);
  string rsp = response->body();

  bool success = response->success();
  delete response;
  delete client;

  if (!success) {
    throw "HTTP Client Response Failed.";
  }


  Document d;
  d.Parse(rsp);
  assert(d["balance"].IsInt());
  int balance = d["balance"].GetInt();

  return get_output_amount(balance);
}

string auth(HttpClient *client, string username, string password, string email) {
  client->set_basic_auth(username, password);
  WwwFormEncodedDict encoder = WwwFormEncodedDict();
  encoder.set("username", username); 
  encoder.set("password", password);
  string body = encoder.encode();

  HTTPClientResponse *response;
  response = client->post("/auth-tokens", body);
  string rsp = response->body();
  bool success = response->success();
  delete response;

  if (!success) {
    throw "HTTP Client Response Failed.";
  }

  Document d;
  d.Parse(rsp);
  assert(d["auth_token"].IsString());
  assert(d["user_id"].IsString());
  auth_token = d["auth_token"].GetString();
  user_id = d["user_id"].GetString();
  
  is_loged_in = true;
  return update_email(email);
}

string get_balance(HttpClient *client) {
  if (!is_loged_in) {
    throw "balance: No User Logged In.";
  }
  // make a API request
  string path = "/users/" + user_id;
  client->set_header("x-auth-token", auth_token);
  HTTPClientResponse *response = client->get(path);
  string rsp = response->body();
  bool success = response->success();
  delete response;

  if (!success) {
    throw "HTTP Client Response Failed.";
  }

  Document d;
  d.Parse(rsp);
  assert(d["balance"].IsInt());
  return get_output_amount(d["balance"].GetInt());
}

string deposit(HttpClient *client, string amount, string card, string year, string month, string cvc) {
  if (!is_loged_in) {
    throw "deposit: No User Logged In.";
  }

  // API call to string to get token
  // from the dcash wallet to Stripe
  HttpClient *stripe_client = new HttpClient("api.stripe.com", 443, true);
  stripe_client->set_header("Authorization", string("Bearer ") + PUBLISHABLE_KEY);
  stripe_client->set_basic_auth(PUBLISHABLE_KEY, "");

  WwwFormEncodedDict encoder;
  encoder.set("card[number]", card.c_str());
  encoder.set("card[exp_month]", atoi(month.c_str()));
  encoder.set("card[exp_year]", atoi(year.c_str()));
  encoder.set("card[cvc]", atoi(cvc.c_str()));
  string body = encoder.encode();

  HTTPClientResponse *client_response = stripe_client->post("/v1/tokens", body);
  // This method converts the HTTP body into a rapidjson document
  Document *d = client_response->jsonBody();
  bool success = client_response->success();
  delete stripe_client;
  delete client_response;

  if (!success) {
    throw "Stripe Client Response Failed.";
  }

  string token = (*d)["id"].GetString();
  delete d;

  // send amount and token to API server
  encoder = WwwFormEncodedDict();
  encoder.set("stripe_token", token); 
  encoder.set("amount", get_input_amount(amount));
  body = encoder.encode();
  client->set_header("x-auth-token", auth_token);
  client_response = client->post("/deposits", body);

  if (!client_response->success()) {
    delete client_response;
    throw "HTTP Client Response Failed.";
  }

  d = client_response->jsonBody();
  delete client_response;

  assert((*d)["balance"].IsInt());
  int balance = (*d)["balance"].GetInt();
  delete d;
  return get_output_amount(balance);
}

string send(HttpClient *client, string to, string amount) {
  if (!is_loged_in) {
    throw "send: No User Logged In.";
  }

  WwwFormEncodedDict encoder;
  encoder.set("to", to);
  encoder.set("amount", get_input_amount(amount));
  string body = encoder.encode();

  client->set_header("x-auth-token", auth_token);
  HTTPClientResponse *response = client->post("/transfers", body);
  string rsp = response->body();
  bool success = response->success();
  delete response;

  if (!success) {
    throw "HTTP Client Response Failed.";
  }

  Document d;
  d.Parse(rsp);
  assert(d["balance"].IsInt());
  return get_output_amount(d["balance"].GetInt());
}

void logout(HttpClient *client) {
  if (!is_loged_in) {
    exit(0);
  }

  string path = "/auth-tokens/" + user_id;
  client->set_header("x-auth-token", auth_token);
  HTTPClientResponse *client_response = client->del(path);
  delete client_response;

  // the server show returns nothing
  exit(0);  // exit the program
}

int apply_function(vector<string> argv) {
  string balance = "";
  HttpClient *client = new HttpClient(API_SERVER_HOST.c_str(), API_SERVER_PORT);

  try {
    if (argv[0] == "auth") {
      if (argv.size() != 4) {
        throw "auth: Invalid argv.";
      }
      balance = auth(client, argv[1], argv[2], argv[3]);
    } else if (argv[0] == "balance") {
      if (argv.size() != 1) {
        return 1;
        throw "balance: Invalid argv.";
      }
      balance = get_balance(client);
    } else if (argv[0] == "deposit") {
      if (argv.size() != 6) {
        throw "deposit: Invalid argv.";
      }
      balance = deposit(client, argv[1], argv[2], argv[3], argv[4], argv[5]);
    } else if (argv[0] == "send") {
      if (argv.size() != 3) {
        throw "send: Invalid argv.";
      }
      balance = send(client, argv[1], argv[2]);
    } else if (argv[0] == "logout") {
      if (argv.size() != 1) {
        throw "logout: Invalid argv.";
      }
      logout(client);
    } else {
      throw "Invalid Command.";
    }

    string msg = "Balance: $" + balance + "\n";
    write(STDOUT_FILENO, msg.c_str(), msg.length());
  } catch (const char* msg) {
    #ifdef _TESTING_
    cerr << msg << endl;
    #endif
    char error_message[30] = "Error\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
  } catch (...) {}

  delete client;

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
      char error_message[30] = "Error\n";
      write(STDERR_FILENO, error_message, strlen(error_message));
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
