import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_URL, CONF_METHOD, CONF_HEADERS, CONF_BODY
from esphome.core import CORE
from esphome import automation

DEPENDENCIES = ['network']
AUTO_LOAD = ['async_tcp']

proxy_client_ns = cg.esphome_ns.namespace('proxy_client')
ProxyClient = proxy_client_ns.class_('ProxyClient', cg.Component)
SendAction = proxy_client_ns.class_('SendAction', automation.Action)

# Configuration Schemas
CONF_PROXY_HOST = 'proxy_host'
CONF_PROXY_PORT = 'proxy_port'
CONF_PROXY_USERNAME = 'proxy_username'
CONF_PROXY_PASSWORD = 'proxy_password'
CONF_TIMEOUT = 'timeout'
CONF_ON_SUCCESS = 'on_success'
CONF_ON_ERROR = 'on_error'

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(ProxyClient),
    cv.Required(CONF_PROXY_HOST): cv.string,
    cv.Required(CONF_PROXY_PORT): cv.port,
    cv.Optional(CONF_PROXY_USERNAME): cv.string,
    cv.Optional(CONF_PROXY_PASSWORD): cv.string,
    cv.Optional(CONF_TIMEOUT, default='10s'): cv.positive_time_period_milliseconds,
}).extend(cv.COMPONENT_SCHEMA)

PROXY_SEND_SCHEMA = cv.Schema({
    cv.Required(CONF_URL): cv.string,
    cv.Optional(CONF_METHOD, default='GET'): cv.string,
    cv.Optional(CONF_HEADERS, default={}): cv.Schema({cv.string: cv.string}),
    cv.Optional(CONF_BODY): cv.templatable(cv.string),
    cv.Optional(CONF_ON_SUCCESS): automation.validate_automation(),
    cv.Optional(CONF_ON_ERROR): automation.validate_automation(),
})

@automation.register_action('proxy_client.send', SendAction, PROXY_SEND_SCHEMA)
async def proxy_send_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    cg.add(var.set_url(config[CONF_URL]))
    cg.add(var.set_method(config[CONF_METHOD]))
    
    for key, value in config[CONF_HEADERS].items():
        cg.add(var.add_header(key, value))
    
    if CONF_BODY in config:
        template_ = await cg.templatable(config[CONF_BODY], args, cg.std_string)
        cg.add(var.set_body(template_))
    
    if CONF_ON_SUCCESS in config:
        await automation.build_automation(var.get_on_success_trigger(), [], config[CONF_ON_SUCCESS])
    
    if CONF_ON_ERROR in config:
        await automation.build_automation(var.get_on_error_trigger(), [(cg.std_string, 'error')], config[CONF_ON_ERROR])
    
    return var

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    
    cg.add(var.set_proxy_host(config[CONF_PROXY_HOST]))
    cg.add(var.set_proxy_port(config[CONF_PROXY_PORT]))
    
    if CONF_PROXY_USERNAME in config:
        cg.add(var.set_proxy_username(config[CONF_PROXY_USERNAME]))
    if CONF_PROXY_PASSWORD in config:
        cg.add(var.set_proxy_password(config[CONF_PROXY_PASSWORD]))
    
    cg.add(var.set_timeout(config[CONF_TIMEOUT]))
