from setuptools import setup
from pybind11.setup_helpers import Pybind11Extension, build_ext

from setup_helpers.ugly import inc_dirs, lib_dirs, libs  # https://github.com/vaganov/yatq/issues/2


setup(
    packages=['pytq'],
    ext_modules=[
        Pybind11Extension(
            '_yatq',
            ['_yatq/bind.cpp'],
            include_dirs=inc_dirs + ['../../include'],
            library_dirs=lib_dirs,
            libraries=libs,
            cxx_std=20,
        ),
    ],
    cmdclass=dict(build_ext=build_ext),
)
