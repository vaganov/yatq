from datetime import datetime, timedelta
from functools import partial
import time


def test_smoke(timer_queue):
    x = 2

    def f():
        nonlocal x
        x += 1

    now = datetime.now()
    deadline = now + timedelta(milliseconds=200)
    timer_queue.enqueue(deadline=deadline, job=f)

    time.sleep(0.1)
    assert x == 2

    time.sleep(0.3)
    assert x == 3


def test_cancel(timer_queue):
    x = 2

    def f():
        nonlocal x
        x += 1

    now = datetime.now()
    deadline = now + timedelta(milliseconds=100)
    handle = timer_queue.enqueue(deadline=deadline, job=f)
    assert timer_queue.cancel(uid=handle.uid)

    time.sleep(0.2)
    assert x == 2


def test_two_timers(timer_queue):
    x = 2

    def f():
        nonlocal x
        x += 1

    def g():
        nonlocal x
        x *= 2

    now = datetime.now()
    deadline = now + timedelta(milliseconds=100)
    timer_queue.enqueue(deadline=deadline, job=f)
    timer_queue.enqueue(deadline=deadline, job=g)

    time.sleep(0.2)
    assert x == 6  # NB: order only guaranteed for single-threaded executor


def test_enqueue_prepend(timer_queue):
    x = 2

    def f():
        nonlocal x
        x += 1

    def g():
        nonlocal x
        x *= 2

    now = datetime.now()
    deadline = now + timedelta(milliseconds=200)
    timer_queue.enqueue(deadline=deadline, job=f)
    deadline = now + timedelta(milliseconds=100)
    timer_queue.enqueue(deadline=deadline, job=g)

    time.sleep(0.3)
    assert x == 5


def test_two_timers_cancel(timer_queue):
    x = 2

    def f():
        nonlocal x
        x += 1

    def g():
        nonlocal x
        x *= 2

    now = datetime.now()
    deadline = now + timedelta(milliseconds=100)
    handle = timer_queue.enqueue(deadline, f)
    timer_queue.enqueue(deadline, g)
    assert timer_queue.cancel(uid=handle.uid)

    time.sleep(0.2)
    assert x == 4


def test_two_timers_cancel_non_first(timer_queue):
    x = 2

    def f():
        nonlocal x
        x += 1

    def g():
        nonlocal x
        x *= 2

    now = datetime.now()
    deadline = now + timedelta(milliseconds=100)
    timer_queue.enqueue(deadline, f)
    handle = timer_queue.enqueue(deadline, g)
    assert timer_queue.cancel(uid=handle.uid)

    time.sleep(0.2)
    assert x == 3


def test_enqueue_from_callback(timer_queue):
    x = 2

    def f():
        nonlocal x
        x += 1

    def g(timer_queue):
        now = datetime.now()
        deadline = now + timedelta(milliseconds=100)
        timer_queue.enqueue(deadline, f)

    now = datetime.now()
    deadline = now + timedelta(milliseconds=100)
    timer_queue.enqueue(deadline, partial(g, timer_queue))

    time.sleep(0.3)
    assert x == 3


def test_cancel_from_callback(timer_queue):
    x = 2

    def f():
        nonlocal x
        x += 1

    def g(handle):
        assert timer_queue.cancel(uid=handle.uid)

    now = datetime.now()
    long_deadline = now + timedelta(milliseconds=200)
    handle = timer_queue.enqueue(deadline=long_deadline, job=f)
    short_deadline = now + timedelta(milliseconds=100)
    timer_queue.enqueue(short_deadline, partial(g, handle))

    time.sleep(0.3)
    assert x == 2


def test_enqueue_and_cancel_from_callback(timer_queue):
    x = 2

    def f():
        nonlocal x
        x += 1

    def g():
        timer_queue.enqueue(deadline=datetime.now(), job=lambda: None)

    def h(handle):
        timer_queue.cancel(uid=handle.uid)

    now = datetime.now()
    long_deadline = now + timedelta(milliseconds=200)
    handle = timer_queue.enqueue(deadline=long_deadline, job=f)
    short_deadline = now + timedelta(milliseconds=100)
    timer_queue.enqueue(deadline=short_deadline, job=g)
    timer_queue.enqueue(short_deadline, partial(h, handle))

    time.sleep(0.3)
    assert x == 2


def test_clear(timer_queue):
    x = 2

    def f():
        nonlocal x
        x += 1

    now = datetime.now()
    deadline = now + timedelta(milliseconds=100)
    handle = timer_queue.enqueue(deadline=deadline, job=f)

    timer_queue.clear()

    assert not timer_queue.cancel(uid=handle.uid)


def test_purge(timer_queue):
    x = 2

    def f():
        nonlocal x
        x += 1

    now = datetime.now()
    long_deadline = now + timedelta(milliseconds=200)
    handle = timer_queue.enqueue(deadline=long_deadline, job=f)
    short_deadline = now + timedelta(milliseconds=100)
    timer_queue.enqueue(deadline=short_deadline, job=f)

    timer_queue.cancel(uid=handle.uid)
    timer_queue.purge()

    time.sleep(0.3)
    assert x == 3


def test_in_queue(timer_queue):
    x = 2

    def f():
        nonlocal x
        x += 1

    now = datetime.now()
    long_deadline = now + timedelta(milliseconds=200)
    long_handle = timer_queue.enqueue(deadline=long_deadline, job=f)
    short_deadline = now + timedelta(milliseconds=100)
    short_handle = timer_queue.enqueue(deadline=short_deadline, job=f)

    assert timer_queue.in_queue(uid=long_handle.uid)
    assert timer_queue.in_queue(uid=short_handle.uid)
    timer_queue.cancel(uid=long_handle.uid)
    assert not timer_queue.in_queue(uid=long_handle.uid)

    time.sleep(0.3)
    assert x == 3
    assert not timer_queue.in_queue(uid=short_handle.uid)
