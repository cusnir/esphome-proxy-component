#pragma once

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
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

template<typename... Ts>
class SendAction : public Action<Ts...>, public Parented<ProxyClient> {
 public:
  void set_url(const std::string &url) { url_ = url; }
  void set_method(const std::string &method) { method_ = method; }
  void add_header(const std::string &key, const std::string &value) { headers_[key] = value; }
  void set_body(const std::string &body) { body_ = body; }
  
  void play(Ts... x) override {
    std::string response;
    if (this->parent_->send_request(this->url_, this->method_, this->headers_,
                                   this->body_.value_or(""), response)) {
      ESP_LOGD("proxy_client", "Request successful");
    } else {
      ESP_LOGE("proxy_client", "Request failed: %s", response.c_str());
    }
  }
  
 protected:
  std::string url_;
  std::string method_{"GET"};
  std::map<std::string, std::string> headers_;
  optional<std::string> body_;
};

}  // namespace proxy_client
}  // namespace esphome
