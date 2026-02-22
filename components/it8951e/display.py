import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import core, pins
from esphome import automation
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

# New: parameterized update action
UpdateModeAction = it8951e_ns.class_("UpdateModeAction", automation.Action)

it8951eModel = it8951e_ns.enum("it8951eModel")

MODELS = {
    "M5EPD": it8951eModel.M5EPD,
}

# New: expose update modes as config values
UpdateMode = it8951e_ns.enum("update_mode_e")

# Modes (high-level guidance)
# INIT  - full clean/flash erase, slowest, best for getting rid of ghosting.
# DU    - very fast B/W update, minimal flash, good for boot splash and small changes.
# DU4   - fast 4-level grayscale-ish, more artifacts than GL16/GC16, usually less flash than full.
# A2    - fast B/W optimized for frequent changes (more ghosting), good for quick UI feedback.
# GL16  - “grayscale” update (text/lines look good), slower than DU/DU4/A2.
# GC16  - full quality grayscale update, slowest “normal” mode.
# GLR16/GLD16 - variants used for different waveform behavior; depends on panel/LUT.
# NONE  - no update.

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

CONF_MODE = "mode"
CONF_DESCRIPTION = "description"

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
            cv.Optional(CONF_MODEL, default="M5EPD"): cv.enum(MODELS, upper=True, space="_"),
            cv.Optional(CONF_SLEEP_WHEN_DONE, default=True): cv.boolean,
            cv.Optional(CONF_FULL_UPDATE_EVERY, default=60): cv.int_range(min=0, max=4294967295),
        }
    )
    .extend(cv.polling_component_schema("1s"))
    .extend(spi.spi_device_schema()),
)

@automation.register_action(
    "it8951e.clear",
    ClearAction,
    automation.maybe_simple_id(
        {
            cv.GenerateID(): cv.use_id(IT8951ESensor),
        }
    ),
)
@automation.register_action(
    "it8951e.updateslow",
    UpdateSlowAction,
    automation.maybe_simple_id(
        {
            cv.GenerateID(): cv.use_id(IT8951ESensor),
        }
    ),
)
@automation.register_action(
    "it8951e.draw",
    DrawAction,
    automation.maybe_simple_id(
        {
            cv.GenerateID(): cv.use_id(IT8951ESensor),
        }
    ),
)

# New: one action with a mode parameter.
# We add an optional 'description' field purely for humans; some editors show it, many won't,
# but it never harms validation.
@automation.register_action(
    "it8951e.update",
    UpdateModeAction,
    cv.Schema(
        {
            cv.Required(CONF_ID): cv.use_id(IT8951ESensor),
            cv.Required(CONF_MODE): cv.enum(UPDATE_MODES, upper=True),
            cv.Optional(CONF_DESCRIPTION): cv.string,
        }
    ),
)
async def it8951e_update_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    cg.add(var.set_mode(config[CONF_MODE]))
    # 'description' is intentionally ignored at codegen time.
    return var


async def it8951e_clear_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    return var


async def it8951e_simple_parented_action_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    return var


it8951e_updateslow_to_code = it8951e_simple_parented_action_to_code
it8951e_draw_to_code = it8951e_simple_parented_action_to_code


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
