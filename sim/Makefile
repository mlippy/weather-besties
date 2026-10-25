# Desktop renderer for the Weather Besties UI. Needs gcc + python3 (pillow).
ROOT     := ..
LVGL_DIR := $(ROOT)/components/lvgl
UI_DIR   := $(ROOT)/components/wb_ui
CJSON    := cjson

# Which screens: the one picked in components/wb_ui/ui_select.h, unless overridden
# with  make UI=classic  or  make UI=v2.
UI ?= $(if $(filter 1,$(shell sed -n 's/^\#define WB_UI_CLASSIC \([01]\).*/\1/p' $(UI_DIR)/ui_select.h)),classic,v2)

SHARED_SRC := $(UI_DIR)/wx.c $(UI_DIR)/events.c $(UI_DIR)/cal.c $(UI_DIR)/alerts.c $(UI_DIR)/thermo.c
ifeq ($(UI),classic)
UI_SRC := $(UI_DIR)/classic/ui.c $(UI_DIR)/classic/advice.c \
          $(UI_DIR)/classic/ui_classic_extras.c $(wildcard $(UI_DIR)/classic/assets/*.c) \
          $(wildcard $(UI_DIR)/dog/img_dog_head*.c) classic/sim_main.c
SCENARIOS := classic/scenarios
MAKE_SCENARIOS := classic/make_scenarios.py
OUT := out-classic
else
UI_SRC := $(UI_DIR)/ui.c $(UI_DIR)/outfit.c $(UI_DIR)/cat.c $(UI_DIR)/ui_v2_extras.c \
          $(wildcard $(UI_DIR)/assets/*.c) $(wildcard $(UI_DIR)/dog/*.c) sim_main.c
SCENARIOS := scenarios
MAKE_SCENARIOS := make_scenarios.py
OUT := out
endif

SRC := $(shell find $(LVGL_DIR)/src -name '*.c') $(SHARED_SRC) $(UI_SRC) $(CJSON)/cJSON.c
OBJ := $(patsubst %.c,build/%.o,$(subst $(ROOT)/,root/,$(SRC)))

CFLAGS := -O2 -g -I$(ROOT)/components -I$(LVGL_DIR) -I$(UI_DIR) -I$(CJSON) \
          -DLV_LVGL_H_INCLUDE_SIMPLE -DLV_CONF_INCLUDE_SIMPLE -DLV_KCONFIG_IGNORE -Wno-unused-result -DWB_SIM \
          -MMD -MP   # rebuild a file when a header it uses changes

render-$(UI): $(OBJ)
	$(CC) -o $@ $^ -lm

-include $(OBJ:.o=.d)

build/root/%.o: $(ROOT)/%.c
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

build/%.o: %.c
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

shots: render-$(UI)
	python3 $(MAKE_SCENARIOS)
	mkdir -p $(OUT)
	./render-$(UI) $(OUT) $(SCENARIOS)
	python3 to_png.py $(OUT)
ifneq ($(UI),classic)
	python3 check_renders.py $(OUT)
endif

# Calendar reader and update manifest tests, and a check that every request the
# board makes fits the HTTP client's send buffer.
test: test_request
	@mkdir -p build
	$(CC) -Wall -Wextra -I$(UI_DIR) -o build/test_cal test_cal.c $(UI_DIR)/cal.c
	./build/test_cal
	$(CC) -Wall -Wextra -I$(UI_DIR) -I$(CJSON) -o build/test_update test_update.c $(UI_DIR)/update.c $(CJSON)/cJSON.c
	./build/test_update
	./test_request

test_request: test_request.c $(UI_DIR)/wb_request.c $(UI_DIR)/wb_request.h
	$(CC) -O2 -Wall -Wextra -I$(UI_DIR) -o $@ test_request.c $(UI_DIR)/wb_request.c

clean:
	rm -rf build render-classic render-v2 out-classic test_request


# ---- Browser emulator (WebAssembly) ----
# Needs wasi-sdk (https://github.com/WebAssembly/wasi-sdk). Point WASI_SDK at it.
WASI_SDK ?= /home/claude/wasm/wasi-sdk-33.0-x86_64-linux
WCC      := $(WASI_SDK)/bin/clang --target=wasm32-wasip1 --sysroot=$(WASI_SDK)/share/wasi-sysroot
WEB_SRC  := $(shell find $(LVGL_DIR)/src -name '*.c') \
            $(UI_DIR)/ui.c $(UI_DIR)/wx.c $(UI_DIR)/outfit.c $(UI_DIR)/cat.c $(UI_DIR)/cal.c $(UI_DIR)/wb_request.c \
            $(wildcard $(UI_DIR)/assets/*.c) $(wildcard $(UI_DIR)/dog/*.c) $(CJSON)/cJSON.c web/emu_main.c
WEB_OBJ  := $(patsubst %.c,build-web/%.o,$(subst $(ROOT)/,root/,$(WEB_SRC)))
WEB_CFLAGS := -O2 -MMD -MP -I$(ROOT)/components -I$(LVGL_DIR) -I$(UI_DIR) -I$(CJSON) -Iweb \
              -DLV_LVGL_H_INCLUDE_SIMPLE -DLV_CONF_INCLUDE_SIMPLE -DLV_KCONFIG_IGNORE \
              -D_WASI_EMULATED_SIGNAL -Wno-unused-command-line-argument

web/weather-besties.wasm: $(WEB_OBJ)
	$(WCC) -O2 -mexec-model=reactor -Wl,--strip-all -Wl,-z,stack-size=262144 -o $@ $^ -lm

build-web/root/%.o: $(ROOT)/%.c
	@mkdir -p $(dir $@)
	@$(WCC) $(WEB_CFLAGS) -c $< -o $@

build-web/%.o: %.c
	@mkdir -p $(dir $@)
	@$(WCC) $(WEB_CFLAGS) -c $< -o $@

web: web/weather-besties.wasm
	python3 web/build_page.py

-include $(WEB_OBJ:.o=.d)
