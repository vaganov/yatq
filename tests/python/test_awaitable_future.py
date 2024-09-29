import pytest

import asyncio
from datetime import datetime, timedelta

from pytq import AwaitableFuture


@pytest.mark.asyncio
async def test_yield(timer_queue):
    cnt = 0

    async def coro():
        nonlocal cnt
        while True:
            cnt += 1
            await asyncio.sleep(0.01)  # 10 ms

    now = datetime.now()
    deadline = now + timedelta(milliseconds=100)
    handle = timer_queue.enqueue(deadline=deadline, job=lambda: None)
    future = AwaitableFuture(handle.result)

    await asyncio.wait([asyncio.create_task(coro()), asyncio.create_task(future.get())], return_when=asyncio.FIRST_COMPLETED)
    assert cnt >= 10


@pytest.mark.asyncio
async def test_return_none(thread_pool):
    result = thread_pool.execute(job=lambda: None)
    future = AwaitableFuture(result)
    return_value = await future
    assert return_value is None


@pytest.mark.asyncio
async def test_return_scalar(thread_pool):
    result = thread_pool.execute(job=lambda: 1)
    future = AwaitableFuture(result)
    return_value = await future
    assert return_value == 1


@pytest.mark.asyncio
async def test_return_object(thread_pool):
    class C:
        pass

    obj = C()
    result = thread_pool.execute(job=lambda: obj)
    future = AwaitableFuture(result)
    return_value = await future
    assert return_value is obj
