import esphome.codegen as cg
import esphome.config_validation as cv
import esphome.automation as automation

from esphome.components import display, spi
from esphome import pins
from esphome.const import (
    CONF_ID,
    CONF_CS_PIN,
    CONF_RESET_PIN,
    CONF_BUSY_PIN,
    CONF_ROTATION,
    CONF_REVERSED,
    CONF_UPDATE_INTERVAL,
)

from . import it8951e_ns

IT8951EDisplay = it8951e_ns.class_("IT8951EDisplay", display.DisplayBuffer, cg.PollingComponent)

UpdateMode = it8951e_ns.enum("UpdateMode")

CONF_SPI_ID = "spi_id"
CONF_MODE = "mode"
CONF_FULL = "full"

UPDATE_MODES = {
    "DU": UpdateMode.DU,
    "DU4": UpdateMode.DU4,
    "A2": UpdateMode.A2,
    "GL16": UpdateMode.GL16,
    "GC16": UpdateMode.GC16,
    "INIT": UpdateMode.INIT,
    "NONE": UpdateMode.NONE,
}

DEFAULT_MODE = "GC16"

CONFIG_SCHEMA = display.FULL_DISPLAY_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(IT8951EDisplay),
        cv.Required(CONF_SPI_ID): cv.use_id(spi.SPIComponent),

        # Pin schemas must come from esphome.pins, not cv.pin
        cv.Required(CONF_CS_PIN): pins.internal_gpio_output_pin_schema,
        cv.Required(CONF_RESET_PIN): pins.internal_gpio_output_pin_schema,
        cv.Required(CONF_BUSY_PIN): pins.internal_gpio_input_pin_schema,

        cv.Optional(CONF_ROTATION, default=0): cv.int_,
        cv.Optional(CONF_REVERSED, default=False): cv.boolean,
        cv.Optional(CONF_UPDATE_INTERVAL, default="never"): cv.update_interval,
    }
).extend(cv.polling_component_schema("never"))

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await display.register_display(var, config)

    spi_dev = await cg.get_variable(config[CONF_SPI_ID])
    cg.add(var.set_spi(spi_dev))

    cs = await cg.gpio_pin_expression(config[CONF_CS_PIN])
    rst = await cg.gpio_pin_expression(config[CONF_RESET_PIN])
    busy = await cg.gpio_pin_expression(config[CONF_BUSY_PIN])

    cg.add(var.set_cs_pin(cs))
    cg.add(var.set_reset_pin(rst))
    cg.add(var.set_busy_pin(busy))

    cg.add(var.set_rotation(config[CONF_ROTATION]))
    cg.add(var.set_reversed(config[CONF_REVERSED]))


IT8951EUpdateAction = it8951e_ns.class_("IT8951EUpdateAction", automation.Action)
IT8951EClearAction = it8951e_ns.class_("IT8951EClearAction", automation.Action)

IT8951E_UPDATE_ACTION_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_ID): cv.use_id(IT8951EDisplay),
        cv.Optional(CONF_MODE, default=DEFAULT_MODE): cv.enum(UPDATE_MODES, upper=True),
        cv.Optional(CONF_FULL, default=True): cv.boolean,
    }
)

IT8951E_CLEAR_ACTION_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_ID): cv.use_id(IT8951EDisplay),
    }
)

@automation.register_action("it8951e.update", IT8951EUpdateAction, IT8951E_UPDATE_ACTION_SCHEMA)
async def it8951e_update_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    cg.add(var.set_mode(config[CONF_MODE]))
    cg.add(var.set_full(config[CONF_FULL]))
    return var

@automation.register_action("it8951e.clear", IT8951EClearAction, IT8951E_CLEAR_ACTION_SCHEMA)
async def it8951e_clear_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    return var

