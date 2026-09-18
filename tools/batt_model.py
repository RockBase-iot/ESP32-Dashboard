"""Battery-ADC math model for NM-EPD-420 / NM-Display-420.

Purpose: prove or disprove the claim that the voltage->percentage mapping in
dashboardApp.cpp can never reach 100%.

Pin chain:  VBAT --[ 1:1 divider ]--> PIN_BATT_ADC(IO3) --> ADC1_CH2
Enable:     PIN_ADC_EN(IO43) HIGH during the read only.

Firmware formula under review (nm_epd_420_4c/Board.cpp:111):
    return raw * 3300UL * BATT_ADC_DIV / 4095UL;      // BATT_ADC_DIV = 2

Percentage mapping under review (dashboardApp.cpp:426):
    <=3300 mV -> 0%      >=4200 mV -> 100%      else linear over 900 mV
"""

DIV = 2.0              # BATT_ADC_DIV: battery mV = ADC pin mV * 2
RAW_MAX = 4095.0       # 12-bit analogRead


def adc_reported_mv(vbat_v, fs_mv):
    """Replicate firmware: physical vbat -> ADC pin mV -> raw counts -> reported mV.

    vbat_v is in VOLTS; fs_mv is the ADC full-scale input in MILLIVOLTS.
    """
    v_pin_mv = vbat_v * 1000.0 / DIV
    raw = v_pin_mv / fs_mv * RAW_MAX
    raw = max(0.0, min(RAW_MAX, raw))
    raw = int(raw + 0.5)                     # analogRead() returns an integer
    reported = raw * 3300.0 * DIV / RAW_MAX  # the formula in our Board.cpp
    return raw, reported


def our_pct(mv):
    if mv <= 3300:
        return 0
    if mv >= 4200:
        return 100
    return int((mv - 3300) * 100 / 900)


def official_pct(mv):
    """Official config.h spec window: BATT_MIN_MV 2500 .. BATT_MAX_MV 4500."""
    lo, hi = 2500.0, 4500.0
    if mv <= lo:
        return 0
    if mv >= hi:
        return 100
    return int((mv - lo) * 100 / (hi - lo))


def report(title, fs_mv):
    print(f"\n=== {title}  (ADC full-scale assumed {fs_mv:.0f} mV) ===")
    print(f"{'Vbat':>6} {'raw':>6} {'reported_mV':>12} {'our%':>5} {'official%':>10}")
    for v in (3.00, 3.30, 3.50, 3.70, 3.85, 4.00, 4.10, 4.20):
        raw, mv = adc_reported_mv(v, fs_mv)
        print(f"{v:>6.2f} {raw:>6} {mv:>12.0f} {our_pct(mv):>5} {official_pct(mv):>10}")


if __name__ == "__main__":
    # ESP32-S3 nominal full-scale input voltages per Espressif ADC docs.
    report("S3 @ ADC_11db (~3100 mV FS)", 3100.0)
    report("S3 @ ADC_6db  (~1750 mV FS)", 1750.0)

    print("\n=== Verdict ===")
    # With the correct 11 dB attenuation a healthy 4.20 V cell reports ~4200 mV.
    _, mv_full = adc_reported_mv(4.20, 3100.0)
    print(f"Full cell (4.20 V) -> reported {mv_full:.0f} mV -> our {our_pct(mv_full)}%")
    # Charger float / CV taper typically holds the cell slightly below 4.2 V.
    _, mv_float = adc_reported_mv(4.15, 3100.0)
    print(f"Post-charge rest (4.15 V) -> reported {mv_float:.0f} mV -> our {our_pct(mv_float)}%")
    # Naive 3.7 V "nominal" cell that is only ever charged to a 4.2 V CC/CV cut-off.
    for v in (4.10, 4.12, 4.15, 4.18, 4.20):
        _, mv = adc_reported_mv(v, 3100.0)
        print(f"  {v:.2f} V -> {mv:.0f} mV -> {our_pct(mv)}%")
