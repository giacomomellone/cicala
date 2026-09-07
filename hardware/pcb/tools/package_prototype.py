"""Package the public source/engineering snapshot and verify every archived hash.

This records integrity, not new electrical or mechanical qualification. Run
the checks and update the verification record before replacing a dated handoff.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
from zipfile import ZIP_DEFLATED, ZipFile

ROOT = Path(__file__).resolve().parents[3]
EXPORTS = ROOT / 'hardware/pcb/cicala_rev_a/exports'


def sha(data):
    return hashlib.sha256(data).hexdigest()


def snapshot_files():
    tracked = subprocess.check_output(['git', 'ls-files', '-z'], cwd=ROOT)
    added = subprocess.check_output([
        'git', 'ls-files', '-z', '--others', '--exclude-standard',
        'hardware', 'firmware', 'docs/firmware_rev_a.md'], cwd=ROOT)
    names = sorted(set((tracked + added).decode().split('\0')) - {''})
    files = []
    for name in names:
        path = ROOT / name
        if (not path.is_file() or path.is_symlink()
                or path.suffix in {'.bak', '.kicad_prl', '.pyc'}
                or 'cicala_rev_a_prototype_' in path.name
                or path.name in {'source_sha256.json', '.DS_Store'}
                or any(part in {'.history', '__pycache__', 'backups'} for part in path.parts)):
            continue
        if path.suffix == '.pem' and name != 'firmware/keys/firmware-signing-dev.pem':
            continue
        files.append(path)
    return files


def verify(path, working_tree=False):
    with ZipFile(path) as archive:
        if archive.testzip() is not None:
            raise ValueError('Archive CRC check failed')
        manifest = json.loads(archive.read('handoff_sha256.json'))
        if set(archive.namelist()) != set(manifest) | {'handoff_sha256.json'}:
            raise ValueError('Archive file coverage differs from manifest')
        for name, digest in manifest.items():
            if sha(archive.read(name)) != digest:
                raise ValueError(f'Archive checksum mismatch: {name}')
            if working_tree and name != 'START_HERE.md':
                if not (ROOT / name).is_file() or sha((ROOT / name).read_bytes()) != digest:
                    raise ValueError(f'Working source/export changed after packaging: {name}')
    sidecar = path.with_suffix('.sha256')
    if sha(path.read_bytes()) != sidecar.read_text().split()[0]:
        raise ValueError('Whole-archive SHA256 mismatch')
    print(f'Verified {len(manifest)} files, CRC and SHA256: {path.name}')


def package(date):
    review = EXPORTS / 'review'
    record = json.loads((review / 'verification.json').read_text())
    if record['date'] != date or not record['checks']['digital_review_passed']:
        raise ValueError('Update the dated verification record after running the review')
    for name, digest in record['critical_source_sha256'].items():
        if sha((ROOT / name).read_bytes()) != digest:
            raise ValueError(f'Repeat validation after source change: {name}')
    for text in ('Found 0 DRC violations', 'Found 0 unconnected pads', 'Found 0 Footprint errors'):
        if text not in (review / 'drc.rpt').read_text():
            raise ValueError('DRC/parity report is not clean')
    if 'ERC messages: 0  Errors 0  Warnings 0' not in (review / 'erc.rpt').read_text():
        raise ValueError('ERC report is not clean')
    if not json.loads((review / 'usb_audit.json').read_text())['passed']:
        raise ValueError('USB geometry check failed')
    from audit_fabrication import check_ipc2581
    check_ipc2581(EXPORTS / 'cicala_rev_a_ipc2581.zip')
    files = snapshot_files()
    hashes = {str(p.relative_to(ROOT)): sha(p.read_bytes()) for p in files}
    source_manifest = review / 'source_sha256.json'
    source_manifest.write_text(json.dumps(hashes, indent=2) + '\n')
    files.append(source_manifest)
    start = (
        '# Cicala Rev A engineering prototype handoff\n\n'
        f'Public working-source snapshot dated {date}. Read these first:\n\n'
        '- hardware/pcb/MANUFACTURING.md: exact JLCPCB inputs and order settings.\n'
        '- hardware/case/README.md: printing, button-stop adjustment and assembly.\n'
        '- docs/firmware_rev_a.md: toolchain, build, flash, debug and recovery.\n'
        '- hardware/pcb/REVIEW.md: findings, evidence and prototype acceptance.\n\n'
        'Upload the inner hardware/pcb/cicala_rev_a/exports/cicala_rev_a_gerbers.zip\n'
        'to the PCB fabrication field, then the two assembly/*_jlc_*.csv files\n'
        'to the assembly fields. Do not upload this entire snapshot as Gerbers.\n\n'
        'This snapshot contains public source files without Git metadata or\n'
        'downloaded dependencies. Firmware commands assume a complete Git checkout\n'
        'of https://github.com/giacomomellone/cicala containing these revised files.\n'
        'No physical board, flash or print test is represented by the digital checks.\n'
        'Factory CAM/sourcing and the documented first-prototype measurements remain.\n'
    ).encode()
    destination = EXPORTS / f'cicala_rev_a_prototype_{date}.zip'
    manifest = {}
    with ZipFile(destination, 'w', ZIP_DEFLATED, compresslevel=9) as archive:
        for path in sorted(files):
            name = str(path.relative_to(ROOT))
            data = path.read_bytes()
            archive.write(path, name)
            manifest[name] = sha(data)
        archive.writestr('START_HERE.md', start)
        manifest['START_HERE.md'] = sha(start)
        archive.writestr('handoff_sha256.json', json.dumps(manifest, indent=2) + '\n')
    destination.with_suffix('.sha256').write_text(f'{sha(destination.read_bytes())}  {destination.name}\n')
    verify(destination, working_tree=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('date', help='YYYY-MM-DD matching the verification record')
    parser.add_argument('--check', action='store_true', help='verify archive and current working files')
    args = parser.parse_args()
    if args.check:
        verify(EXPORTS / f'cicala_rev_a_prototype_{args.date}.zip', working_tree=True)
    else:
        package(args.date)
