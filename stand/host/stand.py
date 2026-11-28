# /// script
# requires-python = ">=3.10"
# dependencies = ["pyserial>=3.5"]
# ///
"""Стенд UlpanIR на Mac: прошивка пульта и проверка платы через ESP32-C6 (openspec add-test-stand).

Стенд знает только команды `#…` (stand/README.md); сценарий, коды и допуски живут здесь (design §10).

    uv run stand/host/stand.py [--port P] flash <bin>   прошить образ
    uv run stand/host/stand.py [--port P] test [--long]        проверить уже прошитую плату (настройки стираются)
    uv run stand/host/stand.py [--port P] run [--long] <bin>   прошить, стереть настройки, проверить
    uv run stand/host/stand.py [--port P] monitor [#команда ...]   вывести порт как есть

--long — долгий тест: проверка 6 ждёт, пока режим настройки погаснет сам (60 с). Без него проверка 6 пропущена,
режим настройки гасится перезапуском чипа по отладке (`#chip`), сценарий идёт около 30 с (оценка).

Код возврата: 0 — ГОДНА (или команда выполнена), 1 — БРАК или ошибка, 2 — стенд не найден.
"""

from __future__ import annotations

import argparse
import contextlib
import datetime as dt
import itertools
import re
import sys
import time
import zlib
from dataclasses import dataclass, field
from pathlib import Path

ESPRESSIF_VID = 0x303A
REPORTS = Path(__file__).resolve().parent.parent / "reports"

# Проверяемые коды: A = 0x32/0x02, B = 0x32/0x03, 3 кадра/с, полный кадр (docs/ble_protocol.md).
CONFIG_HEX = "0101320002013200030300"
CODE_A = ("0x32", "0x02")
CODE_B = ("0x32", "0x03")
REPEAT_MS = 1000 / 3
REPEAT_TOL = 0.10

OFF_AFTER_RELEASE_S = 2.0  # «выкл не позже 2 с» (design §6)
CONFIG_TIMEOUT_S = 70.0  # режим настройки сам гаснет через 60 с без команд
FRAME_TAIL_S = 0.3  # после отпускания пульт досылает идущий кадр

NEC_RE = re.compile(r"^# ir: nec (0x[0-9a-f]+) (0x[0-9a-f]+) t=(\d+)$")
POWER_RE = re.compile(r"^# power: (вкл|выкл) ([0-9.]+) В$")


class StandError(Exception):
    """Стенд не ответил или ответил не тем."""


# --------------------------------------------------------------------------- порт


def find_port() -> str | None:
    from serial.tools import list_ports

    for p in sorted(list_ports.comports(), key=lambda p: p.device):
        if p.vid == ESPRESSIF_VID and p.device.startswith("/dev/cu.usbmodem"):
            return p.device
    return None


def open_port(path: str):
    """RTS=0 и DTR=0 до open(): USB Serial/JTAG C6 перезагружает чип при DTR=0, RTS=1 (stand-port)."""
    import serial

    s = serial.Serial()
    s.port = path
    s.baudrate = 115200
    s.timeout = 0.02
    s.rts = False
    s.dtr = False
    s.open()
    return s


@dataclass
class Line:
    t: float  # время прихода по часам Link, с
    text: str

    @property
    def stand(self) -> bool:
        return self.text.startswith("# ")


class Link:
    """Один поток порта: строки стенда (`# …`) и лог пульта. Все строки копятся в self.lines."""

    def __init__(self, transport, clock=time.monotonic, sleep=time.sleep, echo=None):
        self.t = transport
        self.clock = clock
        self.sleep = sleep
        self.echo = echo
        self.lines: list[Line] = []
        self._buf = b""

    def pump(self) -> None:
        data = self.t.read(4096)
        if not data:
            self.sleep(0.01)
            return
        self._buf += data
        while b"\n" in self._buf:
            raw, self._buf = self._buf.split(b"\n", 1)
            text = raw.decode("utf-8", "replace").rstrip("\r")
            self.lines.append(Line(self.clock(), text))
            if self.echo:
                self.echo(text)

    def mark(self) -> int:
        return len(self.lines)

    def send(self, cmd: str) -> int:
        self.pump()
        m = self.mark()
        self.t.write((cmd + "\n").encode())
        return m

    def wait(self, prefix: str, since: int, timeout: float) -> Line:
        """Первая строка стенда, начинающаяся с `# <prefix>`, после индекса since."""
        end = self.clock() + timeout
        i = since
        while True:
            while i < len(self.lines):
                ln = self.lines[i]
                i += 1
                if ln.stand and ln.text.startswith("# " + prefix):
                    return ln
            if self.clock() >= end:
                raise StandError(f"нет ответа «# {prefix}» за {timeout:g} с")
            self.pump()

    def call(self, cmd: str, prefix: str, timeout: float = 5.0) -> str:
        return self.wait(prefix, self.send(cmd), timeout).text

    def idle(self, seconds: float) -> None:
        end = self.clock() + seconds
        while self.clock() < end:
            self.pump()

    def flash(self, image: bytes, timeout: float = 90.0) -> str:
        m = self.send(f"#flash {len(image)} {zlib.crc32(image) & 0xFFFFFFFF:08x}")
        for i in range(0, len(image), 4096):
            self.t.write(image[i : i + 4096])
        return self.wait("flash:", m, timeout).text


# --------------------------------------------------------------------------- сценарий


@dataclass
class Check:
    num: int
    name: str
    ok: bool
    value: str
    skipped: bool = False

    @property
    def mark(self) -> str:
        return "пропущена" if self.skipped else ("да" if self.ok else "нет")


@dataclass
class Result:
    checks: list[Check] = field(default_factory=list)
    uid: str = "nouid"

    @property
    def failed(self) -> list[int]:
        return [c.num for c in self.checks if not c.ok and not c.skipped]

    def verdict(self) -> str:
        return "ГОДНА" if not self.failed else "БРАК: " + " ".join(map(str, self.failed))


NAMES = {
    1: "чип CH59x",
    2: "прошивка и чистые настройки",
    3: "выключен после прошивки",
    4: "A без кода: нет ИК, выключился",
    5: "режим настройки и запись кодов",
    6: "режим настройки погас сам",
    7: "A: один кадр 0x32/0x02, выключился",
    8: "B: один кадр 0x32/0x03",
    9: "A 2 с: 6–7 кадров через 333 мс",
    10: "A+B 0,5 с: ни одного кадра",
}


class Scenario:
    def __init__(self, link: Link, image: bytes | None, long: bool = False):
        self.l = link
        self.image = image
        self.long = long
        self.r = Result()

    def add(self, num: int, ok: bool, value: str, skipped: bool = False) -> None:
        self.r.checks.append(Check(num, NAMES[num], ok, value, skipped))

    # --- измерения

    def power_off_within(self, seconds: float) -> tuple[bool, str]:
        """Опрос `#power`, пока не «выкл» или не вышло время. Возвращает (выкл?, последний ответ)."""
        end = self.l.clock() + seconds
        last = ""
        while True:
            last = self.l.call("#power", "power:")
            m = POWER_RE.match(last)
            if m and m.group(1) == "выкл":
                return True, last[2:]
            if self.l.clock() >= end:
                return False, last[2:]
            self.l.idle(0.2)

    def press(self, button: str, ms: int, tail: float = FRAME_TAIL_S) -> tuple[list[tuple[str, str, int]], list[str]]:
        """Нажатие с приёмом ИК: кадры nec (адрес, команда, t) и строки лога от нажатия до конца хвоста tail."""
        self.l.call("#ir on", "ir:")
        m = self.l.send(f"#press {button} {ms}")
        self.l.wait("press:", m, ms / 1000 + 5)
        self.l.idle(tail)
        self.l.call("#ir off", "ir:")
        frames, log = [], []
        for ln in self.l.lines[m:]:
            nm = NEC_RE.match(ln.text)
            if nm:
                frames.append((nm.group(1), nm.group(2), int(nm.group(3))))
            elif not ln.stand:
                log.append(ln.text)
        return frames, log

    # --- проверки

    def run(self) -> Result:
        steps = [self.c1, self.c2, self.c3, self.c4, self.c5, self.c6, self.c7, self.c8, self.c9, self.c10]
        for i, step in enumerate(steps, 1):
            try:
                step()
            except StandError as e:
                self.add(i, False, str(e))
            if i <= 2 and not self.r.checks[-1].ok:
                for j in range(i + 1, 11):
                    self.add(j, False, "не проводилась")
                break
        self.l.call("#lines off", "lines:")
        return self.r

    def c1(self):
        ans = self.l.call("#chip", "chip:")
        m = re.search(r"uid=([0-9a-f]{16})", ans)
        if m:
            self.r.uid = m.group(1)
        self.add(1, ans.startswith("# chip: CH59x"), ans[2:])

    def c2(self):
        parts = []
        ok = True
        if self.image is not None:
            ans = self.l.flash(self.image)
            ok = ans.startswith("# flash: записано")
            parts.append(ans[2:])
        if ok:
            ans = self.l.call("#erase data", "erase:")
            ok = ans == "# erase: настройки стёрты"
            parts.append(ans[2:])
        self.add(2, ok, "; ".join(parts))

    def c3(self):
        ok, val = self.power_off_within(OFF_AFTER_RELEASE_S)
        self.add(3, ok, val)

    def c4(self):
        frames, log = self.press("a", 100, tail=0.9)  # «за 1 с ни одного ИК-кадра»
        no_code = any("A has no code" in s for s in log)
        off, val = self.power_off_within(OFF_AFTER_RELEASE_S)
        ok = not frames and no_code and off
        why = [f"кадров {len(frames)}", "A has no code" if no_code else "нет «A has no code»", val]
        self.add(4, ok, "; ".join(why))

    def c5(self):
        _, log = self.press("ab", 4500)
        mode = any("-> config mode" in s for s in log)
        if not mode:
            return self.add(5, False, "нет «-> config mode» в логе")
        con = self.l.call("#ble connect", "ble:", timeout=15)
        if not con.startswith("# ble: подключён"):
            return self.add(5, False, con[2:])
        w = self.l.call(f"#ble write config {CONFIG_HEX}", "ble:")
        rd = self.l.call("#ble read config", "ble:")
        self.l.call("#ble close", "ble:")
        ok = w == "# ble: записано config" and rd == f"# ble: config {CONFIG_HEX}"
        self.add(5, ok, f"{con[7:]}; {w[7:]}; {rd[7:]}")

    def c6(self):
        if self.long:
            ok, val = self.power_off_within(CONFIG_TIMEOUT_S)
            return self.add(6, ok, val)
        # Короткий тест: не ждать 60 с, а перезапустить МК по отладке — режим настройки кончается, пульт гаснет.
        chip = self.l.call("#chip", "chip:")
        off, val = self.power_off_within(OFF_AFTER_RELEASE_S)
        if off:
            return self.add(6, True, f"без --long; погашен перезапуском `#chip`, {val}", skipped=True)
        self.add(6, False, f"после перезапуска `#chip` не выключился: {chip[2:]}; {val}")

    def c7(self):
        frames, _ = self.press("a", 100)
        off, val = self.power_off_within(OFF_AFTER_RELEASE_S)
        codes = [(a, c) for a, c, _ in frames]
        self.add(7, codes == [CODE_A] and off, f"кадры {fmt(codes)}; {val}")

    def c8(self):
        frames, _ = self.press("b", 100)
        codes = [(a, c) for a, c, _ in frames]
        self.power_off_within(OFF_AFTER_RELEASE_S)
        self.add(8, codes == [CODE_B], f"кадры {fmt(codes)}")

    def c9(self):
        frames, _ = self.press("a", 2000)
        codes = [(a, c) for a, c, _ in frames]
        gaps = [b[2] - a[2] for a, b in itertools.pairwise(frames)]
        lo, hi = REPEAT_MS * (1 - REPEAT_TOL), REPEAT_MS * (1 + REPEAT_TOL)
        ok = len(frames) in (6, 7) and all(c == CODE_A for c in codes) and all(lo <= g <= hi for g in gaps)
        self.power_off_within(OFF_AFTER_RELEASE_S)
        self.add(9, ok, f"кадров {len(frames)}, промежутки {gaps} мс")

    def c10(self):
        frames, _ = self.press("ab", 500)
        self.power_off_within(OFF_AFTER_RELEASE_S)
        self.add(10, not frames, f"кадров {len(frames)}")


def fmt(codes) -> str:
    return "[" + ", ".join(f"{a}/{c}" for a, c in codes) + "]" if codes else "нет"


def report_text(res: Result, lines: list[Line]) -> str:
    out = [f"{c.num}. {c.name}: {c.mark} — {c.value}" for c in res.checks]
    out.append(res.verdict())
    out.append("")
    out.append("--- порт стенда ---")
    out += [ln.text for ln in lines]
    return "\n".join(out) + "\n"


def save_report(res: Result, lines: list[Line], when: dt.datetime | None = None, folder: Path = REPORTS) -> Path:
    when = when or dt.datetime.now()
    folder.mkdir(parents=True, exist_ok=True)
    path = folder / f"{when:%Y-%m-%d_%H%M%S}_{res.uid}.txt"
    path.write_text(report_text(res, lines), encoding="utf-8")
    return path


def run_checks(link: Link, image: bytes | None, out=print, folder: Path = REPORTS, long: bool = False) -> int:
    res = Scenario(link, image, long).run()
    for c in res.checks:
        out(f"{c.num:2}. {c.name}: {c.mark if c.skipped else c.mark.ljust(3)} — {c.value}")
    out(res.verdict())
    path = save_report(res, link.lines, folder=folder)
    out(f"Протокол: {path}")
    out(f"Папка:    {path.parent}/")
    return 0 if not res.failed else 1


# --------------------------------------------------------------------------- CLI


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--port", help="порт стенда; по умолчанию первый /dev/cu.usbmodem* с VID 0x303A")
    ap.add_argument("-v", "--verbose", action="store_true", help="печатать весь поток порта")
    sub = ap.add_subparsers(dest="cmd", required=True)
    sub.add_parser("flash").add_argument("bin", type=Path)
    long = argparse.ArgumentParser(add_help=False)
    long.add_argument(
        "--long", action="store_true", help="долгий тест: ждать, пока режим настройки погаснет сам (60 с)"
    )
    sub.add_parser("test", parents=[long])
    sub.add_parser("run", parents=[long]).add_argument("bin", type=Path)
    mon = sub.add_parser("monitor")
    mon.add_argument("commands", nargs="*", help="команды стенда, например '#info'")
    a = ap.parse_args(argv)

    path = a.port or find_port()
    if not path:
        print("Стенд не найден: нет /dev/cu.usbmodem* с VID 0x303A (Espressif). Укажите --port.", file=sys.stderr)
        return 2
    image = a.bin.read_bytes() if a.cmd in ("flash", "run") else None
    port = open_port(path)
    echo = (lambda s: print("  | " + s)) if a.verbose or a.cmd == "monitor" else None
    link = Link(port, echo=echo)
    try:
        if a.cmd == "monitor":
            for c in a.commands:
                link.send(c)
            while True:
                link.pump()
        if a.cmd == "flash":
            ans = link.flash(image)
            print(ans)
            return 0 if ans.startswith("# flash: записано") else 1
        return run_checks(link, image, long=a.long)
    except KeyboardInterrupt:
        return 0
    except StandError as e:
        print(f"Ошибка: {e}", file=sys.stderr)
        return 1
    finally:
        # Порт мог уже пропасть (стенд выдернули): отпустить линии не удалось — закрыть всё равно.
        with contextlib.suppress(OSError):
            link.t.write(b"#lines off\n")
        port.close()


if __name__ == "__main__":
    sys.exit(main())
