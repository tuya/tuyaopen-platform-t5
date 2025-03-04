/*
 * Copyright (c) 2019-2020, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __VT_S_TESTS_H__
#define __VT_S_TESTS_H__

#include "test_framework.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Register testsuite for the PSA vulnerable S interface
 *        tests.
 *
 * \param[in] p_test_suite  The test suite to be executed.
 */
void register_testsuite_s_psa_vt_interface(struct test_suite_t *p_test_suite);

#ifdef __cplusplus
}
#endif

#endif /* __VT_S_TESTS_H__ */
