# Производственные файлы

| Файл | Что | Откуда |
|---|---|---|
| `Gerber_PCB1_v3_2026-11-26.zip` | Gerber и сверловка платы, разводка v3 (2 слоя, 37 × 24 мм) | EasyEDA Pro, PCB1 → Gerber, 26.11.2026 |

BOM и CPL (pick-and-place) для сборки JLCPCB ещё не выгружены — задача 3.2 в
`openspec/changes/add-board-hardware/tasks.md`. Гребёнка J1 в BOM не входит (только отверстия).

Исходники: проект EasyEDA — `hardware/easyeda/UlpanIR.epro2` (схема, PCB1 и копии разводок v1–v3),
схема в PDF/SVG/PNG и список соединений — `hardware/schematic/UlpanIR_schematic_easyeda.*`,
`hardware/schematic/UlpanIR_netlist.enet`.
