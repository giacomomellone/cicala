"""Compare the assembled-board datums and compiled firmware to the Rev A contract."""
import argparse
import ast
import csv
import json
from pathlib import Path
import re
import struct
import sys

import ksexp

ROOT = Path(__file__).resolve().parents[3]


def number(source, name):
    match = re.search(r'^' + re.escape(name) + r'\s*=\s*([\d.]+);', source, re.M)
    if not match:
        raise ValueError(f'Expected explicit SCAD dimension {name}')
    return float(match[1])


def mechanical(board, geometry, scad):
    fps = {ksexp.ref_of(fp): fp for fp in ksexp.children(board, 'footprint')}
    for ref, x, y, angle, face in (
        ('SW1', number(scad, 'category_x'), number(scad, 'category_y'), 0, 'F.Cu'),
        ('SW2', number(scad, 'next_x'), number(scad, 'next_y'), 0, 'F.Cu'),
        ('J2', number(scad, 'fpc_x'), number(scad, 'fpc_y'), 90, 'F.Cu'),
        ('J1', number(scad, 'usb_x'), 50.1, 180, 'B.Cu'),
        ('D4', number(scad, 'light_pipe_x'), 51.3, 180, 'B.Cu'),
        ('J3', 73.6, 40, 90, 'B.Cu'),
        ('J4', 39, 20, 0, 'B.Cu'),
        ('U1', 16.25, 23, -90, 'B.Cu'),
    ):
        at = list(map(float, ksexp.child(fps[ref], 'at')[1:]))
        if len(at) == 2:
            at.append(0)
        if (abs(at[0] - x) + abs(at[1] - y) > 1e-5
                or abs((at[2] - angle + 180) % 360 - 180) > 1e-5
                or ksexp.child(fps[ref], 'layer')[1] != face):
            raise ValueError(f'{ref}: case datum disagrees with PCB placement')
    mount = re.search(r'^mount_points\s*=\s*(\[.*?\]);', scad, re.M | re.S)
    if mount is None:
        raise ValueError('Missing case mounting-hole array')
    for i, expected in enumerate(ast.literal_eval(mount[1]), 1):
        actual = list(map(float, ksexp.child(fps[f'H{i}'], 'at')[1:3]))
        if actual != expected:
            raise ValueError(f'H{i}: case mounting hole disagrees with PCB')
    outline = geometry['outline']
    x0, y0 = min(p[0] for p in outline), min(p[1] for p in outline)
    x1, y1 = max(p[0] for p in outline), max(p[1] for p in outline)
    dimensions = (x0, y0, x1 - x0, y1 - y0,
                  float(ksexp.child(ksexp.child(board, 'general'), 'thickness')[1]))
    expected = tuple(number(scad, n) for n in
                     ('pcb_x', 'pcb_y', 'pcb_width', 'pcb_depth', 'pcb_thickness'))
    if any(abs(a - b) > 1e-5 for a, b in zip(dimensions, expected)):
        raise ValueError(f'PCB outline/thickness disagrees with enclosure: {dimensions}')
    return {'pcb_datums_checked': 12, 'pcb_dimensions_mm': dimensions}


def firmware(build, contract, boot=False):
    sys.path.insert(0, str(ROOT / 'deps/zephyr/scripts/dts/python-devicetree/src'))
    from devicetree import dtlib
    tree = dtlib.DT(str(build / 'zephyr.dts'))
    config = dict(re.findall(r'^(CONFIG_\w+)=(.+)$', (build / '.config').read_text(), re.M))

    def require(actual, expected, label):
        if actual != expected:
            raise ValueError(f'{build}: {label}: expected {expected}, got {actual}')

    require(tree.label2node['flash0'].props['reg'].to_nums(), [0, 16 * 1024 * 1024], 'flash')
    require(tree.label2node['psram0'].props['status'].to_string(), 'disabled', 'PSRAM node')
    require(config.get('CONFIG_ESP_SPIRAM', 'n'), 'n', 'PSRAM driver')
    require(config['CONFIG_FLASH_SIZE'], '16777216', 'configured flash bytes')
    if boot:
        return {'flash_bytes': 16777216, 'psram': False}

    pins = {r['net_name']: r['gpio'] for r in csv.DictReader(contract.open())}
    user = tree.get_node('/zephyr,user')
    checked = {}

    def gpio(node, prop, nets):
        cells = struct.unpack('>' + 'I' * (len(node.props[prop].value) // 4), node.props[prop].value)
        require(len(cells), len(nets) * 3, prop + ' cells')
        for i, net in enumerate(nets):
            controller, pin, flags = cells[i * 3:i * 3 + 3]
            require(tree.phandle2node[controller], tree.label2node['gpio0'], net + ' port')
            require(pin, int(pins[net]), net + ' pin')
            expected_flags = (17 if net.startswith('BTN_') else
                              1 if net in ('EPD_CS', 'EPD_RESET') else 0)
            require(flags, expected_flags, net + ' polarity/pull-up')
            checked[net] = {'gpio': pin, 'flags': flags}

    for prop, nets in {
        'cicala-vbus-gpios': ['VBUS_SENSE'],
        'cicala-battery-enable-gpios': ['VBAT_SENSE_EN'],
        'cicala-panel-power-gpios': ['EPD_PWR_EN'],
        'cicala-panel-mosi-gpios': ['EPD_MOSI'],
        'cicala-panel-clock-gpios': ['EPD_CLK'],
        'cicala-charge-disable-gpios': ['CHG_ENABLE'],
        'cicala-charger-status-gpios': ['CHG_STAT1', 'CHG_STAT2'],
    }.items():
        gpio(user, prop, nets)
    for label, net in [('cicala_category', 'BTN_CATEGORY'), ('cicala_next', 'BTN_NEXT'),
                       ('led_red', 'STATUS_RED'), ('led_green', 'STATUS_GREEN')]:
        gpio(tree.label2node[label], 'gpios', [net])
    gpio(tree.label2node['spi2'], 'cs-gpios', ['EPD_CS'])
    gpio(tree.label2node['cicala_mipi_dbi'], 'dc-gpios', ['EPD_DC'])
    gpio(tree.label2node['cicala_mipi_dbi'], 'reset-gpios', ['EPD_RESET'])
    panel = tree.label2node['cicala_epaper']
    pinmux = [cell for group in tree.label2node['spim2_default'].nodes.values()
              for cell in group.props['pinmux'].to_nums()]
    require(sorted(pinmux), sorted([0x32ffcc, 0x33ffcb]), 'SPI2 clock GPIO12 / MOSI GPIO11')
    gpio(panel, 'busy-gpios', ['EPD_BUSY'])
    require(panel.props['rotation'].to_num(), 180, 'panel installation rotation')
    require(panel.props['width'].to_num(), 250, 'panel width')
    require(panel.props['height'].to_num(), 122, 'panel height')
    adc = struct.unpack('>II', user.props['io-channels'].value)
    require(tree.phandle2node[adc[0]], tree.label2node['adc0'], 'battery ADC1')
    require(adc[1], 0, 'battery ADC channel (GPIO1)')
    require(pins['VBAT_SENSE'], '1', 'ADC pin contract')
    require(tree.get_node('/chosen').props['zephyr,console'].to_path(),
            tree.label2node['usb_serial'], 'native USB console')
    require(tree.label2node['usb_otg'].props['status'].to_string(), 'disabled', 'USB OTG conflict')
    require('ws2812-gpios' in user.props, False, 'DevKit RGB LED removed')
    for key, value in {
        'CONFIG_CICALA_REV_A': 'y', 'CONFIG_CICALA_POWER_DIVIDER_NUM': '147',
        'CONFIG_CICALA_POWER_DIVIDER_DEN': '47',
        'CONFIG_CICALA_POWER_DIVIDER_SETTLE_MS': '200',
        'CONFIG_CICALA_POWER_SAMPLE_MS': '1000',
    }.items():
        require(config.get(key), value, key)
    if config.get('CONFIG_CICALA_SLEEP') == 'y':
        require(config.get('CONFIG_CICALA_PANEL_DEEP_SLEEP'), 'y', 'panel sleep before rail off')
    return {'flash_bytes': 16777216, 'psram': False, 'gpio_signals': checked,
            'display_rotation': 180, 'adc_divider': [147, 47]}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('geometry', type=Path)
    parser.add_argument('--app', type=Path)
    parser.add_argument('--boot', type=Path)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    project = ROOT / 'hardware/pcb/cicala_rev_a'
    result = mechanical(ksexp.load(project / 'cicala_rev_a.kicad_pcb'),
                        json.loads(args.geometry.read_text()),
                        (ROOT / 'hardware/case/cicala_enclosure.scad').read_text())
    for name, path in [('app', args.app), ('boot', args.boot)]:
        if path:
            result[name] = firmware(path, project / 'pin_contract.csv', boot=name == 'boot')
    if args.output:
        args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))
