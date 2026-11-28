"""Тест скрипта stand.py на поддельном стенде: uv run --with pytest --with pyserial pytest stand/host

Поддельный стенд отвечает на команды так, как ответил бы стенд с исправной платой (строки — по прошивке пульта
firmware/src/main.c и спекам stand-*), на поддельных часах: сценарий с 70-секундным ожиданием идёт мгновенно.
"""

from __future__ import annotations

import datetime as dt
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import stand

CONFIG = "0101320002013200030300"


class FakeTime:
    def __init__(self):
        self.now = 0.0

    def clock(self) -> float:
        return self.now

    def sleep(self, s: float) -> None:
        self.now += s


class FakeStand:
    """Модель стенда с платой. Параметры порчи: frames_long — кадров на `#press a 2000`, stuck_on — пульт
    не выключается начиная с этого `#press`, ir_blocked — ИК не доходит до приёмника."""

    def __init__(self, tm: FakeTime, frames_long: int = 7, stuck_on_press: int | None = None, ir_blocked=False):
        self.tm = tm
        self.out: list[tuple[float, bytes]] = []
        self.inbuf = b""
        self.binary_left = 0
        self.flash_len = 0
        self.codes = False
        self.config_mode = False
        self.off_at = 0.0  # питание есть до этого времени
        self.ir_on_at = None
        self.presses = 0
        self.frames_long = frames_long
        self.stuck_on_press = stuck_on_press
        self.ir_blocked = ir_blocked
        self.ble = False

    # --- транспорт

    def write(self, data: bytes) -> None:
        self.inbuf += data
        while self.inbuf:
            if self.binary_left:
                k = min(self.binary_left, len(self.inbuf))
                self.inbuf = self.inbuf[k:]
                self.binary_left -= k
                if not self.binary_left:
                    self.emit(3.0, f"# flash: записано {self.flash_len} байт за 3000 мс")
                continue
            if b"\n" not in self.inbuf:
                return
            line, self.inbuf = self.inbuf.split(b"\n", 1)
            self.command(line.decode())

    def read(self, n: int) -> bytes:
        ready = [x for x in self.out if x[0] <= self.tm.now]
        self.out = [x for x in self.out if x[0] > self.tm.now]
        return b"".join(d for _, d in sorted(ready, key=lambda x: x[0]))

    # --- модель

    def emit(self, after: float, line: str) -> None:
        self.out.append((self.tm.now + after, (line + "\n").encode()))

    def stand_t(self, after: float) -> int:
        return int((self.tm.now + after - self.ir_on_at) * 1000)

    def frame(self, after: float, cmd: str) -> None:
        self.emit(after, f"   {int(after * 1000)} ms: frame 0x32/{cmd}")
        if self.ir_on_at is not None and not self.ir_blocked:
            self.emit(after + 0.07, f"# ir: nec 0x32 {cmd} t={self.stand_t(after + 0.07)}")

    def command(self, line: str) -> None:
        w = line.split()
        name = w[0]
        if name == "#chip":
            self.emit(0.4, "# chip: CH59x uid=0011223344556677")
        elif name == "#flash":
            self.flash_len = self.binary_left = int(w[1])
        elif name == "#erase":
            self.codes = False
            self.emit(0.3, "# erase: настройки стёрты")
        elif name == "#power":
            on = self.tm.now < self.off_at
            self.emit(0.01, "# power: вкл 3.29 В" if on else "# power: выкл 0.03 В")
        elif name == "#ir":
            self.ir_on_at = self.tm.now if w[1] == "on" else None
            self.emit(0.0, "# ir: вкл" if w[1] == "on" else "# ir: выкл")
        elif name == "#lines":
            self.emit(0.0, "# lines: отпущены")
        elif name == "#press":
            self.press(w[1], int(w[2]) / 1000)
        elif name == "#ble":
            self.ble_cmd(w[1:])
        else:
            self.emit(0.0, f"# неизвестная команда: {name[1:]}")

    def press(self, button: str, s: float) -> None:
        self.presses += 1
        self.emit(0.002, "boot: UlpanIR fw 0.3.0")
        off = s + 0.03
        if button == "ab":
            if s >= 4.0:
                self.emit(4.0, "   4000 ms: both buttons 4000 ms -> config mode")
                self.config_mode = True
                off = 1e9  # гаснет через 60 с после последней команды BLE
        elif not self.codes:
            self.emit(0.005, f"   5 ms: {button.upper()} has no code: flash, no IR")
        else:
            cmd = "0x02" if button == "a" else "0x03"
            n = self.frames_long if s >= 2.0 else int(s / (1 / 3)) + 1
            for k in range(n):
                self.frame(k / 3, cmd)
        if self.stuck_on_press is not None and self.presses >= self.stuck_on_press:
            off = 1e9
        self.off_at = self.tm.now + off
        self.emit(s, f"# press: {button} {int(s * 1000)}")

    def ble_cmd(self, w: list[str]) -> None:
        if w[0] == "connect":
            self.ble = self.config_mode
            self.emit(1.0, "# ble: подключён IR-Remote-1A2B rssi=-55" if self.ble else "# ble: не найден")
        elif w[0] == "write":
            if w[1] == "config" and w[2] == CONFIG:
                self.codes = True
            self.emit(0.1, f"# ble: записано {w[1]}")
        elif w[0] == "read":
            self.emit(0.1, f"# ble: config {CONFIG}" if self.codes else "# ble: config 0100000000000000000300")
        elif w[0] == "close":
            self.ble = False
            self.config_mode = False
            self.off_at = self.tm.now + 60.0
            self.emit(0.05, "# ble: отключён")


def scenario(tmp_path, image=b"\x01" * 1000, **kw):
    tm = FakeTime()
    fake = FakeStand(tm, **kw)
    link = stand.Link(fake, clock=tm.clock, sleep=tm.sleep)
    out: list[str] = []
    code = stand.run_checks(link, image, out=out.append, folder=tmp_path)
    return code, out, fake, tm


def test_healthy_board(tmp_path):
    """«Исправная плата» на записанных строках: все 10 проверок — да, ГОДНА, код 0, протокол сохранён."""
    code, out, _, tm = scenario(tmp_path)
    assert code == 0, "\n".join(out)
    assert out[10] == "ГОДНА"
    assert all(" да " in s for s in out[:10]), "\n".join(out)
    reports = list(tmp_path.glob("*_0011223344556677.txt"))
    assert len(reports) == 1
    text = reports[0].read_text(encoding="utf-8")
    assert "ГОДНА" in text and "-> config mode" in text and "# ir: nec 0x32 0x02" in text
    assert tm.now < 200  # с 60-секундным режимом настройки укладывается в ~2 мин


def test_fake_stand_four_frames_is_brak_9(tmp_path):
    """«Решение по поддельному стенду»: на проверке 9 приходит 4 кадра — итог БРАК: 9."""
    code, out, _, _ = scenario(tmp_path, frames_long=4)
    assert code == 1
    assert out[10] == "БРАК: 9"
    assert "кадров 4" in out[8]


def test_remote_does_not_switch_off(tmp_path):
    """«Пульт не выключается»: на проверке 4 стенд отвечает `# power: вкл 3.30 В` — «нет» с напряжением."""
    code, out, _, _ = scenario(tmp_path, stuck_on_press=1)
    assert code == 1
    assert out[3].startswith(" 4.") and " нет " in out[3]
    assert "power: вкл 3.29 В" in out[3]
    assert out[10].startswith("БРАК: 4")


def test_ir_blocked_is_brak_7_8_9(tmp_path):
    """«ИК не доходит до приёмника»: проверки 7–9 — нет, итог БРАК: 7 8 9."""
    code, out, _, _ = scenario(tmp_path, ir_blocked=True)
    assert code == 1
    assert out[10] == "БРАК: 7 8 9"


def test_without_image_only_erases(tmp_path):
    """`test`: без образа проверка 2 только стирает настройки."""
    code, out, _, _ = scenario(tmp_path, image=None)
    assert code == 0
    assert "записано" not in out[1] and "настройки стёрты" in out[1]


def test_flash_sends_header_and_image():
    tm = FakeTime()
    fake = FakeStand(tm)
    sent = []
    orig = fake.write
    fake.write = lambda d: (sent.append(d), orig(d))
    link = stand.Link(fake, clock=tm.clock, sleep=tm.sleep)
    img = bytes(range(256)) * 40
    ans = link.flash(img)
    assert sent[0] == f"#flash 10240 {stand.zlib.crc32(img):08x}\n".encode()
    assert b"".join(sent[1:]) == img
    assert ans == "# flash: записано 10240 байт за 3000 мс"


def test_chip_absent_skips_rest(tmp_path):
    tm = FakeTime()
    fake = FakeStand(tm)
    orig = fake.command
    fake.command = lambda line: fake.emit(0.5, "# chip: нет ответа") if line == "#chip" else orig(line)
    link = stand.Link(fake, clock=tm.clock, sleep=tm.sleep)
    out: list[str] = []
    code = stand.run_checks(link, b"x", out=out.append, folder=tmp_path)
    assert code == 1
    assert out[10] == "БРАК: 1 2 3 4 5 6 7 8 9 10"
    assert "не проводилась" in out[1]


def test_stand_silent_is_error_not_hang(tmp_path):
    tm = FakeTime()

    class Mute:
        def write(self, d):
            pass

        def read(self, n):
            return b""

    link = stand.Link(Mute(), clock=tm.clock, sleep=tm.sleep)
    try:
        link.call("#info", "ulpanir_stand", timeout=2)
    except stand.StandError as e:
        assert "2 с" in str(e)
    else:
        raise AssertionError("ожидалась StandError")


def test_partial_log_line_waits_for_newline():
    tm = FakeTime()

    class Chunks:
        def __init__(self):
            self.parts = [b"btn A", b"\n# x\n"]

        def write(self, d):
            pass

        def read(self, n):
            return self.parts.pop(0) if self.parts else b""

    link = stand.Link(Chunks(), clock=tm.clock, sleep=tm.sleep)
    link.pump()
    assert link.lines == []
    link.pump()
    assert [ln.text for ln in link.lines] == ["btn A", "# x"]
    assert link.lines[1].stand and not link.lines[0].stand


def test_report_name(tmp_path):
    res = stand.Result(uid="abcd")
    res.checks.append(stand.Check(1, "чип", True, "ok"))
    p = stand.save_report(res, [], when=dt.datetime(2026, 12, 1, 9, 5, 7), folder=tmp_path)
    assert p.name == "2026-12-01_090507_abcd.txt"
    assert re.search(r"^1\. чип: да — ok$", p.read_text(encoding="utf-8"), re.MULTILINE)
