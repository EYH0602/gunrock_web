import os
import json
import requests
import stripe

class Wallet:
  def __init__(self, url, username, password):
    self.url = url
    self.username = username
    self.password = password
    self.user_info = json.loads(self.create_or_login_user())

  def get_detail(self):
    return json.dumps(self.user_info)

  def create_or_login_user(self) -> str:
    url = self.url + "/auth-tokens"
    data = {
      "username": self.username,
      "password": self.password
    }
    rsp = requests.post(url, data)
    return rsp.text

  def update_email(self, email: str) -> str:
    url = self.url + "/users/" + self.user_info["user_id"]
    header = {
      "x-auth-token": self.user_info["auth_token"]
    }
    data = {
      "email": email,
    }
    rsp = requests.put(url, data, headers=header)
    return rsp.text

  def get_account_info(self) -> str:
    url = self.url + "/users/" + self.user_info["user_id"]
    header = {
      "x-auth-token": self.user_info["auth_token"]
    }
    rsp = requests.get(url, headers=header)
    return rsp.text

  def deposit(self, amount: int, stripe_token: str) -> str:
    url = self.url + "/deposits"
    header = {
      "x-auth-token": self.user_info["auth_token"]
    }
    data = {
      "amount": amount,
      "stripe_token": stripe_token
    }
    rsp = requests.post(url, data=data, headers=header)
    return rsp.text

  def transfer(self, to: str, amount: int) -> str:
    url = self.url + "/transfers"
    header = {
      "x-auth-token": self.user_info["auth_token"]
    }
    data = {
      "amount": amount,
      "to": to
    }
    rsp = requests.post(url, data=data, headers=header)
    return rsp.text

  def delete_account(self):
    url = self.url + "/auth-tokens/" + self.user_info["user_id"]
    header = {
      "x-auth-token": self.user_info["auth_token"]
    }
    rsp = requests.delete(url, data=None, headers=header)
    return rsp.text

f = open("config.json", 'r')
settings = json.load(f)
url = "http://" + settings["api_server_host"] + ":" + str(settings["api_server_port"])
stripe.api_key = settings["stripe_publishable_key"]
rsp = json.loads(str(stripe.Token.create(
  card={
    "number": "4242424242424242",
    "exp_month": 5,
    "exp_year": 2022,
    "cvc": "314",
  },
)))
stripe_token = rsp["id"]


kingst = Wallet(url, "kingst", "123")
honey = Wallet(url, "honey", "123")


kingst.update_email("kingst@ucdavis.edu")
honey.update_email("honey@ucdavis.edu")

kingst.deposit(25000, stripe_token)
# kingst.deposit(1000, stripe_token)
# kingst.deposit(1000, stripe_token)

# print(kingst.transfer("honey", 400))
print(kingst.get_account_info())
print(honey.get_account_info())


# print(kingst.get_account_info())
# print(honey.get_account_info())

# kingst.delete_account()
# print(kingst.get_account_info())

