import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins, automation
from esphome.const import CONF_ID, CONF_FILTER, CONF_IDLE, CONF_BUFFER_SIZE
from esphome.core import coroutine_with_priority, TimePeriod

CODEOWNERS = ["@mansellrace"]
comelit_intercom_ns = cg.esphome_ns.namespace("comelit_intercom")
ComelitIntercom = comelit_intercom_ns.class_("ComelitComponent", cg.Component)

ComelitIntercomSendAction = comelit_intercom_ns.class_(
    "ComelitIntercomSendAction", automation.Action
)

CONF_COMELIT_ID = "comelit_intercom"
CONF_SENSITIVITY = "sensitivity"
CONF_HW_VERSION = "hw_version"
CONF_RX_PIN = "rx_pin"
CONF_TX_PIN = "tx_pin"
CONF_LOGBOOK_LANGUAGE = "logbook_language"
CONF_LOGBOOK_ENTITY = "logbook_entity"
CONF_DUMP = "dump"
CONF_EVENT = "event"
CONF_COMMAND = "command"
CONF_ADDRESS = "address"
MULTI_CONF = False

HardwareType = comelit_intercom_ns.enum("Hw_Version")
HW_TYPES = {
    "2.5": HardwareType.HW_VERSION_TYPE_2_5,
    "2.6": HardwareType.HW_VERSION_TYPE_2_6,
    "older": HardwareType.HW_VERSION_TYPE_OLDER,
}

SENSITIVITY_TYPES = {
    "2.5" : ["low", "high"],
    "2.6" : ["1", "2", "3", "4", "5", "6"],
}

LanguageType = comelit_intercom_ns.enum("Language")
LANGUAGE_TYPES = {
    "disabled": LanguageType.LANGUAGE_DISABLED,
    "plain_command": LanguageType.LANGUAGE_PLAIN_COMMAND,
    "italian": LanguageType.LANGUAGE_ITALIAN,
    "english": LanguageType.LANGUAGE_ENGLISH,
}
def validate_config(config):
    hw_version = config[CONF_HW_VERSION]
    if config[CONF_SENSITIVITY] != "default":
        sensitivity = config[CONF_SENSITIVITY]
        if hw_version == "older":
            raise cv.Invalid("Older boards do not support the choice of sensitivity")

        if not sensitivity in SENSITIVITY_TYPES[hw_version]:
            raise cv.Invalid(
                f"For HW version {hw_version}, the sensitivity must {SENSITIVITY_TYPES[hw_version]} "
            )
    return config

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(ComelitIntercom),
            cv.Optional(CONF_HW_VERSION, default="2.5"): cv.enum(HW_TYPES, upper=False),
            cv.Optional(CONF_SENSITIVITY, default="default"): cv.string,
            cv.Optional(CONF_RX_PIN, default=12): pins.internal_gpio_input_pin_schema,
            cv.Optional(CONF_TX_PIN, default=5): pins.internal_gpio_output_pin_schema,
            cv.Optional(CONF_LOGBOOK_LANGUAGE, default="disabled"): cv.enum(LANGUAGE_TYPES),
            cv.Optional(CONF_LOGBOOK_ENTITY, default="none"): cv.string,
            cv.Optional(CONF_FILTER, default="1000us"): cv.All(
                cv.positive_time_period_microseconds,
                cv.Range(max=TimePeriod(microseconds=2500)),
            ),
            cv.Optional(CONF_IDLE, default="10ms"): cv.positive_time_period_microseconds,
            cv.Optional(CONF_BUFFER_SIZE, default="400b"): cv.validate_bytes,
            cv.Optional(CONF_DUMP, default=False): cv.boolean,
            cv.Optional(CONF_EVENT, default="comelit"): cv.string,
        }   
    )
    .extend(cv.COMPONENT_SCHEMA),
    validate_config,
)

@coroutine_with_priority(1.0)
async def to_code(config):
    cg.add_global(comelit_intercom_ns.using)
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    cg.add(var.set_hw_version(config[CONF_HW_VERSION]))

    rx_pin = await cg.gpio_pin_expression(config[CONF_RX_PIN])
    cg.add(var.set_rx_pin(rx_pin))

    pin = await cg.gpio_pin_expression(config[CONF_TX_PIN])
    cg.add(var.set_tx_pin(pin))

    cg.add(var.set_logbook_language(config[CONF_LOGBOOK_LANGUAGE]))
    cg.add(var.set_logbook_entity(config[CONF_LOGBOOK_ENTITY]))
    cg.add(var.set_sensitivity(config[CONF_SENSITIVITY]))
    cg.add(var.set_filter_us(config[CONF_FILTER]))
    cg.add(var.set_idle_us(config[CONF_IDLE]))
    cg.add(var.set_buffer_size(config[CONF_BUFFER_SIZE]))
    cg.add(var.set_dump(config[CONF_DUMP]))
    cg.add(var.set_event("esphome." + config[CONF_EVENT]))


COMELIT_INTERCOM_SEND_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(ComelitIntercom),
        cv.Required(CONF_COMMAND): cv.templatable(cv.hex_uint16_t),
        cv.Required(CONF_ADDRESS): cv.templatable(cv.hex_uint16_t),
    }
)


@automation.register_action(
    "comelit_intercom.send", ComelitIntercomSendAction, COMELIT_INTERCOM_SEND_SCHEMA
)
async def comelit_intercom_send_to_code(config, action_id, template_args, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_args, paren)
    template_ = await cg.templatable(config[CONF_COMMAND], args, cg.uint16)
    cg.add(var.set_command(template_))
    template_ = await cg.templatable(config[CONF_ADDRESS], args, cg.uint16)
    cg.add(var.set_address(template_))
    return var