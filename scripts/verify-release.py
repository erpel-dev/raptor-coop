#!/usr/bin/env python3
"""Verify all desktop packages and assemble a canonical release checksum file."""

import argparse
import hashlib
from pathlib import Path
import shutil
import sys


def verify(directory, version):
    checksums = sorted(directory.rglob('*.sha256'))
    if len(checksums) != 3:
        files = ', '.join(str(p.relative_to(directory)) for p in directory.rglob('*') if p.is_file())
        raise ValueError(f'Expected three checksum files; found {len(checksums)}. Files: {files}')
    entries = []
    platforms = set()
    for checksum in checksums:
        digest, name = checksum.read_text().strip().split()
        if Path(name).name != name or checksum.name != name + '.sha256':
            raise ValueError(f'Invalid checksum filename: {checksum}')
        platform = next((p for p in ('linux', 'windows', 'macos')
                         if name.startswith(f'raptor-coop-{version}-{p}-')), None)
        if platform is None or platform in platforms:
            raise ValueError(f'Unexpected or duplicate platform archive: {name}')
        archive = checksum.parent / name
        actual = hashlib.sha256(archive.read_bytes()).hexdigest()
        if actual != digest:
            raise ValueError(f'Checksum mismatch for {name}: expected {digest}, got {actual}')
        platforms.add(platform)
        if archive.parent != directory:
            shutil.copy2(archive, directory / name)
        entries.append(f'{actual}  {name}\n')
        print(f'Verified {name}', flush=True)
    (directory / 'SHA256SUMS').write_text(''.join(sorted(entries)), encoding='utf-8')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path)
    parser.add_argument('--version', required=True)
    args = parser.parse_args()
    try:
        verify(args.directory.resolve(), args.version)
    except (ValueError, OSError) as error:
        print(f'::error::{error}', file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
