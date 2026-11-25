# Proposal

## Why

После проверки ИК, настройки и питания на макете нужна своя плата в корпусе брелока. Четвёртая, последняя задача пульта: плата, заказ на JLCPCB, прошивка с ESP32 и проверка готового пульта на проекторе.

## What Changes

- Замеры корпуса-донора (брелок типа AB Shutter3).
- Проект EasyEDA Pro: МК по референсу Nordic QCAA, защёлка с номиналами из `add-power-latch`, ИК-каскад на двух VSMY14940 с JP1, площадки SWD.
- Gerber, BOM с кодами LCSC, CPL; заказ платы и сборки на JLCPCB.
- Первая прошивка с ESP32 по SWD; проверки готового пульта: питание, BLE в корпусе, дальность до Optoma X341.

## Capabilities

### New Capabilities
- `board-hardware`: плата и корпус — референс Nordic, радио в корпусе, ИК-светодиоды и JP1, SWD, габариты, изготовление на JLCPCB, дальность.

### Modified Capabilities
<!-- Нет. -->

## Impact

- Новый каталог `hardware/` (проект, `fab/`, `enclosure.md`), дополнение `docs/measurements.md`.
- Зависит от результатов `add-power-latch` (номиналы, решение по защёлке).
- Внешнее: EasyEDA Pro, JLCPCB, ESP32 с ESP32_nRF52_SWD, проектор Optoma X341 для проверки.
