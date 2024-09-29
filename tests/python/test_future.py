from datetime import datetime, timedelta


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
