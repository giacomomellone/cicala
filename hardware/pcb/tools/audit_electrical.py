"""Check Rev A circuit and land-pattern invariants that ERC cannot detect.

Run after exporting the schematic netlist. Dimensional references are listed
in ../REVIEW.md and ../cicala_rev_a/models/README.md.
"""
import sys
from pathlib import Path
from ksexp import load, child, children, ref_of


def check_duplicate_labels(sheet):
    seen = set()
    for kind in ("label", "global_label", "hierarchical_label"):
        for label in children(sheet, kind):
            position = tuple(map(float, child(label, "at")[1:3]))
            key = (kind, label[1], position)
            if key in seen:
                raise ValueError(f"Duplicate {kind} {label[1]} at {position}")
            seen.add(key)


def check_switches(board):
    footprints = {ref_of(fp): fp for fp in children(board, 'footprint')}
    for switch, resistor in (("SW1", "R21"), ("SW2", "R22")):
        properties = {p[1]: p[2] for p in children(footprints[switch], 'property')}
        if properties.get('MPN') != 'KSC323GLFG':
            raise ValueError(f'{switch}: review minimum switching current after substitution')
        properties = {p[1]: p[2] for p in children(footprints[resistor], 'property')}
        if properties.get('MPN') != 'RC0603FR-0722KL':
            raise ValueError(f'{resistor}: expected 22 kΩ 1% for gold-contact wetting current')
        pads = list(children(footprints[switch], 'pad'))
        centres = {tuple(map(float, child(p, 'at')[1:3])) for p in pads}
        if centres != {(-4.45, -2), (4.45, -2), (-4.45, 2), (4.45, 2)}:
            raise ValueError(f'{switch}: KSC3 G land centres changed')
        if any(list(map(float, child(p, 'size')[1:])) != [3.1, 1.0] for p in pads):
            raise ValueError(f'{switch}: KSC3 G land size changed')
    minimum_current_ma = 3.3 * .95 / (22000 * 1.01) * 1000
    if minimum_current_ma < .1:
        raise ValueError('Gold-contact minimum switching current is 0.1 mA')


def audit(netlist, project):
    project = Path(project)
    for path in [project / "cicala_rev_a.kicad_sch", *sorted((project / "sheets").glob("*.kicad_sch"))]:
        check_duplicate_labels(load(path))
    data = load(netlist)
    pins = {}
    for net in children(child(data, "nets"), "net"):
        for node in children(net, "node"):
            pins[(child(node, "ref")[1], child(node, "pin")[1])] = child(net, "name")[1]
    expected = {
        "U1": {"2": "3V3", "3": "CHIP_EN", "27": "BOOT_IO0",
               "13": "USB_DN", "14": "USB_DP", "37": "UART0_TX", "36": "UART0_RX",
               "39": "VBAT_SENSE", "38": "VBAT_SENSE_EN", "4": "BTN_CATEGORY",
               "5": "EPD_PWR_EN", "6": "CHG_STAT1", "7": "CHG_STAT2",
               "12": "EPD_RESET", "17": "EPD_BUSY", "18": "EPD_CS",
               "19": "EPD_MOSI", "20": "EPD_CLK", "22": "CHG_ENABLE",
               "8": "STATUS_RED", "9": "STATUS_GREEN", "10": "BTN_NEXT",
               "11": "EPD_DC", "23": "VBUS_SENSE", "31": "FACTORY_RESET_OD"},
        "U2": {"1": "VSYS", "2": "VBAT", "3": "CHG_STAT2", "4": "CHG_CE",
               "5": "GND", "6": "BATT_NTC", "9": "CHG_STAT1", "10": "USB_VBUS", "11": "GND"},
        "U3": {"1": "VSYS", "2": "GND", "3": "GND", "5": "REG_PG",
               "6": "3V3", "8": "GND", "10": "VSYS"},
        "J3": {"1": "VBAT_CELL", "2": "BATT_NTC", "3": "GND"},
        "J4": {"1": "3V3", "2": "GND", "3": "CHIP_EN", "4": "BOOT_IO0",
               "5": "UART0_TX", "6": "UART0_RX"},
        "D4": {"1": "Net-(D4-A1)", "2": "Net-(D4-A2)", "3": "GND", "4": "GND"},
        "C28": {"1": "3V3", "2": "GND"},
        "C29": {"1": "3V3", "2": "GND"},
        "C30": {"1": "3V3", "2": "GND"},
        "C14": {"2": "3V3"},
        "C34": {"1": "VBAT", "2": "GND"},
        "U7": {"3": "BATT_NTC", "6": "BATT_NTC", "4": "GND", "8": "USB_VBUS",
               "1": "/Charge temperature guard/TEMP_OK",
               "7": "/Charge temperature guard/TEMP_OK",
               "2": "/Charge temperature guard/TH_HOT_REF",
               "5": "/Charge temperature guard/TH_COLD_REF"},
        "Q2": {"1": "/Charge temperature guard/TEMP_OK", "2": "CHG_ENABLE", "3": "CHG_CE"},
        "R13": {"1": "CHG_ENABLE", "2": "GND"},
        "R35": {"1": "3V3", "2": "/Charge temperature guard/TH_COLD_REF"},
        "R36": {"1": "/Charge temperature guard/TH_COLD_REF",
                "2": "/Charge temperature guard/TH_HOT_REF"},
        "R37": {"1": "/Charge temperature guard/TH_HOT_REF", "2": "GND"},
        "R38": {"1": "USB_VBUS", "2": "/Charge temperature guard/TEMP_OK"},
        "R39": {"1": "VSYS", "2": "CHG_CE"},
        "C32": {"1": "USB_VBUS", "2": "GND"},
        "C33": {"1": "/Charge temperature guard/TEMP_OK", "2": "GND"},
    }
    for ref, mapping in expected.items():
        for pin, net in mapping.items():
            actual = pins.get((ref, pin))
            if actual != net:
                raise ValueError(f"{ref}.{pin}: expected {net}, got {actual}")
    if not (pins["U3", "4"] == pins["R14", "2"] == pins["R15", "1"] != "3V3"):
        raise ValueError("Regulator feedback divider is bypassed or disconnected")
    if pins["U6", "4"] != pins["C14", "1"]:
        raise ValueError("Panel slew capacitor does not reach CT")
    from audit_temperature import audit as audit_temperature
    audit_temperature(netlist)
    board = load(Path(project) / "cicala_rev_a.kicad_pcb")
    check_switches(board)
    for ref in ["J1", "J3", "U3", "L1", "L2", "D4"]:
        fp = next(f for f in children(board, "footprint") if ref_of(f) == ref)
        models = children(fp, "model")
        if not models:
            raise ValueError(f"{ref}: no component model")
        for model in models:
            path = Path(str(model[1]).replace("${KIPRJMOD}", str(Path(project).resolve())))
            if not path.is_file():
                raise ValueError(f"{ref}: missing model {path}")
    footprint = load(Path(project) / "cicala.pretty/LED_Kingbright_APBA2006SURKCGKC.kicad_mod")
    expected_xy = {"1": (-.975, 0), "4": (-.315, -.245), "3": (.315, -.245), "2": (.975, 0)}
    for pad in children(footprint, "pad"):
        xy = tuple(map(float, child(pad, "at")[1:3]))
        if xy != expected_xy[pad[1]]:
            raise ValueError(f"LED manufacturer terminal position changed: {pad[1]} at {xy}")
    footprint = load(Path(project) / "cicala.pretty/Texas_DLA0010A_VSON-HR-10_2x3mm_P0.5mm.kicad_mod")
    pad8 = next(p for p in children(footprint, "pad") if p[1] == "8")
    if list(map(float, child(pad8, "size")[1:])) != [1.3, .25]:
        raise ValueError("TPS63802 PGND land must follow the extended DLA0010A pad")
    print("Electrical audit: critical pin mappings, bypass ground, feedback, LED lands and local models passed")


if __name__ == "__main__":
    audit(sys.argv[1], sys.argv[2])
