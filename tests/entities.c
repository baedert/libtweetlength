#include "libtweetlength.h"


static void
mentions (void)
{
  gsize text_length;
  gsize n_entities;
  TlEntity *entities;

  entities = tl_extract_entities ("@foobar", &n_entities, &text_length);
  g_assert_cmpint (n_entities, ==, 1);
  g_assert_cmpint (text_length, ==, 7);
  g_assert_nonnull (entities);
  g_assert_cmpint (entities[0].type, ==, TL_ENT_MENTION);
  g_assert_cmpint (entities[0].start_character_index, ==, 0);
  g_assert_cmpint (entities[0].length_in_characters, ==, 7);

  g_free (entities);

  entities = tl_extract_entities ("@_foobar", &n_entities, &text_length);
  g_assert_cmpint (n_entities, ==, 1);
  g_assert_cmpint (text_length, ==, 8);
  g_assert_nonnull (entities);
  g_assert_cmpint (entities[0].type, ==, TL_ENT_MENTION);
  g_assert_cmpint (entities[0].start_character_index, ==, 0);
  g_assert_cmpint (entities[0].length_in_characters, ==, 8);

  g_free (entities);

  entities = tl_extract_entities ("@foobar?", &n_entities, &text_length);
  g_assert_cmpint (n_entities, ==, 1);
  g_assert_cmpint (text_length, ==, 8);
  g_assert_nonnull (entities);
  g_assert_cmpint (entities[0].type, ==, TL_ENT_MENTION);
  g_assert_cmpint (entities[0].start_character_index, ==, 0);
  g_assert_cmpint (entities[0].length_in_characters, ==, 7);

  g_free (entities);

  entities = tl_extract_entities ("@foobar-bla", &n_entities, &text_length);
  g_assert_cmpint (n_entities, ==, 1);
  g_assert_cmpint (text_length, ==, 11);
  g_assert_nonnull (entities);
  g_assert_cmpint (entities[0].type, ==, TL_ENT_MENTION);
  g_assert_cmpint (entities[0].start_character_index, ==, 0);
  g_assert_cmpint (entities[0].length_in_characters, ==, 7);

  g_free (entities);

  entities = tl_extract_entities ("aaa @foobar-bla", &n_entities, &text_length);
  g_assert_cmpint (n_entities, ==, 1);
  g_assert_cmpint (text_length, ==, 15);
  g_assert_nonnull (entities);
  g_assert_cmpint (entities[0].type, ==, TL_ENT_MENTION);
  g_assert_cmpint (entities[0].start_character_index, ==, 4);
  g_assert_cmpint (entities[0].length_in_characters, ==, 7);

  g_free (entities);
}

static void
hashtags (void)
{
  gsize text_length;
  gsize n_entities;
  TlEntity *entities;

  entities = tl_extract_entities ("#foobar", &n_entities, &text_length);
  g_assert_cmpint (n_entities, ==, 1);
  g_assert_cmpint (text_length, ==, 7);
  g_assert_nonnull (entities);
  g_assert_cmpint (entities[0].type, ==, TL_ENT_HASHTAG);
  g_assert_cmpint (entities[0].start_character_index, ==, 0);
  g_assert_cmpint (entities[0].length_in_characters, ==, 7);

  g_free (entities);
}

static void
links (void)
{
  gsize text_length;
  gsize n_entities;
  TlEntity *entities;

  entities = tl_extract_entities ("twitter.com", &n_entities, &text_length);
  g_assert_cmpint (n_entities, ==, 1);
  g_assert_cmpint (text_length, ==, 23);
  g_assert_nonnull (entities);
  g_assert_cmpint (entities[0].type, ==, TL_ENT_LINK);
  g_assert_cmpint (entities[0].start_character_index, ==, 0);
  g_assert_cmpint (entities[0].length_in_characters, ==, 11);

  g_free (entities);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

#ifdef DEBUG
  g_setenv ("G_MESSAGES_DEBUG", "libtl", TRUE);
#endif

  g_test_add_func ("/entities/mentions", mentions);
  g_test_add_func ("/entities/hashtags", hashtags);
  g_test_add_func ("/entities/links", links);

  return g_test_run ();
}
