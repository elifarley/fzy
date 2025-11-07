#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>

#include "../config.h"
#include "options.h"
#include "choices.h"

#include "greatest/greatest.h"

#define ASSERT_SIZE_T_EQ(a,b) ASSERT_EQ_FMT((size_t)(a), (b), "%zu")

static options_t default_options;
static choices_t choices;

static void setup(void *udata) {
    (void)udata;

    options_init(&default_options);
    choices_init(&choices, &default_options);
}

static void teardown(void *udata) {
    (void)udata;
    choices_destroy(&choices);
}

TEST test_choices_empty() {
	ASSERT_SIZE_T_EQ(0, choices.size);
	ASSERT_SIZE_T_EQ(0, choices.available);
	ASSERT_SIZE_T_EQ(0, choices.selection);

	choices_prev(&choices);
	ASSERT_SIZE_T_EQ(0, choices.selection);

	choices_next(&choices);
	ASSERT_SIZE_T_EQ(0, choices.selection);

	PASS();
}

TEST test_choices_1() {
	choices_add(&choices, "tags");

	choices_search(&choices, "");
	ASSERT_SIZE_T_EQ(1, choices.available);
	ASSERT_SIZE_T_EQ(0, choices.selection);

	choices_search(&choices, "t");
	ASSERT_SIZE_T_EQ(1, choices.available);
	ASSERT_SIZE_T_EQ(0, choices.selection);

	choices_prev(&choices);
	ASSERT_SIZE_T_EQ(0, choices.selection);

	choices_next(&choices);
	ASSERT_SIZE_T_EQ(0, choices.selection);

	ASSERT(!strcmp(choices_get(&choices, 0), "tags"));
	ASSERT_EQ(NULL, choices_get(&choices, 1));

	PASS();
}

TEST test_choices_2() {
	choices_add(&choices, "tags");
	choices_add(&choices, "test");

	/* Empty search */
	choices_search(&choices, "");
	ASSERT_SIZE_T_EQ(0, choices.selection);
	ASSERT_SIZE_T_EQ(2, choices.available);

	choices_next(&choices);
	ASSERT_SIZE_T_EQ(1, choices.selection);
	choices_next(&choices);
	ASSERT_SIZE_T_EQ(0, choices.selection);

	choices_prev(&choices);
	ASSERT_SIZE_T_EQ(1, choices.selection);
	choices_prev(&choices);
	ASSERT_SIZE_T_EQ(0, choices.selection);

	/* Filtered search */
	choices_search(&choices, "te");
	ASSERT_SIZE_T_EQ(1, choices.available);
	ASSERT_SIZE_T_EQ(0, choices.selection);
	ASSERT_STR_EQ("test", choices_get(&choices, 0));

	choices_next(&choices);
	ASSERT_SIZE_T_EQ(0, choices.selection);

	choices_prev(&choices);
	ASSERT_SIZE_T_EQ(0, choices.selection);

	/* No results */
	choices_search(&choices, "foobar");
	ASSERT_SIZE_T_EQ(0, choices.available);
	ASSERT_SIZE_T_EQ(0, choices.selection);

	/* Different order due to scoring */
	choices_search(&choices, "ts");
	ASSERT_SIZE_T_EQ(2, choices.available);
	ASSERT_SIZE_T_EQ(0, choices.selection);
	ASSERT_STR_EQ("test", choices_get(&choices, 0));
	ASSERT_STR_EQ("tags", choices_get(&choices, 1));

	PASS();
}

TEST test_choices_without_search() {
	/* Before a search is run, it should return no results */

	ASSERT_SIZE_T_EQ(0, choices.available);
	ASSERT_SIZE_T_EQ(0, choices.selection);
	ASSERT_SIZE_T_EQ(0, choices.size);
	ASSERT_EQ(NULL, choices_get(&choices, 0));

	choices_add(&choices, "test");

	ASSERT_SIZE_T_EQ(0, choices.available);
	ASSERT_SIZE_T_EQ(0, choices.selection);
	ASSERT_SIZE_T_EQ(1, choices.size);
	ASSERT_EQ(NULL, choices_get(&choices, 0));

	PASS();
}

/* Regression test for segfault */
TEST test_choices_unicode() {
	choices_add(&choices, "Edmund Husserl - Méditations cartésiennes - Introduction a la phénoménologie.pdf");
	choices_search(&choices, "e");

	PASS();
}

TEST test_choices_large_input() {
	const int N = 100000;
	char *strings[100000];

	for(int i = 0; i < N; i++) {
		asprintf(&strings[i], "%i", i);
		choices_add(&choices, strings[i]);
	}

	choices_search(&choices, "12");

	/* Must match `seq 0 99999 | grep '.*1.*2.*' | wc -l` */
	ASSERT_SIZE_T_EQ(8146, choices.available);

	ASSERT_STR_EQ("12", choices_get(&choices, 0));

	for(int i = 0; i < N; i++) {
		free(strings[i]);
	}

	PASS();
}

TEST test_choices_get_index() {
	choices_add(&choices, "apple");    // index 0
	choices_add(&choices, "banana");   // index 1
	choices_add(&choices, "apricot");  // index 2
	choices_add(&choices, "avocado");  // index 3

	/* Search for "a" - should return apple, apricot, avocado (in score order) */
	choices_search(&choices, "a");
	ASSERT_SIZE_T_EQ(4, choices.available);
	/* Verify the strings are in expected order */
	ASSERT_STR_EQ("apple", choices_get(&choices, 0));
	ASSERT_STR_EQ("apricot", choices_get(&choices, 1));
	ASSERT_STR_EQ("avocado", choices_get(&choices, 2));
	ASSERT_STR_EQ("banana", choices_get(&choices, 3));
	/* Verify indexes match original positions */
	ASSERT_SIZE_T_EQ(0, choices_getindex(&choices, 0)); // apple was at index 0
	ASSERT_SIZE_T_EQ(2, choices_getindex(&choices, 1)); // apricot was at index 2
	ASSERT_SIZE_T_EQ(3, choices_getindex(&choices, 2)); // avocado was at index 3
	ASSERT_SIZE_T_EQ(1, choices_getindex(&choices, 3)); // banana was at index 1

	PASS();
}

SUITE(choices_suite) {
	SET_SETUP(setup, NULL);
	SET_TEARDOWN(teardown, NULL);

	RUN_TEST(test_choices_empty);
	RUN_TEST(test_choices_1);
	RUN_TEST(test_choices_2);
	RUN_TEST(test_choices_without_search);
	RUN_TEST(test_choices_unicode);
	RUN_TEST(test_choices_large_input);
	RUN_TEST(test_choices_get_index);
}
