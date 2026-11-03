// Keep host threading headers separate from the kernel's pid_t.
#include <assert.h>
#include <threads.h>

static thrd_t thread;
static void (*worker)(void);

static int run_worker(void *arg)
{
	(void)arg;
	worker();
	return 0;
}

void test_thread_start(void (*fn)(void))
{
	worker = fn;
	assert(thrd_create(&thread, run_worker, NULL) == thrd_success);
}

void test_thread_join(void)
{
	int result;
	assert(thrd_join(thread, &result) == thrd_success);
	assert(result == 0);
}

void test_thread_yield(void)
{
	thrd_yield();
}
