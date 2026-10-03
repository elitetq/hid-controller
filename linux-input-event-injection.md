# Faking button presses in the Linux kernel (diagnostic notes)

*2026-09-11 — while debugging why `hardwaretester.com/gamepad` showed nothing on Linux.*

## The problem this solved

The board enumerated perfectly on Linux — `lsusb -v` returned the full report
descriptor, `dmesg` showed it binding, `/dev/input/js0` and `/dev/input/event24`
both existed with correct ACLs — but **no browser would display the gamepad**.
Firefox and Chromium both showed nothing. Firefox on Windows showed it.

Everything in the Linux stack checked out, so the remaining suspect was the
Gamepad API's interaction requirement: a browser will not expose a pad via
`navigator.getGamepads()` until the user interacts with it.

The firmware can't test that. `gamepad_update_buttons()` in
`hid-controller-code/hid-controller-code/Core/Src/main.c` zeroes `buttons`, sets
hat/triggers to neutral, and `return`s before polling a single GPIO. A 5-second
capture confirmed it: **zero `EV_KEY` events, ever.** Only `ABS_RY` moved
(ADC noise).

So rather than reflash to test a theory, I synthesized a button press on the real
device from inside the kernel. It worked — the pad appeared immediately.
Diagnosis confirmed: the browsers were waiting on a signal the firmware can
never send.

**This is a diagnostic, not a fix.** Nothing is persisted; unplug and it's gone.

## How a USB gamepad becomes browser input

```
USB wire
  └─ usbhid            binds the interface (bInterfaceClass = 3)
      └─ hid-core      parses the HID report descriptor
          └─ hid-input maps HID usages → Linux input codes
                       Usage(X) 0x30 → ABS_X ; Button 1 → BTN_A
              └─ input_dev          ← injection happens HERE
                  ├─ evdev  → /dev/input/eventN   (modern, full fidelity)
                  └─ joydev → /dev/input/jsN      (legacy joystick API)
                      └─ udev sets ID_INPUT_JOYSTICK=1 + ACLs
                          └─ browser opens the node and polls
```

Both device nodes are views onto the **same** `input_dev`. That fact is what makes
the diagnostic verifiable — see "Why two operations at once" below.

## Why writing to an evdev node works

`/dev/input/event*` nodes are bidirectional:

- **reading** gives you events the device sent
- **writing** calls `evdev_write()` in `drivers/input/evdev.c`, which calls
  `input_inject_event()` — the kernel pushes the event into the input core as
  though the hardware had produced it

Every consumer downstream (joydev, other evdev clients, the browser) sees a real
button press on the real controller. The event never existed in the USB traffic;
it was created downstream of USB entirely.

Injection **must** go through evdev — `joydev_fops` has no `.write` handler, so
`jsN` is read-only.

Write access comes from the udev `uaccess` ACL (`crw-rw----+`), which grants the
active seat user `rw`. No root needed.

## The two struct layouts

### `struct input_event` — evdev, 24 bytes on x86-64 (what we write)

```c
struct input_event {
    struct timeval time;  /* 16 bytes; zeros => kernel timestamps it */
    __u16 type;           /* EV_SYN=0, EV_KEY=1, EV_ABS=3 */
    __u16 code;           /* BTN_A=0x130, ABS_X=0 */
    __s32 value;          /* 1=press, 0=release */
};
```

```python
struct.pack('<QQHHi', 0, 0, type, code, value)
```

The `<` is essential — it forces little-endian with **no padding**, matching the
kernel layout exactly. Python's native mode (`@`) would insert alignment padding
and the kernel would reject or misread the write.

**`EV_SYN`/`SYN_REPORT` after each event is mandatory.** It is the frame
boundary. Input events arrive in batches and consumers only act on a completed
frame. Omit it and the press sits in a buffer and is never delivered.

### `struct js_event` — joydev, 8 bytes (what we read)

```c
struct js_event {
    __u32 time;    /* milliseconds */
    __s16 value;   /* axes: -32767..32767, buttons: 0/1 */
    __u8  type;    /* 0x01=BUTTON, 0x02=AXIS, |0x80=INIT */
    __u8  number;  /* index */
};
```

```python
time, value, type, number = struct.unpack('<IhBB', data)
```

## Why two operations at once

The script writes to `event24` **and** simultaneously reads `js0`. That is one
action plus one independent measurement, not two changes.

Writing to an evdev node is obscure enough not to assume it works. Since `js0`
and `event24` are two different handlers on the *same* `input_dev`, seeing
`BUTTON num=0 val=1` come out of `js0` proves the event actually traversed the
input core — rather than landing in a file descriptor and vanishing. Without
that, a negative browser result would be ambiguous: no button, or injection
silently failing?

## The joydev quirk that wasted an hour

`cat /dev/input/js0` returned **0 bytes**. `dd if=/dev/input/js0 bs=8` returned
**144**. The reason is in `joydev_read()`:

```c
if (count == sizeof(struct js_event) &&
    client->startup < joydev->nabs + joydev->nkey)
        return joydev_0x_read(client, joydev->handle.dev, buf);
```

On open, joydev synthesizes a complete state snapshot — the `0x80` INIT events —
so a client knows where every axis and button currently sits. But it only emits
them when you read **exactly 8 bytes at a time**. `cat` uses a 128 KB buffer, so
`count != 8` and it falls through to the normal blocking path, which returns
nothing on an idle device.

144 bytes = 18 events × 8 = 10 buttons + 8 axes. Always read `jsN` with `bs=8`.

That snapshot is also what proved the board was transmitting: `ABS_Z` and
`ABS_RZ` read raw **128**. The kernel's initial cached value for an axis is 0,
which scales to −32767. Only a real report could have put 128 there — the
firmware's `lt = 128; rt = 128;`.

## Reading the diagnostics

`/proc/bus/input/devices` prints capability bitmasks as longs, most-significant
first:

```
I: Bus=0003 Vendor=1209 Product=028e Version=0111
N: Name="Jonart B J-Controller"
H: Handlers=event24 js0
B: EV=1b
B: KEY=3ff000000000000 0 0 0 0
B: ABS=3003f
```

- `EV=1b` → bits 0,1,3,4 = `EV_SYN`, `EV_KEY`, `EV_ABS`, `EV_MSC`
- `ABS=3003f` → `0x3f` = bits 0–5 (`ABS_X`..`ABS_RZ`); `0x30000` = bits 16,17
  (`ABS_HAT0X`/`ABS_HAT0Y`)
- `KEY=3ff000000000000` → set bits are 304–313 = `BTN_A`..`BTN_TR2`, i.e. exactly
  10 buttons

That is precisely what the report descriptor specifies, which is how we knew the
descriptor, driver binding, and HID→input mapping were all correct.

joydev scales the driver's `absinfo` range (0–255 here, from the descriptor's
Logical Min/Max) to −32767..32767. So raw 0 → −32767, raw 128 → ~0,
raw 255 → +32767.

## The scripts

### `inject.py` — one press, with verification

```python
import os, struct, threading, time, select, sys
EV24, JS = '/dev/input/event24', '/dev/input/js0'
EV_SYN, EV_KEY, BTN_A = 0x00, 0x01, 0x130

cap = []
def capture():
    f = os.open(JS, os.O_RDONLY | os.O_NONBLOCK)
    t0 = time.time()
    while time.time() - t0 < 3:
        r, _, _ = select.select([f], [], [], 0.2)
        if r:
            try: d = os.read(f, 8)      # exactly 8 => joydev INIT events
            except BlockingIOError: continue
            if d: cap.append(d)
    os.close(f)

th = threading.Thread(target=capture); th.start(); time.sleep(0.6)

ev = lambda t, c, v: struct.pack('<QQHHi', 0, 0, t, c, v)
try:
    fd = os.open(EV24, os.O_WRONLY)
    os.write(fd, ev(EV_KEY, BTN_A, 1) + ev(EV_SYN, 0, 0)); time.sleep(0.15)
    os.write(fd, ev(EV_KEY, BTN_A, 0) + ev(EV_SYN, 0, 0)); os.close(fd)
    print(">>> injected BTN_A press+release into event24 OK")
except Exception as e:
    print(">>> INJECT FAILED:", type(e).__name__, e); sys.exit(1)

th.join()
live = [d for d in cap if not (struct.unpack('<IhBB', d)[2] & 0x80)]
print(f">>> js0: {len(cap)} events total, {len(live)} live (non-init)")
for d in live:
    tm, val, typ, num = struct.unpack('<IhBB', d)
    print(f"    {'BUTTON' if typ==1 else 'AXIS'} num={num} val={val}")
```

`O_NONBLOCK` + `select()` keeps the capture thread from hanging forever on a
quiet device.

### `wiggle.py` — repeated presses, for testing in a browser

Auto-discovers the node, because `event24` is not stable across replugs.

```python
#!/usr/bin/env python3
"""Inject repeated BTN_A press/release into the real J-Controller evdev node."""
import os, struct, time, glob, sys

node = None
for d in glob.glob('/sys/class/input/event*'):
    try:
        if 'J-Controller' in open(os.path.join(d, 'device/name')).read():
            node = '/dev/input/' + os.path.basename(d); break
    except OSError:
        pass
if not node:
    sys.exit("J-Controller evdev node not found - is it plugged in?")

EV_SYN, EV_KEY, BTN_A = 0x00, 0x01, 0x130
ev = lambda t, c, v: struct.pack('<QQHHi', 0, 0, t, c, v)
fd = os.open(node, os.O_WRONLY)
print(f"Injecting BTN_A on {node} for 25s - switch to your browser tab NOW.")
end = time.time() + 25
n = 0
while time.time() < end:
    os.write(fd, ev(EV_KEY, BTN_A, 1) + ev(EV_SYN, 0, 0)); time.sleep(0.12)
    os.write(fd, ev(EV_KEY, BTN_A, 0) + ev(EV_SYN, 0, 0)); time.sleep(0.45)
    n += 1
    print(f"  press {n}", flush=True)
os.close(fd)
print("done")
```

Note: an injected `BTN_A=1` is overwritten within ~1 ms by the next HID report
(which says 0), because `input_event()` only emits on a *change*. The press and
release still both get delivered, which is all the browser needs.

## Useful one-liners

```bash
# does the kernel see the device, and how is it mapped?
grep -A9 J-Controller /proc/bus/input/devices

# joydev state snapshot (MUST be bs=8)
dd if=/dev/input/js0 bs=8 count=18 2>/dev/null | xxd

# is anything actually arriving right now?
timeout 5 cat /dev/input/event24 > /tmp/ev.bin; ls -l /tmp/ev.bin

# did udev classify it as a joystick, and who may read it?
udevadm info /dev/input/js0 | grep -E "ID_INPUT|TAGS"

# which process has the device open? (browsers hold it while polling)
for p in /proc/[0-9]*; do
  ls -l $p/fd 2>/dev/null | grep -q event24 && echo "$p $(cat $p/comm)"
done
```

## The actual fix

Injection only proves the cause. The real fix is in the firmware:

1. **Poll the buttons.** Remove the early `return;` in
   `gamepad_update_buttons()` (`Core/Src/main.c`) and read the GPIOs.
2. **`MX_GPIO_Init` is missing pins.** Per the PCB netlist, `PA7`/`BTN_2`,
   `PB0`/`BTN_1`, `PB1`/`BTN_3`, `PB10`/`BTN_4`, `PB11`/`BTN_STR`,
   `PB13`/`BTN_RB`, `PA10`/`DPAD_1` are never configured. `PB4` is configured
   but unconnected on the board. Unconfigured F103 pins reset to floating input,
   so they read noise.
3. **`PB8`/`AN_SEL1` is unconfigured**, leaving the analog mux in an undefined
   state — which is why `ABS_X`/`ABS_Y`/`ABS_RX` read raw 0 and `ABS_RY` drifts
   (observed at ~197, then ~254, oscillating on its own).
4. **Triggers must rest at 0, not 128** (`main.c`), or `ABS_Z`/`ABS_RZ` read
   permanently half-pressed. `BTN_LT` (PB5) and `BTN_RT` (PB15) are digital on
   this board, so drive those axes 0 or 255.
