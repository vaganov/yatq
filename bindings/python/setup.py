from pathlib import Path
import subprocess
import sys

from setuptools import setup
from pybind11.setup_helpers import Pybind11Extension, build_ext


def ugly_helper():  # https://github.com/vaganov/yatq/issues/2
    dirpath = str(Path(__file__).parent / 'setup_helpers')

    args = ['cmake', '-P', 'find_dep_headers.cmake']
    result = subprocess.run(args, cwd=dirpath, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if result.returncode != 0:
        print(result.stderr.decode(), end='', file=sys.stderr, flush=True)
        raise RuntimeError(f"{' '.join(args)} returned {result.returncode}")

    _inc_dirs = set(line.strip() for line in result.stderr.decode().split())  # sic! 'stderr'
    inc_dirs = list(_inc_dirs)

    args = ['cmake', '-P', 'find_dep_libs.cmake']
    result = subprocess.run(args, cwd=dirpath, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if result.returncode != 0:
        print(result.stderr.decode(), end='', file=sys.stderr, flush=True)
        raise RuntimeError(f"{' '.join(args)} returned {result.returncode}")

    lib_prefix = 'lib'
    lib_prefix_len = len(lib_prefix)

    _lib_dirs = set()
    libs = []
    for line in result.stderr.decode().split():  # sic! 'stderr'
        path = Path(line.strip())
        _lib_dirs.add(str(path.parent))
        libname = path.stem
        if libname.startswith(lib_prefix):
            libname = libname[lib_prefix_len:]
        libs.append(libname)
    lib_dirs = list(_lib_dirs)

    return {'inc_dirs': inc_dirs, 'lib_dirs': lib_dirs, 'libs': libs}


HELPER_FLAGS = ugly_helper()


setup(
    packages=['pytq'],
    ext_modules=[
        Pybind11Extension(
            '_yatq',
            ['_yatq/bind.cpp'],
            include_dirs=HELPER_FLAGS['inc_dirs'] + ['_yatq/include'],  # https://github.com/vaganov/yatq/issues/4
            library_dirs=HELPER_FLAGS['lib_dirs'],
            libraries=HELPER_FLAGS['libs'],
            cxx_std=20,
        ),
    ],
    cmdclass=dict(build_ext=build_ext),
)
