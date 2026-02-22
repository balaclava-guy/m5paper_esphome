import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import core, pins, automation
from esphome.components import display, spi
from esphome.const import __version__ as ESPHOME_VERSION
from esphome.const import (
    CONF_NAME,
    CONF_ID,
    CONF_RESET_PIN,
    CONF_RESET_DURATION,
    CONF_BUSY_PIN,
    CONF_LAMBDA,
    CONF_MODEL,
    CONF_REVERSED,
    CONF_SLEEP_WHEN_DONE,
    CONF_FULL_UPDATE_EVERY,
)

DEPENDENCIES = ["spi"]

# -----------------------------------------------------------------------------
# Namespace / classes
# -----------------------------------------------------------------------------
it8951e_ns = cg.esphome_ns.namespace("it8951e")

IT8951ESensor = it8951e_ns.class_(
    "IT8951ESensor",
    cg.PollingComponent,
    spi.SPIDevice,
    display.DisplayBuffer,
    display.Display,
)

ClearAction = it8951e_ns.class_("ClearAction", automation.Action)
UpdateSlowAction = it8951e_ns.class_("UpdateSlowAction", automation.Action)
DrawAction = it8951e_ns.class_("DrawAction", automation.Action)

# New: parameterised update action (mode selectable)
UpdateAction = it8951e_ns.class_("UpdateAction", automation.Action)

# -----------------------------------------------------------------------------
# Enums / config keys
# -----------------------------------------------------------------------------
it8951eModel = it8951e_ns.enum("it8951eModel")
MODELS = {
    "M5EPD": it8951eModel.M5EPD,
}

CONF_MODE = "mode"

# IMPORTANT:
# These names must match the C++ enum you added in your fork.
# Keep them stable for backwards compatibility.
it8951eUpdateMode = it8951e_ns.enum("it8951eUpdateMode")
UPDATE_MODES = {
    # Fast, lowest quality, best for frequent UI updates
    "A2": it8951eUpdateMode.A2,
    # Very fast monochrome partial (if implemented in your C++)
    "DU": it8951eUpdateMode.DU,
    "DU4": it8951eUpdateMode.DU4,
    # Grayscale modes, better quality, slower
    "GL16": it8951eUpdateMode.GL16,
    # Full update / best quality (slowest)
    "GC16": it8951eUpdateMode.GC16,
}

DEFAULT_UPDATE_MODE = "GC16"

# -----------------------------------------------------------------------------
# Schema
# -----------------------------------------------------------------------------
CONFIG_SCHEMA = cv.All(
    display.FULL_DISPLAY_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(IT8951ESensor),
            cv.Optional(CONF_NAME): cv.string,
            cv.Required(CONF_RESET_PIN): pins.gpio_output_pin_schema,
            cv.Required(CONF_BUSY_PIN): pins.gpio_input_pin_schema,
            cv.Optional(CONF_REVERSED): cv.boolean,
            cv.Optional(CONF_RESET_DURATION): cv.All(
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
    .extend(cv.polling_component_schema("1s"))
    .extend(spi.spi_device_schema()),
)

# -----------------------------------------------------------------------------
# Actions
# -----------------------------------------------------------------------------
@automation.register_action(
    "it8951e.clear",
    ClearAction,
    automation.maybe_simple_id(
        {
            cv.GenerateID(): cv.use_id(IT8951ESensor),
        }
    ),
)
async def it8951e_clear_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    return var


@automation.register_action(
    "it8951e.updateslow",
    UpdateSlowAction,
    automation.maybe_simple_id(
        {
            cv.GenerateID(): cv.use_id(IT8951ESensor),
        }
    ),
)
async def it8951e_updateslow_to_code(config, action_id, template_arg, args):
    # Backwards compatible "slow update" action
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    return var


@automation.register_action(
    "it8951e.draw",
    DrawAction,
    automation.maybe_simple_id(
        {
            cv.GenerateID(): cv.use_id(IT8951ESensor),
        }
    ),
)
async def it8951e_draw_to_code(config, action_id, template_arg, args):
    # Backwards compatible "draw" action
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    return var


@automation.register_action(
    "it8951e.update",
    UpdateAction,
    automation.maybe_simple_id(
        {
            cv.GenerateID(): cv.use_id(IT8951ESensor),
            # Optional mode, defaults to full/quality update
            cv.Optional(CONF_MODE, default=DEFAULT_UPDATE_MODE): cv.enum(
                UPDATE_MODES, upper=True, space="_"
            ),
        }
    ),
)
async def it8951e_update_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])

    # Optional with default, avoids KeyError
    cg.add(var.set_mode(config[CONF_MODE]))
    return var


# -----------------------------------------------------------------------------
# Component codegen
# -----------------------------------------------------------------------------
async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])

    # Old ESPHome versions required manual register_component for some display setups.
    if cv.Version.parse(ESPHOME_VERSION) < cv.Version.parse("2023.12.0"):
        await cg.register_component(var, config)

    await display.register_display(var, config)
    await spi.register_spi_device(var, config)

    if CONF_MODEL in config:
        cg.add(var.set_model(config[CONF_MODEL]))

    if CONF_LAMBDA in config:
        lambda_ = await cg.process_lambda(
            config[CONF_LAMBDA],
            [(display.DisplayRef, "it")],
            return_type=cg.void,
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
