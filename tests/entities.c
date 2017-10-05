/*  This file is part of libtweetlength, a Gtk+ linux Twitter client.
 *  Copyright (C) 2017 Timm Bäder
 *
 *  libtweetlength is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  libtweetlength is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with libtweetlength.  If not, see <http://www.gnu.org/licenses/>.
 */

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

  entities = tl_extract_entities ("@12345", &n_entities, NULL);
  g_assert_cmpint (n_entities, ==, 1);
  g_assert_nonnull (entities);
  g_assert_cmpint (entities[0].type, ==, TL_ENT_MENTION);
  g_assert_cmpint (entities[0].start_character_index, ==, 0);
  g_assert_cmpint (entities[0].length_in_characters, ==, 6);

  g_free (entities);

  // According to [1], this should be a mention, but twitter.com disagrees...
  entities = tl_extract_entities ("の@usernameに到着を待っている", &n_entities, NULL);
  g_assert_cmpint (n_entities, ==, 0);
  g_assert_null (entities);

  g_free (entities);

  entities = tl_extract_entities ("abc@foo", &n_entities, NULL);
  g_assert_cmpint (n_entities, ==, 0);
  g_assert_null (entities);

  g_free (entities);

  entities = tl_extract_entities ("Current Status: @_@ (cc: @username)", &n_entities, NULL);
  g_assert_cmpint (n_entities, ==, 1);
  g_assert_nonnull (entities);
  g_assert_cmpint (entities[0].type, ==, TL_ENT_MENTION);
  g_assert_cmpint (entities[0].start_character_index, ==, 25);
  g_assert_cmpint (entities[0].length_in_characters, ==, 9);

  g_free (entities);

  entities = tl_extract_entities ("@username email me @test@example.com", &n_entities, NULL);
  g_assert_cmpint (n_entities, ==, 1);
  g_assert_nonnull (entities);
  g_assert_cmpint (entities[0].type, ==, TL_ENT_MENTION);
  g_assert_cmpint (entities[0].start_character_index, ==, 0);
  g_assert_cmpint (entities[0].length_in_characters, ==, 9);

  g_free (entities);



  // [1] https://github.com/twitter/twitter-text/blob/master/conformance/validate.yml
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
  gsize n_entities;
  TlEntity *entities;

  entities = tl_extract_entities ("twitter.com", &n_entities, NULL);
  g_assert_cmpint (n_entities, ==, 1);
  g_assert_nonnull (entities);
  g_assert_cmpint (entities[0].type, ==, TL_ENT_LINK);
  g_assert_cmpint (entities[0].start_character_index, ==, 0);
  g_assert_cmpint (entities[0].length_in_characters, ==, 11);

  g_free (entities);

  // Numbers at the start of domains are also valid - RFC 952 said no but RFC 1123 said yes
  // https://en.wikipedia.org/wiki/Hostname#Restrictions_on_valid_hostnames
  entities = tl_extract_entities ("000example.com", &n_entities, NULL);
  g_assert_cmpint (n_entities, ==, 1);
  g_assert_nonnull (entities);
  g_assert_cmpint (entities[0].type, ==, TL_ENT_LINK);
  g_assert_cmpint (entities[0].start_character_index, ==, 0);
  g_assert_cmpint (entities[0].length_in_characters, ==, 14);

  g_free (entities);

  // That also allows completely numeric domains (e.g. "123.[many-tlds]" is already registered)
  entities = tl_extract_entities ("123.com", &n_entities, NULL);
  g_assert_cmpint (n_entities, ==, 1);
  g_assert_nonnull (entities);
  g_assert_cmpint (entities[0].type, ==, TL_ENT_LINK);
  g_assert_cmpint (entities[0].start_character_index, ==, 0);
  g_assert_cmpint (entities[0].length_in_characters, ==, 7);

  g_free (entities);

  // But make sure that we don't allow numeric TLD
  entities = tl_extract_entities ("http://example.000", &n_entities, NULL);
  g_assert_cmpint (n_entities, ==, 0);
  g_assert_null (entities);

  g_free (entities);

  entities = tl_extract_entities ("twitter.c", &n_entities, NULL);
  g_assert_cmpint (n_entities, ==, 0);
  g_assert_null (entities);

  g_free (entities);

  entities = tl_extract_entities ("http://", &n_entities, NULL);
  g_assert_cmpint (n_entities, ==, 0);
  g_assert_null (entities);

  g_free (entities);

  entities = tl_extract_entities ("foobar.com::8080/foo.html", &n_entities, NULL);
  g_assert_cmpint (n_entities, ==, 1);
  g_assert_nonnull (entities);
  g_assert_cmpint (entities[0].type, ==, TL_ENT_LINK);
  g_assert_cmpint (entities[0].start_character_index, ==, 0);
  g_assert_cmpint (entities[0].length_in_characters, ==, 10);

  g_free (entities);

  entities = tl_extract_entities ("foo /twitter.com", &n_entities, NULL);
  g_assert_cmpint (n_entities, ==, 0);
  g_assert_null (entities);

  g_free (entities);

  entities = tl_extract_entities ("foo.商城", &n_entities, NULL);
  g_assert_cmpint (n_entities, ==, 1);
  g_assert_nonnull (entities);
  g_assert_cmpint (entities[0].start_character_index, ==, 0);
  g_assert_cmpint (entities[0].length_in_characters, ==, 6);

  g_free (entities);

  entities = tl_extract_entities ("foo.de http://foo.de", &n_entities, NULL);
  g_assert_cmpint (n_entities, ==, 1);
  g_assert_nonnull (entities);
  g_assert_cmpint (entities[0].start_character_index, ==, 7);

  g_free (entities);

  entities = tl_extract_entities ("http://foobar.co.uk", &n_entities, NULL);
  g_assert_cmpint (n_entities, ==, 1);
  g_assert_nonnull (entities);
  g_assert_cmpint (entities[0].start_character_index, ==, 0);

  g_free (entities);

  entities = tl_extract_entities ("foobar.uk", &n_entities, NULL);
  g_assert_cmpint (n_entities, ==, 0);
  g_assert_null (entities);

  // .co is a "special tld"
  entities = tl_extract_entities ("foobar.co", &n_entities, NULL);
  g_assert_cmpint (n_entities, ==, 1);
  g_assert_nonnull (entities);
  g_assert_cmpint (entities[0].start_character_index, ==, 0);
  g_assert_cmpint (entities[0].length_in_characters, ==, 9);

  g_free (entities);
}

static void
combined (void)
{
  gsize n_entities;
  TlEntity *entities;

  entities = tl_extract_entities ("twitter.com abc #foo def @zomg", &n_entities, NULL);
  g_assert_cmpint (n_entities, ==, 3);
  g_assert_nonnull (entities);
  g_assert_cmpint (entities[0].type, ==, TL_ENT_LINK);
  g_assert_cmpint (entities[0].start_character_index, ==, 0);
  g_assert_cmpint (entities[0].length_in_characters, ==, 11);
  g_assert_cmpint (entities[1].type, ==, TL_ENT_HASHTAG);
  g_assert_cmpint (entities[1].start_character_index, ==, 16);
  g_assert_cmpint (entities[1].length_in_characters, ==, 4);
  g_assert_cmpint (entities[2].type, ==, TL_ENT_MENTION);
  g_assert_cmpint (entities[2].start_character_index, ==, 25);
  g_assert_cmpint (entities[2].length_in_characters, ==, 5);

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
  g_test_add_func ("/entities/combined", combined);

  return g_test_run ();
}
