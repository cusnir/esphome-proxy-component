# ESPHome HTTP Proxy Component

A custom component for ESPHome that adds support for HTTP/HTTPS requests through a proxy server.
This component is particularly useful in environments where direct internet access is restricted and traffic must be routed through a proxy server.

## Features

- Support for HTTP/HTTPS requests through a proxy server
- CONNECT method support for HTTPS tunneling
- Configurable proxy host and port
- Customizable request headers
- Error handling and logging
- Example implementations for common use cases (InfluxDB, webhooks, etc.)

## Installation

### Method 1: External Components (Recommended)

Add this to your ESPHome configuration:

```yaml
external_components:
  - source: github://cusnir/esphome-proxy-component@main
    components: [ proxy_client ]
```

### Method 2: Manual Installation

1. Create a `components` directory in your ESPHome configuration directory
2. Clone this repository into the components directory:
```bash
cd ~/.esphome/components  # or your ESPHome config directory
git clone https://github.com/cusnir/esphome-proxy-component.git proxy_client
```

## Configuration

### Basic Configuration

```yaml
# Example minimal configuration
proxy_client:
  proxy_host: "192.168.1.5"
  proxy_port: 3128
```

### Full Configuration Options

```yaml
proxy_client:
  proxy_host: "192.168.1.5"    # Required: Proxy server hostname or IP
  proxy_port: 3128             # Required: Proxy server port
  proxy_username: "user"       # Optional: Proxy authentication username
  proxy_password: "pass"       # Optional: Proxy authentication password
  timeout: 10s                 # Optional: Connection timeout (default: 10s)
```

## Usage Examples

### Basic HTTP Request

```yaml
# Example of making a simple HTTP request through proxy
on_boot:
  then:
    - proxy_client.send:
        url: "http://example.com/api/data"
        method: POST
        headers:
          Content-Type: "application/json"
        body: '{"sensor": "value"}'
```

### Sending Data to InfluxDB

```yaml
# Example of sending sensor data to InfluxDB through proxy
sensor:
  - platform: dht
    temperature:
      name: "Room Temperature"
      on_value:
        then:
          - proxy_client.send:
              url: "http://influxdb:8086/api/v2/write?org=myorg&bucket=mybucket"
              method: POST
              headers:
                Authorization: "Token YOUR_INFLUXDB_TOKEN"
                Content-Type: "text/plain"
              body: !lambda |-
                return "temperature,room=living value=" + str(x);
```

### HTTPS Request Example

```yaml
# Example of HTTPS request (using CONNECT method)
button:
  - platform: template
    name: "Test HTTPS"
    on_press:
      - proxy_client.send:
          url: "https://api.example.com/data"
          method: GET
          headers:
            Authorization: "Bearer token123"
```

## Advanced Usage

### Using with Automation

```yaml
automation:
  - trigger:
      platform: time
      seconds: /30
    then:
      - proxy_client.send:
          url: !lambda |-
            return "http://example.com/api/data?value=" + id(my_sensor).state;
```

### Error Handling

```yaml
on_boot:
  then:
    - proxy_client.send:
        url: "http://example.com/api"
        on_success:
          then:
            - logger.log: "Request successful!"
        on_error:
          then:
            - logger.log: 
                format: "Request failed: %s"
                args: [ error ]
```

## Example Use Cases

### 1. Sending Data to InfluxDB through Corporate Proxy

```yaml
# Full example of sensor data to InfluxDB through proxy
esphome:
  name: sensor_node

external_components:
  - source: github://cusnir/esphome-proxy-component@main
    components: [ proxy_client ]

proxy_client:
  proxy_host: "192.168.1.5"
  proxy_port: 3128

sensor:
  - platform: pmsx003
    pm_2_5:
      name: "Particulate Matter 2.5"
      on_value:
        then:
          - proxy_client.send:
              url: "http://influxdb:8086/api/v2/write?org=myorg&bucket=sensors"
              method: POST
              headers:
                Authorization: "Token YOUR_INFLUXDB_TOKEN"
                Content-Type: "text/plain"
              body: !lambda |-
                return "air_quality,sensor=pms pm2_5=" + str(x);
```

### 2. Webhook Notifications

```yaml
# Example of sending notifications through webhook
binary_sensor:
  - platform: gpio
    pin: GPIO5
    name: "Door Sensor"
    on_state:
      then:
        - proxy_client.send:
            url: "https://webhook.example.com/door"
            method: POST
            body: !lambda |-
              return "{\"status\": \"" + (x ? "opened" : "closed") + "\"}";
```

## Troubleshooting

### Common Issues

1. **Connection Failed**
   - Check if proxy host and port are correct
   - Verify proxy server is running and accessible
   - Check network connectivity

2. **Authentication Failed**
   - Verify proxy credentials if authentication is required
   - Check if proxy requires specific headers

3. **HTTPS Issues**
   - Ensure proxy supports CONNECT method for HTTPS
   - Verify SSL/TLS certificates if required

### Debug Logging

Enable verbose logging in your configuration:

```yaml
logger:
  level: VERBOSE
  logs:
    proxy_client: DEBUG
```

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## License

This project is licensed under the MIT License - see the LICENSE file for details.
