"""Convert KiCad BOM/positions to JLCPCB headers and check reference coverage."""
import csv
import math
from pathlib import Path
import re
import sys


def references(value):
    result = []
    for token in filter(None, re.split(r'[,;\s]+', value)):
        match = re.fullmatch(r'([A-Za-z]+)(\d+)-([A-Za-z]+)(\d+)', token)
        if match:
            prefix, start, other, end = match.groups()
            if prefix != other or int(end) < int(start):
                raise ValueError(f'Invalid reference range: {token}')
            result.extend(f'{prefix}{i}' for i in range(int(start), int(end) + 1))
        else:
            result.append(token)
    if len(set(result)) != len(result):
        raise ValueError(f'Duplicate references: {value}')
    return result


def convert(bom_path, position_path, directory, prefix='cicala_rev_a'):
    with open(bom_path, newline='') as stream:
        bom = list(csv.DictReader(stream))
    with open(position_path, newline='') as stream:
        positions = list(csv.DictReader(stream))
    expected = set()
    for row in bom:
        expanded = references(row['Refs'])
        row['Refs'] = ','.join(expanded)
        refs = set(expanded)
        if expected & refs or len(refs) != int(row['Qty']):
            raise ValueError(f'Duplicate or incorrect BOM quantity: {row["Refs"]}')
        expected |= refs
    actual = {row['Ref'] for row in positions}
    if actual != expected or len(actual) != len(positions):
        raise ValueError(f'Assembly BOM/CPL mismatch: {actual ^ expected}')
    for row in positions:
        if row['Side'] not in {'top', 'bottom'}:
            raise ValueError(f'Unknown assembly side: {row}')
        if not all(math.isfinite(float(row[k])) for k in ['PosX', 'PosY', 'Rot']):
            raise ValueError(f'Invalid placement: {row}')
    directory = Path(directory)
    with (directory / f'{prefix}_jlc_bom.csv').open('w', newline='') as stream:
        writer = csv.writer(stream, lineterminator='\n')
        writer.writerow(['Comment', 'Designator', 'Footprint', 'LCSC Part #',
                         'Manufacturer', 'MPN', 'Quantity'])
        for row in bom:
            writer.writerow([row['Value'], row['Refs'], row['Footprint'],
                             row.get('LCSC', ''), row['Manufacturer'], row['MPN'], row['Qty']])
    with (directory / f'{prefix}_jlc_cpl.csv').open('w', newline='') as stream:
        writer = csv.writer(stream, lineterminator='\n')
        writer.writerow(['Designator', 'Mid X', 'Mid Y', 'Rotation', 'Layer'])
        for row in positions:
            writer.writerow([row['Ref'], f'{float(row["PosX"]):.6f}',
                             f'{float(row["PosY"]):.6f}',
                             f'{float(row["Rot"]) % 360:.6f}', row['Side']])
    print(f'JLCPCB BOM and CPL cover exactly {len(actual)} fitted components')


if __name__ == '__main__':
    convert(*sys.argv[1:])
