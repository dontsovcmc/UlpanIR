# Стенд UlpanIR (ESP32-C6 Super Mini): сборка и прошивка через PlatformIO.
#
#   make flash   -> собрать и прошить Super Mini по USB (pio run -t upload)
#   make build   -> только собрать: .pio/build/esp32c6/firmware.factory.bin — один образ с адреса 0x0
#   make test    -> тесты core/ на Mac (pio test -e native) и скрипта на поддельном стенде
#   make web     -> образ и manifest.json для страницы прошивки из браузера: ../web/stand/firmware/
#   make serve   -> страница локально: http://localhost:8000/stand/ (Web Serial — только https или localhost)
#
# Плата уже со стендом: зажать BOOT, вставить USB, отпустить — стенд выключает сброс по USB (README.md).

PIO ?= $(shell command -v pio 2>/dev/null || echo $(HOME)/.platformio/penv/bin/pio)
ENV = esp32c6
FACTORY = .pio/build/$(ENV)/firmware.factory.bin
WEB = ../web/stand/firmware
IMAGE = ulpanir_stand-esp32c6.bin
VERSION = $(shell grep -o 'FW_VERSION=\\"[0-9.]*' platformio.ini | grep -o '[0-9][0-9.]*')

build:
	$(PIO) run -e $(ENV)

flash:
	$(PIO) run -e $(ENV) -t upload

test:
	$(PIO) test -e native
	uv run --with pytest --with pyserial pytest host

web: build
	mkdir -p $(WEB)
	cp $(FACTORY) $(WEB)/$(IMAGE)
	printf '%s\n' \
	  '{' \
	  '  "name": "UlpanIR stand",' \
	  '  "version": "$(VERSION)",' \
	  '  "new_install_prompt_erase": true,' \
	  '  "new_install_improv_wait_time": 0,' \
	  '  "builds": [{"chipFamily": "ESP32-C6", "parts": [{"path": "$(IMAGE)", "offset": 0}]}]' \
	  '}' > $(WEB)/manifest.json
	@echo "Образ:  $(abspath $(WEB))/$(IMAGE)"
	@echo "Папка:  $(abspath $(WEB))/"

serve: web
	python3 -m http.server 8000 --bind 127.0.0.1 -d ../web

.PHONY: build flash test web serve
