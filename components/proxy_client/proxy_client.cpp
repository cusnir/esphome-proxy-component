#include "proxy_client.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace proxy_client {

static const char *const TAG = "proxy_client";

void ProxyClient::setup() {
  ESP_LOGCONFIG(TAG, "Setting up HTTP Proxy Client...");
}

void ProxyClient::dump_config() {
  ESP_LOGCONFIG(TAG, "HTTP Proxy Client:");
  ESP_LOGCONFIG(TAG, "  Proxy Host: %s", this->proxy_host_.c_str());
  ESP_LOGCONFIG(TAG, "  Proxy Port: %d", this->proxy_port_);
  if (this->proxy_username_.has_value()) {
    ESP_LOGCONFIG(TAG, "  Proxy Username: %s", this->proxy_username_.value().c_str());
  }
  ESP_LOGCONFIG(TAG, "  Timeout: %dms", this->timeout_);
}

bool ProxyClient::parse_url(const std::string &url, std::string &protocol, std::string &host,
                          uint16_t &port, std::string &path) {
  size_t proto_end = url.find("://");
  if (proto_end == std::string::npos)
    return false;
    
  protocol = url.substr(0, proto_end);
  size_t host_start = proto_end + 3;
  size_t path_start = url.find('/', host_start);
  
  if (path_start == std::string::npos) {
    host = url.substr(host_start);
    path = "/";
  } else {
    host = url.substr(host_start, path_start - host_start);
    path = url.substr(path_start);
  }
  
  size_t port_start = host.find(':');
  if (port_start == std::string::npos) {
    port = (protocol == "https") ? 443 : 80;
  } else {
    port = atoi(host.substr(port_start + 1).c_str());
    host = host.substr(0, port_start);
  }
  
  return true;
}

bool ProxyClient::establish_proxy_tunnel(const std::string &target_host, uint16_t target_port) {
  if (!client_.connect(this->proxy_host_.c_str(), this->proxy_port_)) {
    ESP_LOGE(TAG, "Failed to connect to proxy");
    return false;
  }
  
  // Send CONNECT request
  client_.printf("CONNECT %s:%d HTTP/1.1\r\n", target_host.c_str(), target_port);
  client_.printf("Host: %s:%d\r\n", target_host.c_str(), target_port);
  if (this->proxy_username_.has_value() && this->proxy_password_.has_value()) {
    std::string auth = this->proxy_username_.value() + ":" + this->proxy_password_.value();
    client_.printf("Proxy-Authorization: Basic %s\r\n", base64_encode((uint8_t *) auth.c_str(), auth.length()).c_str());
  }
  client_.println();
  
  // Read response
  String line = client_.readStringUntil('\n');
  if (!line.startsWith("HTTP/1.1 200")) {
    ESP_LOGE(TAG, "Proxy tunnel failed: %s", line.c_str());
    client_.stop();
    return false;
  }
  
  // Skip remaining headers
  while (client_.available()) {
    String skip = client_.readStringUntil('\n');
    if (skip == "\r" || skip == "\n" || skip.length() == 0)
      break;
  }
  
  return true;
}

bool ProxyClient::send_request(const std::string &url, const std::string &method,
                             const std::map<std::string, std::string> &headers,
                             const std::string &body, std::string &response) {
  std::string protocol, host, path;
  uint16_t port;
  
  if (!parse_url(url, protocol, host, port, path)) {
    ESP_LOGE(TAG, "Invalid URL: %s", url.c_str());
    return false;
  }
  
  if (protocol == "https") {
    if (!establish_proxy_tunnel(host, port)) {
      return false;
    }
  } else {
    if (!client_.connect(this->proxy_host_.c_str(), this->proxy_port_)) {
      ESP_LOGE(TAG, "Failed to connect to proxy");
      return false;
    }
  }
  
  // Send request
  if (protocol == "https") {
    client_.printf("%s %s HTTP/1.1\r\n", method.c_str(), path.c_str());
  } else {
    client_.printf("%s %s HTTP/1.1\r\n", method.c_str(), url.c_str());
  }
  client_.printf("Host: %s\r\n", host.c_str());
  
  // Send headers
  for (const auto &header : headers) {
    client_.printf("%s: %s\r\n", header.first.c_str(), header.second.c_str());
  }
  
  // Send body if present
  if (!body.empty()) {
    client_.printf("Content-Length: %d\r\n", body.length());
    client_.println();
    client_.print(body.c_str());
  } else {
    client_.println();
  }
  
  // Read response
  String status = client_.readStringUntil('\n');
  response = status.c_str();
  
  // Read headers and body
  while (client_.available()) {
    String line = client_.readStringUntil('\n');
    response += line.c_str();
    response += "\n";
  }
  