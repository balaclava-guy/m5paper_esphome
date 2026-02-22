import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome import automation
from esphome.components import display, spi
from esphome.const import (
    CONF_ID,
    CONF_RESET_PIN,
    CONF_RESET_DURATION,
    CONF_BUSY_PIN,
    CONF_LAMBDA,
    CONF_MODEL,
    CONF_REVERSED,
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
UpdateAction = it8951e_ns.class_("UpdateAction", automation.Action)

it8951eModel = it8951e_ns.enum("it8951eModel")
update_mode_e = it8951e_ns.enum("update_mode_e")

CONF_MODE = "mode"
CONF_FULL = "full"

# Notes for users (shown via option descriptions below):
# - INIT: panel init refresh (slow)
# - DU / DU4 / A2: fast modes, best for UI/status overlays
# - GC16 / GL16 / GLR16 / GLD16: higher quality grayscale modes
MODES = {
    "INIT": update_mode_e.UPDATE_MODE_INIT,
    "DU": update_mode_e.UPDATE_MODE_DU,
    "GC16": update_mode_e.UPDATE_MODE_GC16,
    "GL16": update_mode_e.UPDATE_MODE_GL16,
    "GLR16": update_mode_e.UPDATE_MODE_GLR16,
    "GLD16": update_mode_e.UPDATE_MODE_GLD16,
    "DU4": update_mode_e.UPDATE_MODE_DU4,
    "A2": update_mode_e.UPDATE_MODE_A2,
    "NONE": update_mode_e.UPDATE_MODE_NONE,
}

MODELS = {
    "M5EPD": it8951eModel.M5EPD,
}

CONFIG_SCHEMA = (
    display.FULL_DISPLAY_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(IT8951ESensor),
            cv.Required(CONF_RESET_PIN): pins.gpio_output_pin_schema,
            cv.Required(CONF_BUSY_PIN): pins.gpio_input_pin_schema,
            cv.Optional(
                CONF_REVERSED,
                default=False,
                description="Invert pixel values in the frame buffer (panel-specific).",
            ): cv.boolean,
            cv.Optional(
                CONF_RESET_DURATION,
                default="100ms",
                description="Reset pulse low duration. Keep under 500ms.",
            ): cv.All(
                cv.positive_time_period_milliseconds,
                cv.Range(max=cv.TimePeriod(milliseconds=500)),
            ),
            cv.Optional(
                CONF_MODEL,
                default="M5EPD",
                description="Panel model preset (M5Paper uses M5EPD).",
            ): cv.enum(MODELS, upper=True, space="_"),
        }
    )
    .extend(cv.polling_component_schema("1s"))
    .extend(spi.spi_device_schema())
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
async def it8951e_clear_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    return var

@automation.register_action(
    "it8951e.update",
    UpdateAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(IT8951ESensor),
            cv.Optional(
                CONF_MODE,
                default="GC16",
                description=(
                    "Refresh waveform mode. "
                    "Use DU/DU4/A2 for fast status updates; "
                    "use GC16/GL16 for higher quality."
                ),
            ): cv.enum(MODES, upper=True, space="_"),
            cv.Optional(
                CONF_FULL,
                default=False,
                description="If true, refresh the full panel area regardless of dirty region tracking.",
            ): cv.boolean,
        }
    ),
)
async def it8951e_update_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    cg.add(var.set_mode(config[CONF_MODE]))
    cg.add(var.set_full(config[CONF_FULL]))
    return var

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])

    # ESPHome 2026.x: always register. Older conditional gating breaks modern builds.
    await cg.register_component(var, config)
    await display.register_display(var, config)
    await spi.register_spi_device(var, config)

    cg.add(var.set_model(config[CONF_MODEL]))

    if CONF_LAMBDA in config:
        lambda_ = await cg.process_lambda(
            config[CONF_LAMBDA],
            [(display.DisplayRef, "it")],
            return_type=cg.void,
        )
        cg.add(var.set_writer(lambda_))

    reset = await cg.gpio_pin_expression(config[CONF_RESET_PIN])
    cg.add(var.set_reset_pin(reset))

    busy = await cg.gpio_pin_expression(config[CONF_BUSY_PIN])
    cg.add(var.set_busy_pin(busy))

    cg.add(var.set_reversed(config[CONF_REVERSED]))
    cg.add(var.set_reset_duration(config[CONF_RESET_DURATION]))

