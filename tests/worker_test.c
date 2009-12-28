/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#include "config.h"

#if defined(NDEBUG)
# undef NDEBUG
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libgearman/gearman.h>

#include "test.h"
#include "test_gearmand.h"

#define WORKER_TEST_PORT 32123

typedef struct
{
  pid_t gearmand_pid;
  gearman_worker_st worker;
} worker_test_st;

/* Prototypes */
test_return_t init_test(void *object);
test_return_t allocation_test(void *object);
test_return_t clone_test(void *object);
test_return_t echo_test(void *object);
test_return_t option_test(void *object);
test_return_t bug372074_test(void *object);

void *create(void *object);
void destroy(void *object);
test_return_t pre(void *object);
test_return_t post(void *object);
test_return_t flush(void);

void *world_create(test_return_t *error);
test_return_t world_destroy(void *object);

test_return_t init_test(void *object __attribute__((unused)))
{
  gearman_worker_st worker;

  if (gearman_worker_create(&worker) == NULL)
    return TEST_FAILURE;

  gearman_worker_free(&worker);

  return TEST_SUCCESS;
}

test_return_t allocation_test(void *object __attribute__((unused)))
{
  gearman_worker_st *worker;

  worker= gearman_worker_create(NULL);
  if (worker == NULL)
    return TEST_FAILURE;

  gearman_worker_free(worker);

  return TEST_SUCCESS;
}

test_return_t clone_test(void *object)
{
  gearman_worker_st *from= (gearman_worker_st *)object;
  gearman_worker_st *worker;

  worker= gearman_worker_clone(NULL, NULL);

  test_truth(worker);
  test_truth(worker->options.allocated);

  gearman_worker_free(worker);

  worker= gearman_worker_clone(NULL, from);
  if (worker == NULL)
    return TEST_FAILURE;

  gearman_worker_free(worker);

  return TEST_SUCCESS;
}

test_return_t option_test(void *object __attribute__((unused)))
{
  gearman_worker_st *gear;
  gearman_worker_options_t default_options;

  gear= gearman_worker_create(NULL);
  test_truth(gear);
  { // Initial Allocated, no changes
    test_truth(gear->options.allocated);
    test_false(gear->options.non_blocking);
    test_truth(gear->options.packet_init);
    test_false(gear->options.grab_job_in_use);
    test_false(gear->options.pre_sleep_in_use);
    test_false(gear->options.work_job_in_use);
    test_false(gear->options.change);
    test_false(gear->options.grab_uniq);
    test_false(gear->options.timeout_return);
  }

  /* Set up for default options */
  default_options= gearman_worker_options(gear);

  /*
    We take the basic options, and push
    them back in. See if we change anything.
  */
  gearman_worker_set_options(gear, default_options);
  { // Initial Allocated, no changes
    test_truth(gear->options.allocated);
    test_false(gear->options.non_blocking);
    test_truth(gear->options.packet_init);
    test_false(gear->options.grab_job_in_use);
    test_false(gear->options.pre_sleep_in_use);
    test_false(gear->options.work_job_in_use);
    test_false(gear->options.change);
    test_false(gear->options.grab_uniq);
    test_false(gear->options.timeout_return);
  }

  /*
    We will trying to modify non-mutable options (which should not be allowed)
  */
  {
    gearman_worker_remove_options(gear, GEARMAN_WORKER_ALLOCATED);
    { // Initial Allocated, no changes
      test_truth(gear->options.allocated);
      test_false(gear->options.non_blocking);
      test_truth(gear->options.packet_init);
      test_false(gear->options.grab_job_in_use);
      test_false(gear->options.pre_sleep_in_use);
      test_false(gear->options.work_job_in_use);
      test_false(gear->options.change);
      test_false(gear->options.grab_uniq);
      test_false(gear->options.timeout_return);
    }
    gearman_worker_remove_options(gear, GEARMAN_WORKER_PACKET_INIT);
    { // Initial Allocated, no changes
      test_truth(gear->options.allocated);
      test_false(gear->options.non_blocking);
      test_truth(gear->options.packet_init);
      test_false(gear->options.grab_job_in_use);
      test_false(gear->options.pre_sleep_in_use);
      test_false(gear->options.work_job_in_use);
      test_false(gear->options.change);
      test_false(gear->options.grab_uniq);
      test_false(gear->options.timeout_return);
    }
  }

  /*
    We will test modifying GEARMAN_WORKER_NON_BLOCKING in several manners.
  */
  {
    gearman_worker_remove_options(gear, GEARMAN_WORKER_NON_BLOCKING);
    { // GEARMAN_WORKER_NON_BLOCKING set to default, by default.
      test_truth(gear->options.allocated);
      test_false(gear->options.non_blocking);
      test_truth(gear->options.packet_init);
      test_false(gear->options.grab_job_in_use);
      test_false(gear->options.pre_sleep_in_use);
      test_false(gear->options.work_job_in_use);
      test_false(gear->options.change);
      test_false(gear->options.grab_uniq);
      test_false(gear->options.timeout_return);
    }
    gearman_worker_add_options(gear, GEARMAN_WORKER_NON_BLOCKING);
    { // GEARMAN_WORKER_NON_BLOCKING set to default, by default.
      test_truth(gear->options.allocated);
      test_truth(gear->options.non_blocking);
      test_truth(gear->options.packet_init);
      test_false(gear->options.grab_job_in_use);
      test_false(gear->options.pre_sleep_in_use);
      test_false(gear->options.work_job_in_use);
      test_false(gear->options.change);
      test_false(gear->options.grab_uniq);
      test_false(gear->options.timeout_return);
    }
    gearman_worker_set_options(gear, GEARMAN_WORKER_NON_BLOCKING);
    { // GEARMAN_WORKER_NON_BLOCKING set to default, by default.
      test_truth(gear->options.allocated);
      test_truth(gear->options.non_blocking);
      test_truth(gear->options.packet_init);
      test_false(gear->options.grab_job_in_use);
      test_false(gear->options.pre_sleep_in_use);
      test_false(gear->options.work_job_in_use);
      test_false(gear->options.change);
      test_false(gear->options.grab_uniq);
      test_false(gear->options.timeout_return);
    }
    gearman_worker_set_options(gear, GEARMAN_WORKER_GRAB_UNIQ);
    { // Everything is now set to false except GEARMAN_WORKER_GRAB_UNIQ, and non-mutable options
      test_truth(gear->options.allocated);
      test_false(gear->options.non_blocking);
      test_truth(gear->options.packet_init);
      test_false(gear->options.grab_job_in_use);
      test_false(gear->options.pre_sleep_in_use);
      test_false(gear->options.work_job_in_use);
      test_false(gear->options.change);
      test_truth(gear->options.grab_uniq);
      test_false(gear->options.timeout_return);
    }
    /*
      Reset options to default. Then add an option, and then add more options. Make sure
      the options are all additive.
    */
    {
      gearman_worker_set_options(gear, default_options);
      { // See if we return to defaults
        test_truth(gear->options.allocated);
        test_false(gear->options.non_blocking);
        test_truth(gear->options.packet_init);
        test_false(gear->options.grab_job_in_use);
        test_false(gear->options.pre_sleep_in_use);
        test_false(gear->options.work_job_in_use);
        test_false(gear->options.change);
        test_false(gear->options.grab_uniq);
        test_false(gear->options.timeout_return);
      }
      gearman_worker_add_options(gear, GEARMAN_WORKER_TIMEOUT_RETURN);
      { // All defaults, except timeout_return
        test_truth(gear->options.allocated);
        test_false(gear->options.non_blocking);
        test_truth(gear->options.packet_init);
        test_false(gear->options.grab_job_in_use);
        test_false(gear->options.pre_sleep_in_use);
        test_false(gear->options.work_job_in_use);
        test_false(gear->options.change);
        test_false(gear->options.grab_uniq);
        test_truth(gear->options.timeout_return);
      }
      gearman_worker_add_options(gear, GEARMAN_WORKER_NON_BLOCKING|GEARMAN_WORKER_GRAB_UNIQ);
      { // GEARMAN_WORKER_NON_BLOCKING set to default, by default.
        test_truth(gear->options.allocated);
        test_truth(gear->options.non_blocking);
        test_truth(gear->options.packet_init);
        test_false(gear->options.grab_job_in_use);
        test_false(gear->options.pre_sleep_in_use);
        test_false(gear->options.work_job_in_use);
        test_false(gear->options.change);
        test_truth(gear->options.grab_uniq);
        test_truth(gear->options.timeout_return);
      }
    }
    /*
      Add an option, and then replace with that option plus a new option.
    */
    {
      gearman_worker_set_options(gear, default_options);
      { // See if we return to defaults
        test_truth(gear->options.allocated);
        test_false(gear->options.non_blocking);
        test_truth(gear->options.packet_init);
        test_false(gear->options.grab_job_in_use);
        test_false(gear->options.pre_sleep_in_use);
        test_false(gear->options.work_job_in_use);
        test_false(gear->options.change);
        test_false(gear->options.grab_uniq);
        test_false(gear->options.timeout_return);
      }
      gearman_worker_add_options(gear, GEARMAN_WORKER_TIMEOUT_RETURN);
      { // All defaults, except timeout_return
        test_truth(gear->options.allocated);
        test_false(gear->options.non_blocking);
        test_truth(gear->options.packet_init);
        test_false(gear->options.grab_job_in_use);
        test_false(gear->options.pre_sleep_in_use);
        test_false(gear->options.work_job_in_use);
        test_false(gear->options.change);
        test_false(gear->options.grab_uniq);
        test_truth(gear->options.timeout_return);
      }
      gearman_worker_add_options(gear, GEARMAN_WORKER_TIMEOUT_RETURN|GEARMAN_WORKER_GRAB_UNIQ);
      { // GEARMAN_WORKER_NON_BLOCKING set to default, by default.
        test_truth(gear->options.allocated);
        test_false(gear->options.non_blocking);
        test_truth(gear->options.packet_init);
        test_false(gear->options.grab_job_in_use);
        test_false(gear->options.pre_sleep_in_use);
        test_false(gear->options.work_job_in_use);
        test_false(gear->options.change);
        test_truth(gear->options.grab_uniq);
        test_truth(gear->options.timeout_return);
      }
    }
  }

  gearman_worker_free(gear);

  return TEST_SUCCESS;
}

test_return_t echo_test(void *object)
{
  gearman_worker_st *worker= (gearman_worker_st *)object;
  gearman_return_t rc;
  size_t value_length;
  const char *value= "This is my echo test";

  value_length= strlen(value);

  rc= gearman_worker_echo(worker, (uint8_t *)value, value_length);
  if (rc != GEARMAN_SUCCESS)
    return TEST_FAILURE;

  return TEST_SUCCESS;
}

void *create(void *object __attribute__((unused)))
{
  worker_test_st *test= (worker_test_st *)object;
  return (void *)&(test->worker);
}

void *world_create(test_return_t *error)
{
  worker_test_st *test;

  test= malloc(sizeof(worker_test_st));

  if (! test)
  {
    *error= TEST_MEMORY_ALLOCATION_FAILURE;
    return NULL;
  }

  memset(test, 0, sizeof(worker_test_st));
  if (gearman_worker_create(&(test->worker)) == NULL)
  {
    *error= TEST_FAILURE;
    return NULL;
  }

  if (gearman_worker_add_server(&(test->worker), NULL, WORKER_TEST_PORT) != GEARMAN_SUCCESS)
  {
    *error= TEST_FAILURE;
    return NULL;
  }

  test->gearmand_pid= test_gearmand_start(WORKER_TEST_PORT, NULL, NULL, 0);

  *error= TEST_SUCCESS;

  return (void *)test;
}

test_return_t world_destroy(void *object)
{
  worker_test_st *test= (worker_test_st *)object;
  gearman_worker_free(&(test->worker));
  test_gearmand_stop(test->gearmand_pid);
  free(test);

  return TEST_SUCCESS;
}

test_st tests[] ={
  {"init", 0, init_test },
  {"allocation", 0, allocation_test },
  {"clone", 0, clone_test },
  {"echo", 0, echo_test },
  {"options", 0, option_test },
  {0, 0, 0}
};

collection_st collection[] ={
  {"worker", 0, 0, tests},
  {0, 0, 0, 0}
};


typedef test_return_t (*libgearman_test_callback_fn)(gearman_worker_st *);
static test_return_t _runner_default(libgearman_test_callback_fn func, worker_test_st *container)
{
  if (func)
  {
    return func(&container->worker);
  }
  else
  {
    return TEST_SUCCESS;
  }
}


static world_runner_st runner= {
  (test_callback_runner_fn)_runner_default,
  (test_callback_runner_fn)_runner_default,
  (test_callback_runner_fn)_runner_default
};

void get_world(world_st *world)
{
  world->collections= collection;
  world->create= world_create;
  world->destroy= world_destroy;
  world->runner= &runner;
}
