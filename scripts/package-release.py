#!/usr/bin/env python3
"""Package a native desktop build without proprietary game assets."""

import argparse
import hashlib
from pathlib import Path
import platform
import re
import shutil
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[1]


def dependencies(binary):
    output = subprocess.check_output(['otool', '-L', str(binary)], text=True)
    return [line.strip().split(' (', 1)[0] for line in output.splitlines()[1:]]


def bundle_macos(binary):
    """Relocate Homebrew libraries beside the executable, including dependencies."""
    queue = [binary]
    bundled = {}
    sdl_prefix = Path(subprocess.check_output(['brew', '--prefix', 'sdl2'], text=True).strip())

    def resolve(dependency, current):
        if dependency.startswith('@loader_path/'):
            return current.parent / dependency[len('@loader_path/'):]
        if dependency.startswith('@executable_path/'):
            return binary.parent / dependency[len('@executable_path/'):]
        if dependency.startswith('@rpath/'):
            name = dependency[len('@rpath/'):]
            output = subprocess.check_output(['otool', '-l', str(current)], text=True)
            rpaths = re.findall(r'cmd LC_RPATH\s+cmdsize \d+\s+path (.*?) \(offset', output)
            for directory in [binary.parent, sdl_prefix / 'lib',
                              *(str(resolve(path, current)) for path in rpaths)]:
                candidate = Path(directory) / name
                if candidate.is_file():
                    return candidate
            raise RuntimeError(f'Cannot resolve {dependency} in {current}')
        return Path(dependency)

    while queue:
        current = queue.pop(0)
        for dependency in dependencies(current):
            if dependency.startswith(('/System/', '/usr/lib/')):
                continue
            source = resolve(dependency, current)
            # Already relocated libraries (including a dylib's own install ID).
            if source.parent == binary.parent and source.exists():
                if dependency.startswith('@rpath/'):
                    subprocess.run(['install_name_tool', '-change', dependency,
                                    '@loader_path/' + source.name, str(current)], check=True)
                continue
            destination = binary.parent / source.name
            resolved = source.resolve(strict=True)
            if source.name in bundled and bundled[source.name] != resolved:
                raise RuntimeError(f'Conflicting library names: {dependency}')
            if source.name not in bundled:
                bundled[source.name] = resolved
                shutil.copy2(resolved, destination)
                destination.chmod(0o755)
                subprocess.run(['install_name_tool', '-id', '@loader_path/' + source.name,
                                str(destination)], check=True)
                queue.append(destination)
            subprocess.run(['install_name_tool', '-change', dependency,
                            '@loader_path/' + source.name, str(current)], check=True)
    # Re-sign after relocation; signatures are ad-hoc, not Apple notarization.
    for library in binary.parent.glob('*.dylib'):
        subprocess.run(['codesign', '--force', '--sign', '-', str(library)], check=True)
    subprocess.run(['codesign', '--force', '--sign', '-', str(binary)], check=True)
    for current in [binary, *binary.parent.glob('*.dylib')]:
        for dependency in dependencies(current):
            if dependency.startswith('/') and not dependency.startswith(('/System/', '/usr/lib/')):
                raise RuntimeError(f'Unbundled library: {dependency}')
            if dependency.startswith('@') and not dependency.startswith('@loader_path/'):
                raise RuntimeError(f'Unrelocated library: {dependency}')
            if dependency.startswith('@loader_path/') and not resolve(dependency, current).is_file():
                raise RuntimeError(f'Missing library: {dependency}')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, required=True)
    parser.add_argument('--output-dir', type=Path, required=True)
    args = parser.parse_args()
    version = (ROOT / 'VERSION').read_text().strip()
    if not re.fullmatch(r'\d+\.\d+\.\d+(?:-[a-zA-Z0-9.]+)?', version):
        parser.error('Invalid VERSION')
    if f'Raptor {version}"' not in (ROOT / 'src/rapver.h').read_text():
        parser.error('VERSION does not match src/rapver.h')
    system = platform.system()
    if system not in ('Linux', 'Windows', 'Darwin'):
        parser.error('Only desktop platforms are supported')
    architecture = platform.machine().lower()
    architecture = {'amd64': 'x86_64', 'aarch64': 'arm64'}.get(architecture, architecture)
    name = f'raptor-coop-{version}-{dict(Linux="linux", Windows="windows", Darwin="macos")[system]}-{architecture}'
    binary_name = 'raptor.exe' if system == 'Windows' else 'raptor'
    candidates = [args.build_dir / 'bin' / 'Release' / binary_name,
                  args.build_dir / 'bin' / binary_name]
    binary = next((path for path in candidates if path.is_file()), None)
    if binary is None:
        parser.error(f'No built {binary_name} found in {args.build_dir}')
    args.output_dir.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory() as temporary:
        package = Path(temporary) / name
        package.mkdir()
        executable = package / binary_name
        shutil.copy2(binary, executable)
        if system != 'Windows':
            executable.chmod(0o755)
        for filename in ('README.md', 'LICENSE', 'VERSION', 'SETUP(ADLIB).INI', 'SETUP(MIDI).INI'):
            shutil.copy2(ROOT / filename, package / filename)
        licenses = package / 'licenses'
        licenses.mkdir()
        for source, filename in [('include/TinySoundFont/LICENSE', 'TinySoundFont.txt'),
                                 ('include/miniz/LICENSE', 'miniz.txt'),
                                 ('include/SDL2-devel-2.28.2-VC/SDL2-2.28.2/COPYING.txt', 'SDL2.txt')]:
            shutil.copy2(ROOT / source, licenses / filename)
        if system == 'Windows':
            shutil.copy2(ROOT / 'include/SDL2-devel-2.28.2-VC/SDL2-2.28.2/lib/x64/SDL2.dll', package)
        if system == 'Darwin':
            bundle_macos(executable)
        (package / 'START-HERE.txt').write_text(
            f'Raptor Co-op {version}\n\n'
            + ('Install runtime dependencies: libsdl2-2.0-0, libasound2 (or libasound2t64), and curl.\n'
               if system == 'Linux' else '')
            + f'Extract the entire archive, then run {binary_name} from this directory.\n'
            + ('macOS builds are ad-hoc signed and are not notarized.\n' if system == 'Darwin' else '')
            + 'Original GLB assets are not included. See README.md for asset setup and co-op controls.\n'
            f'Source: https://github.com/erpel-dev/raptor-coop/tree/v{version}\n',
            encoding='utf-8')
        archive = Path(shutil.make_archive(str(args.output_dir.resolve() / name),
                                           'zip' if system == 'Windows' else 'gztar',
                                           temporary, name))
    checksum = hashlib.sha256(archive.read_bytes()).hexdigest()
    archive.with_name(archive.name + '.sha256').write_text(f'{checksum}  {archive.name}\n')
    print(archive)


if __name__ == '__main__':
    main()
