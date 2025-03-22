import asyncio
from typing import Any

import _yatq.boost


__all__ = ['pythonize']


def pythonize(src: _yatq.boost.future) -> asyncio.Future[Any]:
    chained = asyncio.Future()

    def on_done(future: _yatq.boost.future):
        try:
            chained.get_loop().call_soon_threadsafe(chained.set_result, future.get())
        except Exception as exc:
            chained.set_exception(exc)

    src.then(func=on_done, policy=_yatq.boost.launch.sync)
    return chained
