#!/usr/bin/env python3
"""Import/check the pinned acados C runtime. Normal builds need no Python/Git.

Read committed Git objects, never the upstream working tree. Evaluate upstream
CMake source lists for our supported backends and retain headers plus textual
source includes. No upstream examples, tests, docs or language interfaces ship.
"""

import argparse
import hashlib
import io
import json
import re
import subprocess
import tarfile
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DESTINATION = ROOT / '3rdparty/acados_runtime'
MAX_BYTES = 15_000_000
UPSTREAM = {
    'acados': {
        'url': 'https://github.com/acados/acados',
        'commit': '4c23274e49e1304cf3c859d59ea6694ce36305a7',
        'prefix': '', 'checkout': '.', 'license': 'LICENSE',
    },
    'blasfeo': {
        'url': 'https://github.com/giaf/blasfeo',
        'commit': 'd6251233923c9b475fe894fb729fb63ab693e301',
        'prefix': 'external/blasfeo', 'checkout': 'external/blasfeo', 'license': 'LICENSE.txt',
    },
    'hpipm': {
        'url': 'https://github.com/giaf/hpipm',
        'commit': 'e3a56c1caddd7f12d125d84f337b9a9e5c186271',
        'prefix': 'external/hpipm', 'checkout': 'external/hpipm', 'license': 'LICENSE.txt',
    },
}
BACKENDS = ['GENERIC', 'X64_INTEL_CORE', 'X64_INTEL_HASWELL',
            'ARMV8A_ARM_CORTEX_A53', 'ARMV8A_ARM_CORTEX_A57', 'ARMV8A_APPLE_M1']
LOCAL_FILES = {'CMakeLists.txt', 'README.md', 'manifest.json', 'sources.cmake'}


def run(*args):
    return subprocess.check_output(list(map(str, args)))


def unpack(source, component, destination):
    record = UPSTREAM[component]
    paths = {
        'acados': ('acados', 'interfaces/acados_c', 'LICENSE'),
        'blasfeo': ('CMakeLists.txt', 'LICENSE.txt', 'blasfeo_target.h.in', 'include',
                    'auxiliary', 'blasfeo_hp_pm', 'blasfeo_hp_cm', 'blasfeo_ref',
                    'blasfeo_wr', 'blas_api', 'kernel'),
        'hpipm': ('CMakeLists.txt', 'LICENSE.txt', 'include', 'auxiliary', 'cond',
                  'dense_qp', 'ipm_core', 'ocp_qp', 'tree_ocp_qp', 'interfaces/c'),
    }[component]
    archive = run('git', '-C', source / record['checkout'], 'archive', record['commit'], *paths)
    with tarfile.open(fileobj=io.BytesIO(archive)) as tar:
        tar.extractall(destination, filter='data')


def cmake_sources(root, component, backend, temp):
    text = (root / ('acados/CMakeLists.txt' if component == 'acados' else 'CMakeLists.txt')).read_text()
    start = text.index('file(GLOB_RECURSE ACADOS_SRC') if component == 'acados' else text.index('# source files')
    fragment = text[start:text.index(f'add_library({component}', start)]
    variable = component.upper() + '_SRC'
    if component == 'acados':
        interface = (root / 'interfaces/acados_c/CMakeLists.txt').read_text()
        matches = re.findall(r'\$\{CMAKE_CURRENT_SOURCE_DIR\}/(\w+\.c)', interface)
        fragment += '\nlist(APPEND ACADOS_SRC\n' + '\n'.join(
            f'"{root.as_posix()}/interfaces/acados_c/{name}"' for name in matches) + '\n)\n'
    output = temp / f'{component}-{backend}.txt'
    script = temp / f'{component}-{backend}.cmake'
    variables = {
        'PROJECT_SOURCE_DIR': root.as_posix(), 'TARGET': backend,
        'TARGET2': 'ARMV8A_ARM_CORTEX_A57' if backend == 'ARMV8A_APPLE_M1' else 'PHANTOM',
        'LA': 'HIGH_PERFORMANCE', 'MF': 'PANELMAJ', 'DP_ROUTINES': 'ON',
        'SP_ROUTINES': 'OFF', 'BLASFEO_REF_API': 'ON', 'BLASFEO_HP_API': 'OFF',
        'BLAS_API': 'OFF', 'EXT_DEP': 'ON', 'EXT_DEP_MALLOC': 'ON',
    }
    script.write_text('cmake_minimum_required(VERSION 3.21)\n' + '\n'.join(
        f'set({key} "{value}")' for key, value in variables.items()) + '\n' + fragment +
        f'\nfile(WRITE "{output.as_posix()}" "${{{variable}}}")\n')
    run('cmake', '-P', script)
    files = sorted(set(Path(name).relative_to(root).as_posix()
                       for name in output.read_text().split(';') if name))
    if component == 'hpipm':
        # acados uses double precision throughout. HPIPM's upstream source list
        # has no precision switch; omit its single-precision instantiations.
        files = [name for name in files if not Path(name).name.startswith('s_')]
    if not files:
        raise RuntimeError(f'Upstream source list is empty: {component}/{backend}')
    return files


def source_closure(root, component, sources):
    header_roots = ['acados', 'interfaces/acados_c'] if component == 'acados' else ['include']
    keep = set(sources)
    for directory in header_roots:
        keep.update(p.relative_to(root).as_posix() for p in (root / directory).rglob('*.h'))
    keep.add(UPSTREAM[component]['license'])
    if component == 'blasfeo':
        keep.add('blasfeo_target.h.in')
    pending = list(keep)
    while pending:
        name = pending.pop()
        for included in re.findall(r'^\s*#\s*include\s*"([^"]+)"',
                                   (root / name).read_text(), re.MULTILINE):
            for candidate in ((root / name).parent / included, root / included, root / 'include' / included):
                if candidate.is_file():
                    relative = candidate.resolve().relative_to(root.resolve()).as_posix()
                    if relative not in keep:
                        keep.add(relative)
                        pending.append(relative)
                    break
    return sorted(keep)


def cmake_manifest(sources):
    lines = ['# Generated by scripts/vendor_acados.py. Do not edit.',
             'set(LF_BLASFEO_BACKENDS ' + ' '.join(BACKENDS) + ')', '']
    for name, files in sorted(sources.items()):
        lines += [f'set(LF_{name}_SOURCES'] + [f'    {path}' for path in files] + [')', '']
    return '\n'.join(lines)


def import_runtime(source, destination):
    manifest = {'format': 1, 'maximum_bytes': MAX_BYTES, 'precision': 'double', 'upstream': UPSTREAM,
                'blasfeo_backends': BACKENDS, 'files': {}, 'sources': {}}
    contents = {}
    with tempfile.TemporaryDirectory(prefix='florid-acados-') as scratch:
        temp = Path(scratch)
        for component, record in UPSTREAM.items():
            root = temp / component
            root.mkdir()
            unpack(source, component, root)
            all_sources = set()
            for backend in BACKENDS if component == 'blasfeo' else ['GENERIC']:
                selected = cmake_sources(root, component, backend, temp)
                all_sources.update(selected)
                key = f'BLASFEO_{backend}' if component == 'blasfeo' else component.upper()
                manifest['sources'][key] = [(Path(record['prefix']) / name).as_posix() for name in selected]
            for name in source_closure(root, component, all_sources):
                path = (Path(record['prefix']) / name).as_posix()
                data = (root / name).read_bytes()
                contents[path] = data
                manifest['files'][path] = {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}

    manifest['upstream_bytes'] = sum(len(data) for data in contents.values())
    contents['manifest.json'] = (json.dumps(manifest, indent=2, sort_keys=True) + '\n').encode()
    contents['sources.cmake'] = cmake_manifest(manifest['sources']).encode()
    size = sum(map(len, contents.values())) + sum(
        (destination / name).stat().st_size for name in LOCAL_FILES - contents.keys()
        if (destination / name).exists())
    if size > MAX_BYTES:
        raise RuntimeError(f'Selected runtime is {size:,} bytes, exceeds {MAX_BYTES:,}')
    previous = destination / 'manifest.json'
    if previous.exists():
        for stale in json.loads(previous.read_text())['files'].keys() - contents.keys():
            (destination / stale).unlink()
    for name, data in contents.items():
        path = destination / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
    check_runtime(destination)


def check_runtime(destination):
    manifest = json.loads((destination / 'manifest.json').read_text())
    if (manifest['format'] != 1 or manifest['precision'] != 'double'
            or manifest['maximum_bytes'] != MAX_BYTES):
        raise RuntimeError('Unsupported vendor manifest format or configuration')
    if manifest['upstream'] != UPSTREAM or manifest['blasfeo_backends'] != BACKENDS:
        raise RuntimeError('Vendor metadata does not match importer pins/backends')
    for name, record in manifest['files'].items():
        data = (destination / name).read_bytes()
        if len(data) != record['bytes'] or hashlib.sha256(data).hexdigest() != record['sha256']:
            raise RuntimeError(f'Vendored upstream file was modified: {name}')
    if sum(record['bytes'] for record in manifest['files'].values()) != manifest['upstream_bytes']:
        raise RuntimeError('Upstream size metadata is inconsistent')
    if (destination / 'sources.cmake').read_text() != cmake_manifest(manifest['sources']):
        raise RuntimeError('CMake source manifest differs from the import manifest')
    for source_list in manifest['sources'].values():
        if not set(source_list) <= manifest['files'].keys():
            raise RuntimeError('A compile source is missing from the vendor manifest')
    actual = {p.relative_to(destination).as_posix(): p for p in destination.rglob('*') if p.is_file()}
    unexpected = actual.keys() - manifest['files'].keys() - LOCAL_FILES
    if unexpected:
        raise RuntimeError(f'Unexpected vendor files: {sorted(unexpected)}')
    size = sum(p.stat().st_size for p in actual.values())
    if size > MAX_BYTES:
        raise RuntimeError(f'Vendor size {size:,} exceeds {MAX_BYTES:,} bytes')
    print(f'acados runtime: {len(actual)} files, {size:,} bytes '
          f'({size / 1e6:.3f} MB; limit {MAX_BYTES / 1e6:.0f} MB)')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, help='local acados checkout with BLASFEO/HPIPM Git objects')
    parser.add_argument('--outdir', type=Path, default=DESTINATION)
    parser.add_argument('--check', action='store_true', help='verify hashes, source lists and total size offline')
    args = parser.parse_args()
    if args.check:
        check_runtime(args.outdir)
    elif args.source:
        import_runtime(args.source.resolve(), args.outdir)
    else:
        parser.error('specify --source or --check')


if __name__ == '__main__':
    main()
