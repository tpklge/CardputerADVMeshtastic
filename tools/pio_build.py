"""PlatformIO build integration; upstream remains unmodified."""
from pathlib import Path
import sys

Import("env")
from SCons.Script import COMMAND_LINE_TARGETS

allowed_targets = {"buildprog", "size", "checkprogsize", "clean", "compiledb", "idedata", "envdump"}
unsupported = set(COMMAND_LINE_TARGETS) - allowed_targets
if unsupported:
    raise RuntimeError(f"Only offline build targets are supported: {sorted(unsupported)}")

root = Path(env.subst("$PROJECT_DIR"))
sys.path.insert(0, str(root / "tools"))
from prepare_upstream import verify_sources

verify_sources()
lib_dir = root / ".pio/libdeps" / env.subst("$PIOENV")
paths = [path for library in (lib_dir.iterdir() if lib_dir.exists() else []) if library.is_dir()
         for path in (str(library), str(library / "src"))]
framework = Path(env.PioPlatform().get_package_dir("framework-arduinoespressif32"))
paths += [str(framework / "libraries" / name / "src") for name in ("Wire", "SPI", "FS", "SD", "Preferences", "Ticker", "Update", "EEPROM", "SPIFFS", "LittleFS")]
env.Append(CPPPATH=paths + [str(root / "include")])
if env.subst("$PIOENV") in ("cardputer-adv-m2", "cardputer-adv-m3", "cardputer-adv-m4", "cardputer-adv-m5", "cardputer-adv-m6", "cardputer-adv-m7", "cardputer-adv-m8"):
    from stage_core import stage_core
    stage = stage_core(root)
    # Generated headers must precede the unchanged vendor headers, including variant.h.
    env.Prepend(CPPPATH=[str(stage / "variant"), str(stage / "src"),
                        str(stage / "src/mesh"), str(stage / "src/mesh/generated"),
                        str(stage / "src/gps"), str(stage / "src/buzz"),
                        str(stage / "src/platform/esp32")])
    env.BuildSources("$BUILD_DIR/upstream-core", str(stage / "src"),
        src_filter="+<*> -<platform/portduino/> -<platform/nrf52/> -<platform/stm32wl/> "
        "-<platform/extra_variants/> -<platform/rp2xx0/> -<graphics/niche/> -<mesh/eth/> -<mesh/raspihttp/> "
        "-<modules/AdminModule.cpp> -<modules/ExternalNotificationModule.cpp> -<input/CardputerKeyboard.cpp> -<mesh/http/> -<nimble/> -<motion/> -<modules/Telemetry/Sensor/>")
else:
    env.BuildSources("$BUILD_DIR/upstream-board", str(root / "vendor/meshtastic-firmware/src"),
        src_filter="-<*> +<SPILock.cpp> +<concurrency/Lock.cpp> "
                   "+<concurrency/LockGuard.cpp> +<input/TCA8418KeyboardBase.cpp>")
