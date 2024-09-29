import pytest

from pytq import ThreadPool, TimerQueue


@pytest.fixture(scope='session')
def thread_pool():
    thread_pool = ThreadPool()
    thread_pool.start(num_threads=1)

    yield thread_pool

    thread_pool.stop()


@pytest.fixture(scope='session')
def timer_queue(thread_pool):
    timer_queue = TimerQueue(executor=thread_pool)
    timer_queue.start()

    yield timer_queue

    timer_queue.stop()
