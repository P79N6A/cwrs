/*
 * Copyright (c) 2012 Red Hat Inc
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above
 *       copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the
 *       above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or
 *       other materials provided with the distribution.
 *     * The names of contributors to this software may not be
 *       used to endorse or promote products derived from this
 *       software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * Author: Stef Walter <stefw@redhat.com>
 */

#include "config.h"
#include "CuTest.h"

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "p11-kit.h"
#include "private.h"
#include "hashmap.h"

static CK_FUNCTION_LIST_PTR_PTR
initialize_and_get_modules (CuTest *tc)
{
	CK_FUNCTION_LIST_PTR_PTR modules;
	CK_RV rv;

	rv = p11_kit_initialize_registered ();
	CuAssertIntEquals (tc, CKR_OK, rv);
	modules = p11_kit_registered_modules ();
	CuAssertTrue (tc, modules != NULL && modules[0] != NULL);

	return modules;
}

static void
finalize_and_free_modules (CuTest *tc,
                           CK_FUNCTION_LIST_PTR_PTR modules)
{
	CK_RV rv;

	free (modules);
	rv = p11_kit_finalize_registered ();
	CuAssertIntEquals (tc, CKR_OK, rv);
}

static void
test_no_duplicates (CuTest *tc)
{
	CK_FUNCTION_LIST_PTR_PTR modules;
	hashmap *paths;
	hashmap *funcs;
	char *path;
	int i;

	modules = initialize_and_get_modules (tc);
	paths = _p11_hash_create (_p11_hash_string_hash, _p11_hash_string_equal, NULL, NULL);
	funcs = _p11_hash_create (_p11_hash_direct_hash, _p11_hash_direct_equal, NULL, NULL);

	/* The loaded modules should not contain duplicates */
	for (i = 0; modules[i] != NULL; i++) {
		path = p11_kit_registered_option (modules[i], "module");

		if (_p11_hash_get (funcs, modules[i]))
			CuAssert (tc, "found duplicate function list pointer", 0);
		if (_p11_hash_get (paths, path))
			CuAssert (tc, "found duplicate path name", 0);

		if (!_p11_hash_set (funcs, modules[i], ""))
			CuAssert (tc, "shouldn't be reached", 0);
		if (!_p11_hash_set (paths, path, ""))
			CuAssert (tc, "shouldn't be reached", 0);
	}

	_p11_hash_free (paths);
	_p11_hash_free (funcs);
	finalize_and_free_modules (tc, modules);
}

static CK_FUNCTION_LIST_PTR
lookup_module_with_name (CuTest *tc,
                         CK_FUNCTION_LIST_PTR_PTR modules,
                         const char *name)
{
	CK_FUNCTION_LIST_PTR match = NULL;
	CK_FUNCTION_LIST_PTR module;
	char *module_name;
	int i;

	for (i = 0; match == NULL && modules[i] != NULL; i++) {
		module_name = p11_kit_registered_module_to_name (modules[i]);
		CuAssertPtrNotNull (tc, module_name);
		if (strcmp (module_name, name) == 0)
			match = modules[i];
		free (module_name);
	}

	/*
	 * As a side effect, we should check that the results of this function
	 * matches the above search.
	 */
	module = p11_kit_registered_name_to_module (name);
	CuAssert(tc, "different result from p11_kit_registered_name_to_module()",
	         module == match);

	return match;
}

static void
test_disable (CuTest *tc)
{
	CK_FUNCTION_LIST_PTR_PTR modules;

	/*
	 * The module four should be present, as we don't match any prognames
	 * that it has disabled.
	 */

	modules = initialize_and_get_modules (tc);
	CuAssertTrue (tc, lookup_module_with_name (tc, modules, "four") != NULL);
	finalize_and_free_modules (tc, modules);

	/*
	 * The module two shouldn't have been loaded, because in its config
	 * file we have:
	 *
	 * disable-in: test-disable
	 */

	p11_kit_set_progname ("test-disable");

	modules = initialize_and_get_modules (tc);
	CuAssertTrue (tc, lookup_module_with_name (tc, modules, "four") == NULL);
	finalize_and_free_modules (tc, modules);

	p11_kit_set_progname (NULL);
}

static void
test_disable_later (CuTest *tc)
{
	CK_FUNCTION_LIST_PTR_PTR modules;
	CK_RV rv;

	/*
	 * The module two shouldn't be matched, because in its config
	 * file we have:
	 *
	 * disable-in: test-disable
	 */

	rv = p11_kit_initialize_registered ();
	CuAssertIntEquals (tc, CKR_OK, rv);

	p11_kit_set_progname ("test-disable");

	modules = p11_kit_registered_modules ();
	CuAssertTrue (tc, modules != NULL && modules[0] != NULL);

	CuAssertTrue (tc, lookup_module_with_name (tc, modules, "two") == NULL);
	finalize_and_free_modules (tc, modules);

	p11_kit_set_progname (NULL);
}

static void
test_enable (CuTest *tc)
{
	CK_FUNCTION_LIST_PTR_PTR modules;

	/*
	 * The module three should not be present, as we don't match the current
	 * program.
	 */

	modules = initialize_and_get_modules (tc);
	CuAssertTrue (tc, lookup_module_with_name (tc, modules, "three") == NULL);
	finalize_and_free_modules (tc, modules);

	/*
	 * The module three should be loaded here , because in its config
	 * file we have:
	 *
	 * enable-in: test-enable
	 */

	p11_kit_set_progname ("test-enable");

	modules = initialize_and_get_modules (tc);
	CuAssertTrue (tc, lookup_module_with_name (tc, modules, "three") != NULL);
	finalize_and_free_modules (tc, modules);

	p11_kit_set_progname (NULL);
}

int
main (void)
{
	CuString *output = CuStringNew ();
	CuSuite* suite = CuSuiteNew ();
	int ret;

	_p11_library_init ();

	SUITE_ADD_TEST (suite, test_no_duplicates);
	SUITE_ADD_TEST (suite, test_disable);
	SUITE_ADD_TEST (suite, test_disable_later);
	SUITE_ADD_TEST (suite, test_enable);

	p11_kit_be_quiet ();

	CuSuiteRun (suite);
	CuSuiteSummary (suite, output);
	CuSuiteDetails (suite, output);
	printf ("%s\n", output->buffer);
	ret = suite->failCount;
	CuSuiteDelete (suite);
	CuStringDelete (output);
	return ret;
}
