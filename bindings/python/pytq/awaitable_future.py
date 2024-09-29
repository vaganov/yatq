import asyncio


__all__ = ['AwaitableFuture']


class AwaitableFuture:
    def __init__(self, future):
        self._future = future

    def __await__(self):
        while not self._future.is_ready():
            yield
        return self._future.get()  # NB: return, not yield

    async def get(self):
        while not self._future.is_ready():
            await asyncio.sleep(0)
        return self._future.get()
