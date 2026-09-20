"""Verify the post-fix battery mapping reaches 100% at full charge.

Post-fix parameters (per user decision):
  BATT_ADC_FULL_SCALE_MV = 3100  (ADC_11db nominal top on ESP32-S3)
  BATT_ADC_SAMPLES       = 8     (averaged)
  BATT_ADC_SETTLE_MS     = 20
  BATT_EMPTY_MV          = 3300
  BATT_FULL_MV           = 4200  (user-confirmed)
"""

FULL_SCALE = 3100.0
DIV = 2.0
RAW_MAX = 4095.0
BATT_EMPTY = 3300
BATT_FULL = 4200


def reported_mv(vbat_v):
    v_pin_mv = vbat_v * 1000.0 / DIV
    raw = v_pin_mv / FULL_SCALE * RAW_MAX
    raw = max(0.0, min(RAW_MAX, raw))
    raw = int(raw + 0.5)
    return raw, raw * FULL_SCALE * DIV / RAW_MAX


def pct(mv):
    if mv <= BATT_EMPTY:
        return 0
    if mv >= BATT_FULL:
        return 100
    return int((mv - BATT_EMPTY) * 100 / (BATT_FULL - BATT_EMPTY))


print("Post-fix mapping (12-bit, ADC_11db, 8-sample average)")
print(f"{'Vbat':>6} {'raw':>6} {'reported_mV':>12} {'true_mV':>9} {'err%':>7} {'gauge%':>7}")
for v in (3.00, 3.30, 3.50, 3.70, 3.85, 4.00, 4.10, 4.20):
    raw, mv = reported_mv(v)
    true_mv = v * 1000
    err = (mv - true_mv) / true_mv * 100
    print(f"{v:>6.2f} {raw:>6} {mv:>12.0f} {true_mv:>9.0f} {err:>6.2f}% {pct(mv):>7}")

print()
raw, mv = reported_mv(4.20)
print(f"CHECK 4.20 V full cell -> {mv:.0f} mV -> gauge {pct(mv)}%  "
      f"{'PASS' if pct(mv) == 100 else 'FAIL'}")
raw, mv = reported_mv(4.00)
print(f"CHECK 4.00 V          -> {mv:.0f} mV -> gauge {pct(mv)}%")
raw, mv = reported_mv(3.30)
print(f"CHECK 3.30 V empty    -> {mv:.0f} mV -> gauge {pct(mv)}%")
print()
print("Note: raw never reaches 4095 at 4.2 V, so there is no saturation.")
print(f"      raw at 4.2 V = {raw if False else reported_mv(4.20)[0]} of 4095 "
      f"({reported_mv(4.20)[0]/4095*100:.0f}% of full scale)")
print()
print("Body-diode / IR-drop reserve: headroom at 4.2 V is "
      f"{4095 - reported_mv(4.20)[0]} codes before clipping.")
