import asyncio
from datetime import datetime, timedelta
import os
import sys

from pytq import TimerQueue, ThreadPool, pythonize


class C:
    def __init__(self, s):
        self.s = s

    def __repr__(self):
        return f'C(s={self.s})'


async def main():
    thread_pool = ThreadPool()
    timer_queue = TimerQueue(executor=thread_pool)

    thread_pool.start(num_threads=1)
    timer_queue.start(sched_policy=os.SCHED_FIFO)

    def raise_exc():
        raise RuntimeError('test')

    deadline = datetime.now() + timedelta(milliseconds=500)

    handle = timer_queue.enqueue(deadline=deadline, job=lambda: C('test'))
    another_handle = timer_queue.enqueue(deadline=deadline, job=lambda: C("won't make it"))
    exc_handle = timer_queue.enqueue(deadline=deadline, job=raise_exc)

    timer_queue.cancel(uid=another_handle.uid)

    future = pythonize(handle.result)
    return_value = await future
    print(return_value)

    future = pythonize(exc_handle.result)
    try:
        await future
    except Exception as exc:
        print(exc, file=sys.stderr)

    timer_queue.stop()
    thread_pool.stop()


if __name__ == '__main__':
    asyncio.run(main())
