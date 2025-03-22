import pytest

from datetime import datetime, timedelta

import _yatq.boost


def test_get_non_blocks(timer_queue):
    x = 2

    def f():
        nonlocal x
        x += 1

    now = datetime.now()
    long_deadline = now + timedelta(milliseconds=200)
    handle = timer_queue.enqueue(deadline=long_deadline, job=f)
    short_deadline = now + timedelta(milliseconds=100)
    timer_queue.enqueue(deadline=short_deadline, job=f)

    handle.result.get()

    assert x == 4


def test_wait_non_blocks(timer_queue):
    x = 2

    def f():
        nonlocal x
        x += 1

    now = datetime.now()
    long_deadline = now + timedelta(milliseconds=200)
    handle = timer_queue.enqueue(deadline=long_deadline, job=f)
    short_deadline = now + timedelta(milliseconds=100)
    timer_queue.enqueue(deadline=short_deadline, job=f)

    handle.result.wait()

    assert x == 4


def test_then_default_policy(thread_pool):
    x = 2

    def f(future):
        nonlocal x
        x += 1

    result = thread_pool.execute(job=lambda: None)
    chained = result.then(func=f)
    chained.wait()

    assert x == 3


@pytest.mark.parametrize(
    'policy',
    [
        _yatq.boost.launch.none,
        _yatq.boost.launch.async_,
        _yatq.boost.launch.deferred,
        # _yatq.boost.launch.executor,
        _yatq.boost.launch.inherit,
        _yatq.boost.launch.sync,
        _yatq.boost.launch.any,
    ],
    ids=[
        'policy_none',
        'policy_async',
        'policy_deferred',
        # 'policy_executor',
        'policy_inherit',
        'policy_sync',
        'policy_any',
    ],
)
def test_then(thread_pool, policy):
    x = 2

    def f(future):
        nonlocal x
        x += 1

    result = thread_pool.execute(job=lambda: None)
    chained = result.then(func=f, policy=policy)
    chained.wait()

    assert x == 3
