import time


def test_smoke(thread_pool):
    x = 2

    def f():
        nonlocal x
        x += 1

    thread_pool.execute(job=f)

    time.sleep(0.1)
    assert x == 3
