#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <tpool.h>
#include <bits/pthreadtypes.h>

typedef struct tpool_work {
    thread_func_t func;
    void *arg;
    struct tpool_work *next;
} tpool_work_t;

struct [[maybe_unused]] tpool_t {
    tpool_work_t *work_first;
    tpool_work_t *work_last;

    pthread_mutex_t work_mutex;
    pthread_cond_t work_cond;
    pthread_cond_t working_cond;
    size_t working_count;
    size_t thread_count;
    bool stop;
};

static tpool_work_t *tpool_work_create(thread_func_t func, void *arg) {
    if (func == nullptr)
        return nullptr;

    tpool_work_t *work = malloc(sizeof(*work));
    if (work == nullptr)
        return nullptr;

    work->func = func;
    work->arg = arg;
    work->next = nullptr;
    return work;
}

static void tpool_work_destroy(tpool_work_t *work) {
    if (work == nullptr) return;
    free(work);
}

static tpool_work_t *tpool_work_get(tpool_t *pool) {
    if (pool == nullptr)
        return nullptr;

    tpool_work_t *work = pool->work_first;
    if (work == nullptr)
        return nullptr;

    if (work->next == nullptr) {
        pool->work_first = nullptr;
        pool->work_last = nullptr;
    } else {
        pool->work_first = work->next;
    }

    return work;
}

static void *tpool_worker(void *arg) {
    tpool_t *pool = arg;

    while (1) {
        pthread_mutex_lock(&pool->work_mutex);

        while (pool->work_first == nullptr && !pool->stop)
            pthread_cond_wait(&pool->work_cond, &pool->work_mutex);

        if (pool->stop) break;

        tpool_work_t *work = tpool_work_get(pool);
        pool->working_count++;
        pthread_mutex_unlock(&pool->work_mutex);

        if (work != nullptr) {
            work->func(work->arg);
            tpool_work_destroy(work);
        }

        pthread_mutex_lock(&pool->work_mutex);
        pool->working_count--;
        if (!pool->stop && pool->working_count == 0 && pool->work_first == nullptr)
            pthread_cond_signal(&pool->working_cond);
        pthread_mutex_unlock(&pool->work_mutex);
    }

    pool->thread_count--;
    pthread_cond_signal(&pool->working_cond);
    pthread_mutex_unlock(&pool->work_mutex);
    return nullptr;
}

tpool_t *tpool_create(size_t num) {
    if (num == 0)
        num = 2;

    tpool_t *pool = calloc(1, sizeof(*pool));
    pool->thread_count = num;

    pthread_mutex_init(&pool->work_mutex, nullptr);
    pthread_cond_init(&pool->work_cond, nullptr);
    pthread_cond_init(&pool->working_cond, nullptr);

    pool->work_first = nullptr;
    pool->work_last = nullptr;

    for (size_t i = 0; i < num; i++) {
        pthread_t thread;
        pthread_create(&thread, nullptr, tpool_worker, pool);
        pthread_detach(thread);
    }

    return pool;
}

void tpool_destroy(tpool_t *pool) {
    if (pool == nullptr) return;

    pthread_mutex_lock(&pool->work_mutex);
    tpool_work_t *work = pool->work_first;

    while (work != nullptr) {
        tpool_work_t *work2 = work->next;
        tpool_work_destroy(work);
        work = work2;
    }

    pool->work_first = nullptr;
    pool->stop = true;

    pthread_cond_broadcast(&pool->work_cond);
    pthread_mutex_unlock(&pool->work_mutex);

    tpool_wait(pool);

    pthread_mutex_destroy(&pool->work_mutex);
    pthread_cond_destroy(&pool->work_cond);
    pthread_cond_destroy(&pool->working_cond);

    free(pool);

    printf("Thread pool resources cleaned up successfully.\n");
}

bool tpool_add_work(tpool_t *pool, thread_func_t func, void *arg) {
    if (pool == nullptr) return false;

    tpool_work_t *work = tpool_work_create(func, arg);
    if (work == nullptr) return false;

    pthread_mutex_lock(&pool->work_mutex);
    if (pool->work_first == nullptr) {
        pool->work_first = work;
        pool->work_last = pool->work_first;
    } else {
        pool->work_last->next = work;
        pool->work_last = work;
    }

    pthread_cond_broadcast(&pool->work_cond);
    pthread_mutex_unlock(&pool->work_mutex);

    return true;
}

void tpool_wait(tpool_t *pool) {
    if (pool == nullptr) return;

    pthread_mutex_lock(&pool->work_mutex);
    while (1) {
        if (pool->work_first != nullptr || (!pool->stop && pool->working_count != 0) || (pool->stop && pool->thread_count != 0)) {
            pthread_cond_wait(&pool->working_cond, &pool->work_mutex);
        } else {
            break;
        }
    }
    pthread_mutex_unlock(&pool->work_mutex);
}

