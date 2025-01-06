#pragma once

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/core/log.h"
#include <map>
#include <WiFiClient.h>

namespace esphome {
namespace proxy_client {

class ProxyClient : public Component {
 public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::AFTER_WIFI; }
  
  void set_proxy_host(const std::string &host) { proxy_host_ = host; }
  void set_proxy_port(uint16_t port) { proxy_port_ = port; }
  void set_proxy_username(const std::string &username) { proxy_username_ = username; }
  void set_proxy_password(const std::string &password) { proxy_password_ = password; }
  void set_timeout(uint32_t timeout) { timeout_ = timeout; }
  
  bool send_request(const std::string &url, const std::string &method,
                   const std::map<std::string, std::string> &headers,
                   const std::string &body, std::string &response);

 protected:
  bool establish_proxy_tunnel(const std::string &target_host, uint16_t target_port);
  bool parse_url(const std::string &url, std::string &protocol, std::string &host,
                uint16_t &port, std::string &path);
  
  std::string proxy_host_;
  uint16_t proxy_port_{3128};
  optional<std::string> proxy_username_;
  optional<std::string> proxy_password_;
  uint32_t timeout_{10000};
  WiFiClient client_;
};

class SendAction : public Action<> {
 public:
  void set_parent(ProxyClient *parent) { parent_ = parent; }
  void set_url(const std::string &url) { url_ = url; }
  void set_method(const std::string &method) { method_ = method; }
  void add_header(const std::string &key, const std::string &value) { headers_[key] = value; }
  void set_body(const std::string &body) { body_ = body; }
  
  Trigger<> *get_on_success_trigger() const { return this->on_success_trigger_; }
  Trigger<std::string> *get_on_error_trigger() const { return this->on_error_trigger_; }
  
  void play(Action<> *action) override;
  
 protected:
  ProxyClient *parent_{nullptr};
  std::string url_;
  std::string method_{"GET"};
  std::map<std::string, std::string> headers_;
  optional<std::string> body_;
  Trigger<> *on_success_trigger_{nullptr};
  Trigger<std::string> *on_error_trigger_{nullptr};
};

}  // namespace proxy_client
}  // namespace esphome
