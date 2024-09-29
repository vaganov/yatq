from pathlib import Path
import subprocess
import sys


__all__ = ['inc_dirs', 'lib_dirs', 'libs']


dirpath = str(Path(__file__).parent)

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
