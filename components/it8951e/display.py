import esphome.automation as automation
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import core, pins
from esphome.components import display, spi
from esphome.const import __version__ as ESPHOME_VERSION
from esphome.const import (
    CONF_BUSY_PIN,
    CONF_FULL_UPDATE_EVERY,
    CONF_ID,
    CONF_LAMBDA,
    CONF_MODEL,
    CONF_RESET_DURATION,
    CONF_RESET_PIN,
    CONF_REVERSED,
    CONF_SLEEP_WHEN_DONE,
)

DEPENDENCIES = ["spi"]

it8951e_ns = cg.esphome_ns.namespace("it8951e")

IT8951ESensor = it8951e_ns.class_(
    "IT8951ESensor",
    display.DisplayBuffer,
    spi.SPIDevice,
)

ClearAction = it8951e_ns.class_("ClearAction", automation.Action)
UpdateSlowAction = it8951e_ns.class_("UpdateSlowAction", automation.Action)
DrawAction = it8951e_ns.class_("DrawAction", automation.Action)
UpdateAction = it8951e_ns.class_("UpdateAction", automation.Action)

it8951eModel = it8951e_ns.enum("it8951eModel")
UpdateMode = it8951e_ns.enum("UpdateMode")

CONF_MODE = "mode"
CONF_FULL = "full"

MODELS = {
    "M5EPD": it8951eModel.M5EPD,
}

UPDATE_MODES = {
    "INIT": UpdateMode.UPDATE_MODE_INIT,
    "DU": UpdateMode.UPDATE_MODE_DU,
    "GC16": UpdateMode.UPDATE_MODE_GC16,
    "GL16": UpdateMode.UPDATE_MODE_GL16,
    "GLR16": UpdateMode.UPDATE_MODE_GLR16,
    "GLD16": UpdateMode.UPDATE_MODE_GLD16,
    "DU4": UpdateMode.UPDATE_MODE_DU4,
    "A2": UpdateMode.UPDATE_MODE_A2,
    "NONE": UpdateMode.UPDATE_MODE_NONE,
}

CONFIG_SCHEMA = cv.All(
    display.FULL_DISPLAY_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(IT8951ESensor),
            cv.Required(CONF_RESET_PIN): pins.gpio_output_pin_schema,
            cv.Required(CONF_BUSY_PIN): pins.gpio_input_pin_schema,
            cv.Optional(CONF_REVERSED, default=False): cv.boolean,
            cv.Optional(CONF_RESET_DURATION, default="100ms"): cv.All(
                cv.positive_time_period_milliseconds,
                cv.Range(max=core.TimePeriod(milliseconds=500)),
            ),
            cv.Optional(CONF_MODEL, default="M5EPD"): cv.enum(
                MODELS, upper=True, space="_"
            ),
            cv.Optional(CONF_SLEEP_WHEN_DONE, default=True): cv.boolean,
            cv.Optional(CONF_FULL_UPDATE_EVERY, default=60): cv.int_range(
                min=0, max=4294967295
            ),
        }
    )
    .extend(cv.polling_component_schema("never"))
    .extend(spi.spi_device_schema()),
)


IT8951E_SIMPLE_ACTION_SCHEMA = automation.maybe_simple_id(
    {
        cv.GenerateID(): cv.use_id(IT8951ESensor),
    }
)

IT8951E_UPDATE_ACTION_SCHEMA = automation.maybe_simple_id(
    {
        cv.GenerateID(): cv.use_id(IT8951ESensor),
        cv.Optional(CONF_MODE, default="DU"): cv.enum(UPDATE_MODES, upper=True),
        cv.Optional(CONF_FULL, default=False): cv.boolean,
    }
)


@automation.register_action("it8951e.clear", ClearAction, IT8951E_SIMPLE_ACTION_SCHEMA)
async def it8951e_clear_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    return var


@automation.register_action(
    "it8951e.updateslow", UpdateSlowAction, IT8951E_SIMPLE_ACTION_SCHEMA
)
async def it8951e_updateslow_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    return var


@automation.register_action("it8951e.draw", DrawAction, IT8951E_SIMPLE_ACTION_SCHEMA)
async def it8951e_draw_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    return var


@automation.register_action(
    "it8951e.update", UpdateAction, IT8951E_UPDATE_ACTION_SCHEMA
)
async def it8951e_update_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    cg.add(var.set_mode(config[CONF_MODE]))
    cg.add(var.set_full(config[CONF_FULL]))
    return var


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])

    if cv.Version.parse(ESPHOME_VERSION) < cv.Version.parse("2023.12.0"):
        await cg.register_component(var, config)

    await display.register_display(var, config)
    await spi.register_spi_device(var, config)

    if CONF_MODEL in config:
        cg.add(var.set_model(config[CONF_MODEL]))
    if CONF_LAMBDA in config:
        lambda_ = await cg.process_lambda(
            config[CONF_LAMBDA], [(display.DisplayRef, "it")], return_type=cg.void
        )
        cg.add(var.set_writer(lambda_))
    if CONF_RESET_PIN in config:
        reset = await cg.gpio_pin_expression(config[CONF_RESET_PIN])
        cg.add(var.set_reset_pin(reset))
    if CONF_BUSY_PIN in config:
        busy = await cg.gpio_pin_expression(config[CONF_BUSY_PIN])
        cg.add(var.set_busy_pin(busy))
    if CONF_REVERSED in config:
        cg.add(var.set_reversed(config[CONF_REVERSED]))
    if CONF_RESET_DURATION in config:
        cg.add(var.set_reset_duration(config[CONF_RESET_DURATION]))
    if CONF_SLEEP_WHEN_DONE in config:
        cg.add(var.set_sleep_when_done(config[CONF_SLEEP_WHEN_DONE]))
    if CONF_FULL_UPDATE_EVERY in config:
        cg.add(var.set_full_update_every(config[CONF_FULL_UPDATE_EVERY]))
