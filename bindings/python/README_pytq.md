# pytq-cxx
**pytq-cxx** is a _python_ wrapper for **yatq** -- a _C++_ template library. For the documentation and examples please
see [yatq homepage](https://github.com/vaganov/yatq)

**NB:** This package has nothing to do with [pytq package](https://pypi.org/project/pytq)

**NB:** Module name is, however, **pytq**

    from pytq import ThreadPool, TimerQueue, pythonize

**NB:** On _macOS_ one may need to specify **MACOSX_DEPLOYMENT_TARGET** environment variable on install. Do not set
higher than the current _macOS_ version. Minimal supported version is **13.3**

    (venv) $ MACOSX_DEPLOYMENT_TARGET=13.3 pip install pytq-cxx
