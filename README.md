# ИК-пульт на nRF52810 для проектора Optoma X341 — пакет для проектирования, прошивки и пайки

Дата сборки: 2026-11-25. Всё ниже собрано из источников, найденных в интернете в ходе сессии; свои расчёты и допущения помечены явно. Что не удалось подтвердить — в разделе 7.

## 1. Что делаем (принятые решения)

- Донор: двухкнопочный Bluetooth-брелок для селфи (тип AB Shutter3), свой корпус, плата своя. Размеры корпуса в модели оценочные (нужны замеры штангенциркулем).
- МК: **nRF52810-QCAA (QFN32 5×5)**, схема МК **строго по официальному референсу Nordic**, кварц 32 МГц обязателен. Питание CR2032.
- Настройка с ПК: BLE + Web Bluetooth (Chrome/Edge). Режим настройки: зажать **обе** кнопки 2–3 с; BLE ~1 мин, затем само выключение.
- Прошивка чистого чипа: SWD с ESP32.
- Питание: мягкая защёлка (P-FET + N-FET, кнопки через диоды) — **моя разработка, готовой схемы не искал**; плавный старт Q1 (Rg 100 кОм + Cm 33 нФ) — моя схема по идее TI SLVA716.
- ИК: **2 × SMD side-looker VSMY14940** (940 нм), оба впаяны, LED2 отключён перемычкой JP1; протокол NEC, 3 кадра/с при удержании; цель — 10 м по оси.
- Заказ: JLCPCB (плата + сборка), проект в EasyEDA (Pro).

## 2. Состав архива

| Файл | Что это |
|---|---|
| `README.md` | этот файл |
| `schematic.png`, `schematic.svg` | принципиальная схема v5 (моя; блок B МК нужно заменить на референс Nordic, см. п. 3) |
| `pcb_top_view.png` | вид сверху на плату, компоновка-эскиз (дорожки не проведены) |
| `fob_1_closed.png`, `fob_2_exploded.png`, `fob_3_open_top.png`, `fob_4_open_iso.png` | 3D-рендеры брелка |
| `fob_model_stl.zip` | STL-модель корпуса и платы |
| `BOM_MCU_nRF52810_QCAA.csv` | BOM МК-части по референсу Nordic; коды LCSC только найденные |
| `sim_burst.json`, `sim.py`, `sim2.py` | расчёт токов ИК-каскада (модель Эйлера) |
| `scripts/build_model.py`, `scripts/schematic.py`, `scripts/pcb_top.py` | скрипты генерации модели, схемы и вида сверху |

## 3. Плата: МК и референс

- Официальный референс Nordic (QCAA QFN32: LDO и DC/DC, BOM, правила разводки): https://docs.nordicsemi.com/r/bundle/ps_nrf52810/page/ref_circuitry.html
  - Состав (LDO): C1, C2, C11, C12 12 пФ NP0 0402; C3 0,8 пФ NP0; C4, C5, C8 100 нФ X7R; C6 не монтируется; C7 100 пФ NP0; C9 4,7 мкФ X5R 0603; C10 1 мкФ X7R 0603; L1 3,9 нГн 0402; X1 32 МГц XTAL 2016, CL 8 пФ, ±40 ppm; X2 32,768 кГц (на схеме помечен Optional вместе с C11/C12 — в нашем BOM не используется).
  - Антенна: референс рассчитан на 50 Ом; около ANT менять нельзя; минимум 2 слоя с землёй, VSS на землю (via на каждый VSS), развязка у VDD, внутренние слои пустые под согласованием.
- Страница продукта Nordic (файлы референс-платы, Altium/разводка/production): https://nordicsemi.com/Products/nRF52810/Download  — показана версия «1.0 CAAA»; наличие файла именно QCAA не подтверждено.
- Спецификация nRF52810 (пины, QFN32): https://docs.nordicsemi.com/r/bundle/ps_nrf52810/page/pin.html
- Готовые проекты-образцы:
  - EasyEDA-схема nRF52810-QCAA-R (Nordic DevZone, beacon): https://devzone.nordicsemi.com/cfs-file/__key/communityserver-discussions-components-files/4/beacon.pdf
  - OSHWLab / EasyEDA Pro: nRF52810 + чип-антенна, 10×14 мм (positron96): https://oshwlab.com/positron96/ble-rc ; описание: https://positron96.gitlab.io/projects/ble-rc/index.html ; проект EasyEDA Pro: https://pro.easyeda.com/editor#id=5f86e326cba74d64a7422a910f110782
  - tscircuit nRF52810 CR2032 tracker (2 слоя, QFN48 QFAA, чип-антенна Walsin, BOM): https://github.com/tscircuit/nrf52810
  - Flux nRF52810-QCAA beacon (PI-цепь + меандр): https://www.flux.ai/acolborn/nrf52810-qcaa
  - Flux part nRF52810-QCAA-T: https://www.flux.ai/lcsc/nrf52810-qcaa-t~azxf
- LCSC/JLCPCB:
  - nRF52810-QCAA-R, **C519278**, QFN-32-EP(5×5), Extended, PCBA Economic и Standard: https://jlcpcb.com/partdetail/NordicSemicon-nRF52810_QCAAR/C519278
  - Кварц 32 МГц, 8 пФ, 2016: NDK NX2016SA-32MHZ-EXS00A-CS06654 **C5441424** (https://jlcpcb.com/partdetail/NDK-NX2016SA_32MHZ_EXS00ACS06654/C5441424); Raltron R2016-32.000-8-F-1010-EXT-TR C1986149 (https://jlcpcb.com/partdetail/C1986149); YXC XL7EL89CKI-111YLC-32M C2965582 (https://jlcpcb.com/partdetail/C2965582). Допуск по ppm надо проверять по даташиту (Nordic требует ±40 ppm суммарно).
- Возможности JLCPCB:
  - Плата: https://jlcpcb.com/capabilities/pcb-capabilities ; толщины FR4 0,4/0,6/0,8/1,0/1,2/1,6/2,0 мм.
  - Сборка (мин. шаг выводов 0,4 мм Economic, QFN поддерживается): https://jlcpcb.com/solutions/pcb-prototype-assembly ; https://jlcpcb.com/capabilities/pcb-assembly-capabilities (Economic — одностороннее размещение, Standard — двустороннее).
  - Слои/стек/импеданс: https://jlcpcb.com/quote/pcbOrderFaq/PCB%20Stackup
  - Для двусторонней сборки (если на обратной стороне будут детали) нужен Standard PCBA.
- Импорт в EasyEDA:
  - Altium/Eagle/KiCad → EasyEDA Std: https://docs.easyeda.com/en/Import/Import-Altium-Designer/ ; FAQ https://docs.easyeda.com/en/FAQ/Import-Export/
  - EasyEDA Pro (Altium ASCII, KiCad 5.1/5.9, Format Converter): https://prodocs.easyeda.com/en/import-export/import-altium-designer/ ; https://prodocs.easyeda.com/en/import-export/import-kicad/ ; https://prodocs.easyeda.com/en/import-export/easyeda-pro-format-converter/
  - Миграция Std → Pro: https://prodocs.easyeda.com/en/faq/import-export/
  - KiCad умеет импортировать EasyEDA: https://dev-docs.kicad.org/en/import-formats/easyeda/index.html

## 4. Прошивка и настройка

- Прошивка чистого чипа с ESP32 по SWD (Aaron Christophel): https://github.com/atc1441/ESP32_nRF52_SWD (fork: https://github.com/jameshans/ESP32_nRF52_SWD). Подключение: SWDCLK → GPIO 21, SWDIO → GPIO 19, GND общий, питание nRF «как нужно» (у нас — 3 В на J1 VBAT и замкнуть кнопку, чтобы открылся Q1). Для снятия защиты чтения нужна цепь с MOSFET и GPIO 22/5 (см. README репозитория). Явно про nRF52810 в README не сказано — проверить на макете. Обзор: https://hackaday.com/2021/07/01/esp32-turned-handy-swd-flasher-for-nrf52-chips
- OpenOCD (запасной путь): https://www.github.com/seemoo-lab/openhaystack/wiki/Flashing-nRF-with-OpenOCD---ST-Link ; обсуждение прошивки nRF52810 через openocd (старый пост, ошибки mass erase на ранних версиях): https://devzone.nordicsemi.com/f/nordic-q-a/33140/programming-nrf52810-with-openocd-and-raspberry-pi/156523
- Среда разработки: nRF Connect SDK / Zephyr, цель `nrf52dk/nrf52810` (эмуляция nRF52810 на nRF52 DK; у самого Nordic dev-kit для 52810 нет): https://docs.zephyrproject.org/latest/boards/nordic/nrf52dk/doc/index.html ; https://developer.nordicsemi.com/nRF_Connect_SDK/doc-legacy/2.8.0/nrf/app_dev/device_guides/nrf52/index.html
- PWM на nRF52810 в Zephyr (нужен overlay; аппаратный PWM у 52810 есть): https://devzone.nordicsemi.com/f/nordic-q-a/90753/compiler-error-regarding-pwm-configuration/382955
- Web Bluetooth (настройка с ПК): https://developer.chrome.com/docs/capabilities/bluetooth ; https://developer.mozilla.org/en-US/docs/Web/API/Web_Bluetooth_API ; совместимость: https://caniuse.com/mdn-api_bluetooth_requestdevice (Firefox не поддерживает; только HTTPS и по жесту пользователя).
- Протокол NEC: Altium TechDocs https://techdocs.altium.com/display/FPGA/NEC Infrared Transmission Protocol ; краткое резюме https://finalrewind.org/ir_codes/nec_protocol ; Arduino-IRremote (справка по кодированию NEC): https://github.com/Arduino-IRremote/Arduino-IRremote
  - Тайминги: несущая 38 кГц, импульс 562,5 мкс, «0» — 562,5 мкс пауза, «1» — 1687,5 мкс, преамбула 9 мс + 4,5 мс, затем адрес, инверсия адреса, команда, инверсия команды, стоп-импульс. Повтор при удержании — 9 мс + 2,25 мс + 562,5 мкс.
- Коды Optoma: адрес 0x32 (байты 32 CD); в другом руководстве Optoma (DS349, стр. 51) показаны примеры таблицы: https://www.manualsdir.com/manuals/848764/optoma-ds349.html?page=51 . Руководство H183x/X341/W341/W331/S341, раздел «IR remote codes» на стр. 51: https://manualmachine.com/optoma/w331/5960871-user-manual (текст раздела через загрузчик не читается — **коды Вкл/Выкл для X341 надо взять из PDF руководства вручную**).

## 5. ИК-светодиоды и цепь

- VSMB14940 (side view, 3,2×2,51×1,2 мм, 35 мВт/ср при 70 мА, ±9°): https://wwwlegacy.vishay.com/docs/84210/vsmb14940.pdf
- VSMY14940 (side view, 82 мВт/ср, ±9°, 3,2×2,51×1,2 мм): https://www.farnell.com/datasheets/2049251.pdf  (не путать с VSMY14940UL: https://www.vishay.com/docs/80542/vsmy14940ul.pdf — 70 мВт/ср при 70 мА)
- Сводка Vishay (VSLB3940 3 мм 65 мВт/ср ±22°; VSMB14940; 0805 VSMB1940X01 6 мВт/ср; VSMB2000X01 40 мВт/ср): https://www.vishay.com/docs/49868/49868_pt0210.pdf
- 0805 (для сравнения; для 10 м не годятся): VSMY1940ITX01 (10 мВт/ср, ±60°) https://www.vishay.com/docs/84271/vsmy1940itx01.pdf ; VSMY5940X01 (10–19 мВт/ср при 100 мА) https://docs.rs-online.com/4bbd/A700000015373002.pdf ; VSMB1940X01 https://www.digikey.com/en/products/detail/vishay-semiconductor-opto-division/VSMB1940X01/2713163
- Сверху-смотрящий для сравнения: VSMY2940RG (2,3×2,3×2,8 мм, ±10°, 120–145 мВт/ср): https://www.vishay.com/product/84221
- Формула дальности (моя оценка, по методу Vishay/Everlight): d = √(Ie / Ee_min), Ee_min приёмника типично 0,12, худшее 0,25 мВт/м². Для 10 м нужно Ie ≥ 12…25 мВт/ср. Линейность Ie(I) предположена.

## 6. Батарея, питание, пайка

- CR2032, пики тока: TI SWRA349 «Coin cells and peak current draw» https://TI.com/lit/pdf/swra349 (конденсатор параллельно ячейке — самое эффективное; 30 мА против 15 мА снижают ёмкость в среднем на 9%) ; копия https://www.eenewseurope.com/download_whitepaper/24170
- Nordic + Energizer «High pulse drain impact on CR2032»: https://www.eenewseurope.com/download_whitepaper/25303
- NXP AN4573 (CR2032 с пиками до ~30 мА): https://www.nxp.com/docs/en/application-note/AN4573.pdf
- Duracell CR2032: https://www.duracell.com/wp-content/uploads/2021/06/CR2032CH_245mAh-CS.pdf
- Бросок тока при включении ёмкости / плавный старт: TI SLVA716 (дискретный PMOS, резистор в затвор замедляет нарастание) https://ti.com/lit/slva716 ; TI SLVA670 «Managing Inrush Current» https://ti.com/lit/slva670 ; TI SSZTBX2 https://ti.com/lit/pdf/ssztbx2 ; TI SLVSDK7C (расчёт slew rate по ёмкости) https://www.ti.com/document-viewer/lit/html/SLVSDK7C/managing-inrush-current-slvsdk72183 ; патент US 8988131 https://image-ppubs.uspto.gov/dirsearch-public/print/downloadPdf/8988131
- Пайка QFN (экспонированная площадка, трафарет, реф. профиль):
  - JLCPCB: https://jlcpcb.com/blog/how-to-solder-qfn-components-a-step-by-step-guide
  - TI SLUA271: https://www.ti.com/document-viewer/lit/html/SLUA271C/GUID-97E90003-17DF-4B4B-9F07-32B6C507B069 (трафарет 0,125 мм для QFN)
  - NXP AN1902: https://cn.nxp.com/docs/en/application-note/AN1902.pdf ; ST AN6533: https://www.st.com/resource/en/application_note/an6533-assembly-guidelines-for-qfn-quad-flat-nolead-and-son-small-outline-nolead-packages-stmicroelectronics.pdf ; Infineon AN72845: https://www.infineon.com/an72845 ; Microchip AN2089: https://www.farnell.com/datasheets/2076320.pdf
  - Maxim/ADI AN6382: https://www.analog.com/en/resources/app-notes/smt-assembly-and-pcb-design-guidelines-for-maxims-standard-wirebonded-quad-flatpack.html
- Мягкая защёлка питания: готовых схем не искал; схема в `schematic.png` — моя разработка, на макете не проверена.

## 7. Расчёты (модель, не замер) и что не подтверждено

Параметры: Vf0 = 1,17 В, rd = 2,3 Ом (Vf 1,33 В при 70 мА; rd взят от VSLB3940 — допущение), Q3 0,3 Ом, МК 3 мА, Cvdd 22 мкФ X5R (эфф. ~10 мкФ — допущение), скважность 1/3, RL1 = RL2 = 30 Ом.

- Один LED (JP1 разомкнут): 3,3 В / 8 Ом — LED ≈60 мА, из батареи среднее ≈23 мА, пик ≈25 мА; 3,0 В / 20 Ом — 45 мА и 18 мА; 2,6 В / 60 Ом — 24 мА и 11 мА.
- Два LED (JP1 замкнут): 3,3 В — по ≈56 мА на LED, из батареи среднее ≈40 мА, пик ≈44 мА; 3,0 В — ≈29 мА; 2,6 В — ≈14 мА, но Vmin на МК ≈1,72 В при пороге 1,7 В (мало запаса; запасной вариант RL = 39 Ом).
- Дальность по оси (Ie от тока линейно): два LED, 3,3 В — 23–33 м; 3,0 В — 19–28 м; 2,6 В — 13–18 м. Один LED: 17–24 м (свежая), 11–15 м (почти села). С запасом на прицеливание и окно (÷2,5) — в разы меньше.
- Плавный старт Q1: без ограничения бросок 150–410 мА; Rg 100 кОм + Cm 33 нФ — 23–29 мА, нарастание 7–10 мс (параметры FET — предположения).
- NEC: кадр 68,06 мс, несущая 27,56 мс; 3 кадра/с → средний ток при удержании ≈2 мА + ток LED; ≈0,2–0,26 мкА·ч на кадр.

**Не подтверждено / открыто:**
- коды Вкл/Выкл для Optoma X341 (стр. 51 руководства не прочитана), порог приёма проектора;
- файл референс-платы Nordic именно QCAA (скачать со страницы продукта не удалось); страница OSHWLab positron96 не читается;
- Vf(I) и график Ie(IF) VSMY14940, габариты и положение излучающей грани;
- реальные Vgs(th) Q1/Q2/Q3 при 2 В, ёмкость Cvdd под смещением, утечка 10 МОм-цепей, время старта МК против Cg;
- ток из батареи >30 мА при двух LED (риск принят пользователем), Vmin ≈1,72 В у почти севшей;
- замеры корпуса; LCSC-коды пассивов из BOM Nordic; ориентация side-looker в модели;
- проект EasyEDA (.epro) сам не создан: его нужно собрать из референса Nordic или импортировать готовый проект.

## 8. Порядок работ

1. Скачать референс Nordic (nRF52810 → Download) или клонировать готовый проект EasyEDA/OSHWLab; импортировать в EasyEDA Pro (раздел 3).
2. Заменить схему МК по референсу QCAA; добавить защёлку, ИК-каскад, JP1 и SWD-площадки (`schematic.png`).
3. Подобрать коды LCSC по BOM; проверить допуск кварца.
4. Оформить контур платы по `fob_model_stl.zip` / `pcb_top_view.png`; разместить светодиоды у торца; сверить с замерами корпуса.
5. Разводка по правилам Nordic (2 слоя, земля, via у VSS, развязка у VDD, чистая зона под ANT).
6. DFM-проверка и заказ JLCPCB (Economic PCBA — односторонняя сборка; Standard — двусторонняя).
7. Прошивка: ESP32 SWD (раздел 4), сборка в nRF Connect SDK для `nrf52dk/nrf52810` + overlay PWM; NEC 38 кГц; Web Bluetooth страница настройки.
8. Измерить ток с реальной CR2032 и дальность до проектора; при необходимости RL = 39 Ом.
