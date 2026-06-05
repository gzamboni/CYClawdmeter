# Project context

ESP32 firmware for a desk-side Claude Code usage monitor. Currently CYD-only
(ESP32-2432S028R "Cheap Yellow Display"). HAL pattern preserved under
`firmware/src/hal/` + `boards/<name>/` so future ports stay clean. Adding a
board means dropping in a new folder + a new `[env:...]` block — `main.cpp`,
`ui.cpp`, and `splash.cpp` never see board-specific code. See
[`docs/porting/adding-a-board.md`](docs/porting/adding-a-board.md) and the
`boards/template/` skeleton.

Active port:

- `boards/esp32_2432s028r/` — ESP32-2432S028R "Cheap Yellow Display" (ILI9341, 320×240 landscape, XPT2046 resistive touch, single BOOT button, no PMU/IMU). Build env: `esp32_2432s028r` (**default_envs**).

The shared code calls a small HAL (`firmware/src/hal/`) that each board implements: display, touch, input, power, IMU. Optional features are guarded by `BoardCaps` (runtime) and `BOARD_HAS_*` (compile-time) rather than `#ifdef BOARD_*`.

Connects to a host daemon over BLE; daemon polls Anthropic API for usage data. This file is for future Claude Code sessions to bootstrap quickly. Read this first.

## Hardware (critical pins)

### CYD — ESP32-2432S028R
- Display: **ILI9341** 2.8" TFT via SPI (MOSI=13, SCLK=14, CS=15, DC=2, RST=12, BL=21)
- Touch: **XPT2046** resistive via dedicated SPI (MOSI=32, MISO=39, SCLK=25, CS=33, INT=36). Hit areas expanded +12 px inside ui.cpp to compensate for resistive inaccuracy.
- PMU: **none** — no battery, no AXP2101
- IMU: **none** — `BOARD_HAS_ROTATION=1` but rotation is software (display driver), not IMU-driven
- Buttons: GPIO 0 (BOOT → hold: Space/voice-mode). **No secondary button** — no Shift+Tab HID output.
- USB: **CH340** USB-serial — must press BOOT+RST to enter flash mode; upload speed capped at 460800 baud (CH340 is unreliable at 921600 on macOS). Upload port: `/dev/cu.wchusbserial*` (macOS) or `/dev/ttyUSB0` (Linux).
- Partition: `huge_app.csv` — firmware exceeds the 1 MB default app partition.
- Logo: static PNG replaced with animated 40×40 pixel-art canvas (`splash_render_small()` → `logo_canvas_buf`) on the usage screen. Driven by `ui_tick_anim`.
- Settings: hamburger menu button (top-right, 40×40) on usage screen → Brightness / Pair Mode.

## Architecture

```text
firmware/src/
  hal/                      — board-agnostic interfaces shared code calls into
    board_caps.h            — runtime BoardCaps struct (W, H, button_count, has_* flags)
    display_hal.h           — init / begin / set_brightness / draw_bitmap / tick / round_area
    touch_hal.h             — init / read(&x, &y, &pressed)
    input_hal.h             — init / is_held(PRIMARY|SECONDARY)
    power_hal.h             — init / tick / battery_pct / is_charging / pwr_pressed (edge)
    imu_hal.h               — init / tick / rotation_quadrant
  boards/
    esp32_2432s028r/        — ILI9341 + XPT2046 + BOOT button only, no PMU/IMU, animated mini-logo
    template/               — copy this to bootstrap a new port
  main.cpp                  — setup() + loop(): HAL calls only, zero #ifdef BOARD_*
  ui.{h,cpp}                — 3-screen UI (splash, usage, bluetooth). compute_layout() picks fonts/positions from board_caps() (responsive — current breakpoint: H >= 460 → large, else compact)
  splash.{h,cpp}            — 20×20 pixel-art engine. CELL = min(W,H)/20, centered.
  ble.{h,cpp}               — NimBLE peripheral: custom data service + HID keyboard
  idle.{h,cpp}              — fade in/out brightness state machine; sleep after IDLE_TIMEOUT_MS; wakes on button/touch and on UsageData value changes (ui.cpp:ui_update)
  data.h                    — UsageData struct
  icons.h                   — icon arrays. Battery (5×) are RGB565A8 with alpha; rest are raw RGB565.
  logo.h                    — 80×80 RGB565 logo
  font_*.c                  — pre-compiled LVGL 9 bitmap fonts (Tiempos 56/34, Styrene 48/28/24/20/16/14/12, Mono 32/18)
  splash_animations.h       — generated, do not hand-edit
docs/porting/               — adding-a-board.md, hal-contract.md, capability-flags.md
```

Each board folder contains: `board.h` (pins, I2C addresses, `BOARD_HAS_*` flags),
`board_init.cpp` (Wire.begin + any IO expander), `display.cpp`, `touch.cpp`,
`input.cpp`, `power.cpp`, `imu.cpp`, `caps.cpp` (the `BoardCaps` instance), plus
any board-private hardware drivers.
PlatformIO's `build_src_filter` includes shared code + one board's folder per env.

## Build / flash

```bash
pio run -d firmware                                                             # build CYD (default_envs)
pio run -d firmware -e esp32_2432s028r                                          # build CYD explicit
pio run -d firmware -e esp32_2432s028r -t upload --upload-port /dev/cu.wchusbserial14210      # flash CYD on macOS
pio run -d firmware -e esp32_2432s028r -t upload --upload-port /dev/ttyUSB0                   # flash CYD on Linux
```

Or use the wrapper scripts: `./flash-mac.sh [port]` / `./flash.sh [port]` — both default to CYD env, auto-detect port (`/dev/cu.wchusbserial*` macOS, `/dev/ttyUSB0` Linux).

If `pio` isn't on PATH: try `~/.platformio/penv/bin/pio` (Linux/macOS pio install) or `brew install platformio` on macOS.

Device path: `/dev/cu.wchusbserial*` on macOS, `/dev/ttyUSB0` on Linux — CH340 USB-serial, must hold BOOT then press RST to enter flash mode.

## QA your own UI changes — don't ask the user

CYD has `LV_USE_SNAPSHOT=0` — the snapshot framebuffer (~150 KB for 320×240) doesn't fit in internal SRAM. The serial `screenshot` command prints `SCREENSHOT_UNSUPPORTED`. For UI iteration on CYD, flash and observe the physical display directly.

The boot screen is `SCREEN_SPLASH` and only advances on a physical button press / touch, so a fresh flash will sit on the splash. To iterate on a different screen, **temporarily change the default boot screen** in `main.cpp` (search for `ui_show_screen(SCREEN_SPLASH);`) to `SCREEN_USAGE` / `SCREEN_CONTROLLER` / `SCREEN_BLUETOOTH`, do your iteration, then revert before committing.

## Critical gotchas

1. **pioarduino platform required.** GFX Library for Arduino needs Arduino Core 3.x (`esp32-hal-periman.h`), not the 2.x that standard `espressif32` ships. We pin `pioarduino/platform-espressif32` 55.03.38-1.
2. **LVGL 9 font patching.** `lv_font_conv` outputs LVGL 8 format. Must remove `#if LVGL_VERSION_MAJOR >= 8` guards, drop `.cache` field, add `.release_glyph`, `.kerning`, `.static_bitmap`, `.fallback`, `.user_data`. Without patching, fonts render invisible.
3. **Touch reading is centralized inside the board's `touch.cpp`.** The HAL `touch_hal_read()` is called once per loop from `my_touch_cb`; the board's implementation owns its latched `touch_pressed/x/y` state. Don't call the underlying controller from anywhere else.
4. **LVGL RGB565A8 is planar.** `w*h` RGB565 pixels followed by `w*h` alpha bytes; `data_size = w*h*3`, `stride = w*2`. Use `init_icon_dsc_rgb565a8()` for icons that overlap non-uniform backgrounds. Lucide source PNGs are black-on-transparent — converter must tint to white or icons render invisible. See `tools/png_to_lvgl.js`.
5. **No `#ifdef BOARD_*` in shared code.** The whole point of the HAL — if you're about to add one, you probably want a `BoardCaps` field or a per-board file instead. See `docs/porting/capability-flags.md`.
6. **CYD uses CH340 USB-serial, not native USB-JTAG.** Flash sequence: hold BOOT, press+release RST, release BOOT. Upload speed 460800 — CH340 fails at 921600 on macOS. Port is `/dev/cu.wchusbserial*` (macOS) or `/dev/ttyUSB0` (Linux).
7. **CYD has no PSRAM.** No `BOARD_HAS_PSRAM` flag. Splash canvas and LVGL buffers use `MALLOC_CAP_INTERNAL`. `huge_app.csv` partition required — firmware exceeds 1 MB default app partition.
8. **CYD animated logo on usage screen.** `logo_canvas_buf` in `ui.cpp` is a static array `[SPLASH_GRID * LOGO_MINI_CELL]²` (40×40 × 2 bytes). `ui_tick_anim` calls `splash_render_small(logo_canvas_buf, LOGO_MINI_CELL)` each tick to sync the mini canvas with the splash animation. Hamburger menu (top-right, 40×40 hit area expanded +12 px) replaces the hold-to-pair gesture absent from the PWR button.
9. **Idle wake on data change.** `ui_update()` in `ui.cpp` diffs new vs last-seen `session_pct`, `weekly_pct`, `status`, `ok` and calls `idle_note_activity()` on change. `reset_mins` is skipped (counts down every poll = constant wake). First valid sample seeds the baseline without waking.

## Icons

`tools/png_to_lvgl.js <input.png> <symbol> [W_MACRO] [H_MACRO] [--tint=RRGGBB | --no-tint]` converts an alpha PNG to RGB565A8. Default tint is white (`0xFFFFFF`) — necessary for Lucide PNGs. Splice output into `firmware/src/icons.h` and use `init_icon_dsc_rgb565a8()` in ui.cpp.

## Splash animations

13 × 20×20 pixel-art creature animations sourced from
[claudepix.vercel.app](https://claudepix.vercel.app). Pipeline:

```bash
node tools/scrape_claudepix.js  # → tools/claudepix_data/*.json
node tools/convert_to_c.js      # → firmware/src/splash_animations.h
```

Each animation has a per-animation 10-color RGB565 palette. Cell values 0..9 index it. Default boot screen.

## User profile / preferences

See `~/.claude/projects/.../memory/` files for persistent context (user is an embedded-beginner senior dev, brand-conscious, prefers iterative UI refinement, dislikes me authoring my own art when third-party assets are intended). Always read those memory files at session start.

## Recent session highlights

- **CYD-only + wake-on-data (2026-06-04).** Removed Waveshare AMOLED-2.16, AMOLED-1.8, AMOLED-2.16-C6 board ports — repo is now CYD-only. HAL + `boards/template/` + `docs/porting/` retained for future ports. `flash-mac.sh` / `flash.sh` simplified to board-less invocation. `ui_update()` now diffs `UsageData` and calls `idle_note_activity()` when session/weekly % or status change so the display wakes for new token usage data, not only button/touch.
- **CYD port (2026-06-03).** Added ESP32-2432S028R "Cheap Yellow Display" (320×240, ILI9341 SPI, XPT2046 resistive touch, CH340 USB-serial, no PSRAM/PMU/IMU). UI extended: small-landscape layout in `compute_layout()`, animated mini-logo canvas (40×40) replacing the static logo, hamburger menu (brightness + pair mode) replacing hold-to-pair. `splash_render_small()` added to `splash.cpp` for the canvas.
- **Device-abstraction refactor (2026-05-18).** All board-conditional code moved out of shared files into `boards/<name>/` and behind a HAL in `hal/`. ~30 `#ifdef BOARD_*` blocks went to zero. UI is responsive via `compute_layout()` driven by `board_caps()`. New ports add a folder + a PlatformIO env — no shared file edits.

## Daemon / host side

Bash daemon (`daemon/claude-usage-daemon.sh`) reads OAuth token, polls Anthropic API, sends JSON over BLE GATT. Run with `systemctl --user start claude-usage-daemon`. The unit file's `ExecStart` is the absolute path to the script — repoint it when switching between the worktree and the main checkout.

**Discovery & resilience:**

- Connects by name (`"Clawdmeter"`) on first run, caches resolved MAC at `~/.config/claude-usage-monitor/ble-address`. ESP32 BLE addresses are factory-burned per-chip, so swapping any board invalidates the cache.
- On connect failure: cache is dropped AND device is removed from bluez (`bluetoothctl remove`) so the next scan won't re-pick a dead MAC. Multi-candidate scans pick `head -1` and let the failure cycle converge.
- `POLL_INTERVAL=60`, `TICK=5`. Inner loop wakes every 5s to detect disconnects fast; polls Anthropic when 60s elapsed OR when ESP fires a refresh request.

**GATT characteristics on service `4c41555a-...0001`:**

- `...0002` RX — daemon writes JSON usage payload here.
- `...0003` TX — firmware notifies ack/nack (daemon doesn't subscribe).
- `...0004` REQ — firmware fires `0x01` notify in `onSubscribe` if `has_received_data` is false. Daemon subscribes via `setsid bash -c "stdbuf -oL dbus-monitor … | awk …"`; awk drops a flag file the inner loop picks up. See the `feedback_dbus_monitor_pipe` memory for the three subtle gotchas (pipe buffering, busctl-exits race, `wait` blocking on pipeline jobs).
