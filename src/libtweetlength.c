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
#include "data.h"
#include <string.h>

#define LINK_LENGTH 23

typedef struct {
  guint type;
  const char *start;
  gsize start_character_index;
  gsize length_in_bytes;
  gsize length_in_characters;
} Token;

enum {
  TOK_TEXT = 1,
  TOK_NUMBER,
  TOK_WHITESPACE,
  TOK_COLON,
  TOK_SLASH,
  TOK_OPEN_PAREN,
  TOK_CLOSE_PAREN,
  TOK_QUESTIONMARK,
  TOK_DOT,
  TOK_HASH,
  TOK_AT,
  TOK_EQUALS,
  TOK_DASH,
};

static inline guint
token_type_from_char (gunichar c)
{
  switch (c) {
    case '@':
      return TOK_AT;
    case '#':
      return TOK_HASH;
    case ':':
      return TOK_COLON;
    case '/':
      return TOK_SLASH;
    case '(':
      return TOK_OPEN_PAREN;
    case ')':
      return TOK_CLOSE_PAREN;
    case '.':
      return TOK_DOT;
    case '?':
      return TOK_QUESTIONMARK;
    case '=':
      return TOK_EQUALS;
    case '-':
      return TOK_DASH;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      return TOK_NUMBER;
    case ' ':
    case '\n':
    case '\t':
      return TOK_WHITESPACE;

    default:
    return TOK_TEXT;
  }
}

static inline gboolean
token_in (const Token *t,
          const char  *haystack)
{
  if (t->length_in_bytes > 1) {
    return FALSE;
  }

  const int haystack_len = strlen (haystack);
  int i;

  for (i = 0; i < haystack_len; i ++) {
    if (haystack[i] == t->start[0]) {
      return TRUE;
    }
  }

  return FALSE;
}


static inline void
emplace_token (GArray     *array,
               const char *token_start,
               gsize       token_length,
               gsize       start_character_index,
               gsize       length_in_characters)
{
  Token *t;

  g_array_set_size (array, array->len + 1);
  t = &g_array_index (array, Token, array->len - 1);

  t->type = token_type_from_char (token_start[0]);
  t->start = token_start;
  t->length_in_bytes = token_length;
  t->start_character_index = start_character_index;
  t->length_in_characters = length_in_characters;
}

static inline void
emplace_entity (GArray     *array,
                guint       entity_type,
                const char *entity_start,
                gsize       entity_length_in_bytes,
                gsize       entity_character_start,
                gsize       entity_length_in_characters)
{
  TlEntity *e;

  g_array_set_size (array, array->len + 1);
  e = &g_array_index (array, TlEntity, array->len - 1);

  e->type = entity_type;
  e->start = entity_start;
  e->length_in_bytes = entity_length_in_bytes;
  e->start_character_index = entity_character_start;
  e->length_in_characters = entity_length_in_characters;
}

static inline gboolean
token_is_tld (const Token *t,
              gboolean     has_protocol)
{
  guint i;

  if (t->length_in_characters > GTLDS[G_N_ELEMENTS (GTLDS) - 1].length) {
    return FALSE;
  }

  for (i = 0; i < G_N_ELEMENTS (GTLDS); i ++) {
    if (t->length_in_characters == GTLDS[i].length &&
        strncasecmp (t->start, GTLDS[i].str, t->length_in_bytes) == 0) {
      return TRUE;
    }
  }

  for (i = 0; i < G_N_ELEMENTS (SPECIAL_CCTLDS); i ++) {
    if (t->length_in_characters == SPECIAL_CCTLDS[i].length &&
        strncasecmp (t->start, SPECIAL_CCTLDS[i].str, t->length_in_bytes) == 0) {
      return TRUE;
    }
  }

  if (has_protocol) {
    for (i = 0; i < G_N_ELEMENTS (CCTLDS); i ++) {
      if (t->length_in_characters == CCTLDS[i].length &&
          strncasecmp (t->start, CCTLDS[i].str, t->length_in_bytes) == 0) {
        return TRUE;
      }
    }
  }

  return FALSE;
}

static inline gboolean
token_is_protocol (const Token *t)
{
  // TODO: We probably support more than just http and https?

  if (t->type != TOK_TEXT) {
    return FALSE;
  }

  if (t->length_in_bytes != 4 && t->length_in_bytes != 5) {
    return FALSE;
  }

  return strncasecmp (t->start, "http", t->length_in_bytes) == 0 ||
         strncasecmp (t->start, "https", t->length_in_bytes) == 0;
}

static inline gboolean
char_splits (gunichar c)
{
  switch (c) {
    case ',':
    case '.':
    case '/':
    case '?':
    case '(':
    case ')':
    case ':':
    case ';':
    case '=':
    case '@':
    case '#':
    case '-':
    case '\n':
    case '\t':
    case '\0':
    case ' ':
      return TRUE;
    default:
      return FALSE;
  }

  return FALSE;
}

static inline gsize
entity_length_in_characters (const TlEntity *e)
{
  switch (e->type) {
    case TL_ENT_LINK:
      return LINK_LENGTH;

    default:
      return e->length_in_characters;
  }
}

/*
 * tokenize:
 *
 * Returns: (transfer full): Tokens
 */
static GArray *
tokenize (const char *input,
          gsize       length_in_bytes)
{
  GArray *tokens = g_array_new (FALSE, TRUE, sizeof (Token));
  const char *p = input;
  gsize cur_character_index = 0;

  while (p - input < length_in_bytes) {
    const char *cur_start = p;
    gunichar cur_char = g_utf8_get_char (p);
    gsize cur_length = 0;
    gsize length_in_chars = 0;

    /* If this char already splits, it's a one-char token */
    if (char_splits (cur_char)) {
      const char *old_p = p;
      p = g_utf8_next_char (p);
      emplace_token (tokens, cur_start, p - old_p, cur_character_index, 1);
      cur_character_index ++;
      continue;
    }

    do {
      const char *old_p = p;
      p = g_utf8_next_char (p);
      cur_char = g_utf8_get_char (p);
      cur_length += p - old_p;
      length_in_chars ++;
    } while (!char_splits (cur_char) && p - input < length_in_bytes);

    emplace_token (tokens, cur_start, cur_length, cur_character_index, length_in_chars);

    cur_character_index += length_in_chars;
  }

  return g_steal_pointer (&tokens);
}

static gboolean
parse_link_tail (GArray      *entities,
                 const Token *tokens,
                 gsize        n_tokens,
                 guint       *current_position)
{
  guint i = *current_position;
  const Token *t;

  g_debug ("--------");
  g_debug ("n_tokens; %u", (guint) n_tokens);

  gsize paren_level = 0;
  int first_paren_index = -1;
  for (;;) {
    t = &tokens[i];

    if (t->type == TOK_WHITESPACE) {
      i --;
      g_debug("Found whitespace - backtracked one to %d", i);
      break;
    }

    g_debug ("Token %u: Type: %d, Length: %u, Text:%.*s", i, t->type, (guint)t->length_in_bytes,
         (int)t->length_in_bytes, t->start);
    if (tokens[i].type == TOK_OPEN_PAREN) {

      if (first_paren_index == -1) {
        first_paren_index = i;
        g_debug ("First paren index: %d", (int)first_paren_index);
      }
      paren_level ++;
      if (paren_level == 3) {
        break;
      }
    } else if (tokens[i].type == TOK_CLOSE_PAREN) {
      if (first_paren_index == -1) {
        first_paren_index = i;
        g_debug ("First paren index: %d", (int)first_paren_index);
      }
      g_debug ("Close paren");
      paren_level --;
    }

    i ++;

    if (i == n_tokens) {
      i --;
      break;
    }

    g_debug ("i now: %u", i);
  }

  g_debug ("After i: %u", i);
  g_debug ("paren level: %d", (int)paren_level);
  if (paren_level != 0) {
    g_assert (first_paren_index != -1);
    i = first_paren_index - 1; // Before that paren
  }

  t = &tokens[i];
  /* Whatever happened, don't count trailing punctuation */
  if (t->type == TOK_QUESTIONMARK) {
    // TODO: We should probably have a more generic way of identifying "punctuation"
    i --;
  }


  *current_position = i;

  return TRUE;
}


// Returns whether a link has been parsed or not.
static gboolean
parse_link (GArray      *entities,
            const Token *tokens,
            gsize        n_tokens,
            guint       *current_position)
{
  guint i = *current_position;
  const Token *t;
  guint start_token = *current_position;
  guint end_token;
  gboolean has_protocol = FALSE;

  t = &tokens[i];

  if (token_is_protocol (t)) {
    // need "://" now.
    t = &tokens[i + 1];
    if (t->type != TOK_COLON) {
      return FALSE;
    }
    i ++;

    t = &tokens[i + 1];
    if (t->type != TOK_SLASH) {
      return FALSE;
    }
    i ++;

    t = &tokens[i + 1];
    if (t->type != TOK_SLASH) {
      return FALSE;
    }
    // If we are at the end now, this is not a link, just the protocol.
    if (i + 1 == n_tokens - 1) {
      return FALSE;
    }
    i += 2; // Skip to token after second slash
    has_protocol = TRUE;
  } else {
    // Lookbehind: Token before may not be an @, they are not supported.
    // TODO: There's a list of valid before-url chars and we should just use that here.
    if (i > 0 &&
        (tokens[i - 1].type == TOK_AT ||
         tokens[i - 1].type == TOK_DOT ||
         tokens[i - 1].type == TOK_SLASH ||
         tokens[i - 1].type == TOK_DASH)) {
      return FALSE;
    }
  }

  // Make sure that we have the first part of a domain
  if (tokens[i].type != TOK_TEXT &&
      tokens[i].type != TOK_NUMBER) {
        return FALSE;
  }

  guint scan_position = i;
  guint tld_index = 0;

  g_debug ("Looking for TLD starting from %u", scan_position);

  // Now read until .TLD
  // We're stepping two at a time to look for the repeating dot and then text/number
  // As we're looking ahead then we need to stop two items before the end
  while (scan_position < n_tokens - 2) {
    g_debug ("Trying token %u and %u", scan_position + 1, scan_position + 2);
    if (tokens[scan_position + 1].type == TOK_DOT &&
         (tokens[scan_position + 2].type == TOK_TEXT ||
          tokens[scan_position + 2].type == TOK_NUMBER)) {
      scan_position += 2;

      if (token_is_tld (&tokens[scan_position], has_protocol)) {
        tld_index = scan_position;
      }
    } else {
      break;
    }
  }
  g_debug ("tld_index: %u", tld_index);

  if (tld_index != scan_position ||
      tld_index == 0) {
    return FALSE;
  }

  i = tld_index;

  // If the next token is a colon, we are reading a port
  if (i < n_tokens - 1 && tokens[i + 1].type == TOK_COLON) {
    i ++; // i == COLON
    if (tokens[i + 1].type != TOK_NUMBER) {
      // According to twitter.com, the link reaches until before the COLON
      i --;
    } else {
      // Skip port number
      i ++;
    }
  }

  // To continue a link, the next token must be a slash or a question mark.
  // If it isn't, we stop here.
  if (i < n_tokens - 1) {
    // A trailing slash is part of the link, other punctuation is not.
    if (tokens[i + 1].type == TOK_SLASH ||
        tokens[i + 1].type == TOK_QUESTIONMARK) {
      i ++;

      if (i < n_tokens - 1) {
        if (!parse_link_tail (entities, tokens, n_tokens, &i)) {
          return FALSE;
        }
      } else if (tokens[i].type == TOK_QUESTIONMARK) {
        // Trailing questionmark is not part of the link
        i --;
      }
    }
  }

  g_debug ("end_token = i = %u", i);
  end_token = i;
  g_assert (end_token < n_tokens);

  // Simply add up all the lengths
  gsize length_in_bytes = 0;
  gsize length_in_characters = 0;
  const char *first_byte = tokens[start_token].start;
  for (i = start_token; i <= end_token; i ++) {
    length_in_bytes += tokens[i].length_in_bytes;
    length_in_characters += tokens[i].length_in_characters;
  }

  emplace_entity (entities,
                  TL_ENT_LINK,
                  first_byte,
                  length_in_bytes,
                  tokens[start_token].start_character_index,
                  length_in_characters);

  *current_position = end_token + 1; // Hop to the next token!

  return TRUE;
}

static gboolean
parse_mention (GArray      *entities,
               const Token *tokens,
               gsize        n_tokens,
               guint       *current_position)
{
  const Token *t;
  gsize i = *current_position;
  guint start_token;
  guint end_token;

  t = &tokens[i];
  g_assert (t->type == TOK_AT);
  start_token = i;

  // Lookback at the previous token. If it was a text token
  // without whitespace between, this is not going to be a mention...
  if (i > 0 && tokens[i - 1].type == TOK_TEXT) {
    return FALSE;
  }

  //skip @
  i ++;
  t = &tokens[i];
  if (t->type != TOK_TEXT &&
      t->type != TOK_NUMBER) {
    return FALSE;
  }

  // Mentions ending in an '@' are no mentions, e.g. @_@
  if (i < n_tokens - 1 &&
      tokens[i + 1].type == TOK_AT) {
    return FALSE;
  }

  end_token = i;
  g_assert (end_token < n_tokens);

  // Simply add up all the lengths
  gsize length_in_bytes = 0;
  gsize length_in_characters = 0;
  const char *first_byte = tokens[start_token].start;
  for (i = start_token; i <= end_token; i ++) {
    length_in_bytes += tokens[i].length_in_bytes;
    length_in_characters += tokens[i].length_in_characters;
  }

  emplace_entity (entities,
                  TL_ENT_MENTION,
                  first_byte,
                  length_in_bytes,
                  tokens[start_token].start_character_index,
                  length_in_characters);

  *current_position = end_token + 1; // Hop to the next token!

  return TRUE;
}

static gboolean
parse_hashtag (GArray      *entities,
               const Token *tokens,
               gsize        n_tokens,
               guint       *current_position)
{
  const Token *t;
  gsize i = *current_position;
  guint start_token;
  guint end_token;

  t = &tokens[i];
  g_assert (t->type == TOK_HASH);
  start_token = i;


  //skip #
  i ++;
  t = &tokens[i];
  if (t->type != TOK_TEXT) {
    return FALSE;
  }

  end_token = i;
  g_assert (end_token < n_tokens);

  // Simply add up all the lengths
  gsize length_in_bytes = 0;
  gsize length_in_characters = 0;
  const char *first_byte = tokens[start_token].start;
  for (i = start_token; i <= end_token; i ++) {
    length_in_bytes += tokens[i].length_in_bytes;
    length_in_characters += tokens[i].length_in_characters;
  }

  emplace_entity (entities,
                  TL_ENT_HASHTAG,
                  first_byte,
                  length_in_bytes,
                  tokens[start_token].start_character_index,
                  length_in_characters);

  *current_position = end_token + 1; // Hop to the next token!

  return TRUE;
}

/*
 * parse:
 *
 * Returns: (transfer full): list of tokens
 */
static GArray *
parse (const Token *tokens,
       gsize        n_tokens,
       guint       *n_relevant_entities)
{
  GArray *entities = g_array_new (FALSE, TRUE, sizeof (TlEntity));
  guint i = 0;
  guint relevant_entities = 0;

  while (i < n_tokens) {
    const Token *token = &tokens[i];

    // We always have to do this since links can begin with whatever word
    if (parse_link (entities, tokens, n_tokens, &i)) {
      relevant_entities ++;
      continue;
    }

    switch (token->type) {
      case TOK_AT:
        if (parse_mention (entities, tokens, n_tokens, &i)) {
          relevant_entities ++;
          continue;
        }
      break;

      case TOK_HASH:
        if (parse_hashtag (entities, tokens, n_tokens, &i)) {
          relevant_entities ++;
          continue;
        }
      break;
    }

    emplace_entity (entities,
                    TL_ENT_TEXT,
                    token->start,
                    token->length_in_bytes,
                    token->start_character_index,
                    token->length_in_characters);

    i ++;
  }

  if (n_relevant_entities) {
    *n_relevant_entities = relevant_entities;
  }

  return entities;
}

static gsize
count_entities_in_characters (GArray *entities)
{
  guint i;
  gsize sum = 0;

  for (i = 0; i < entities->len; i ++) {
    const TlEntity *e = &g_array_index (entities, TlEntity, i);

    sum += entity_length_in_characters (e);
  }

  return sum;
}

/*
 * tl_count_chars:
 * input: (nullable): Tweet text
 *
 */
gsize
tl_count_characters (const char *input)
{
  if (input == NULL || input[0] == '\0') {
    return 0;
  }

  return tl_count_characters_n (input, strlen (input));
}

/*
 * tl_count_characters_n:
 * input: (nullable): Text to measure
 * length_in_bytes: Length of @input, in bytes.
 *
 * TODO: We might want to do a g_utf8_make_valid or at least _validate.
 */
gsize
tl_count_characters_n (const char *input,
                       gsize       length_in_bytes)
{
  GArray *tokens;
  const Token *token_array;
  gsize n_tokens;
  GArray *entities;
  gsize length;

  if (input == NULL || input[0] == '\0') {
    return 0;
  }

  // From here on, input/length_in_bytes are trusted to be OK

  g_debug ("------- INPUT: %s %p (Bytes: %u)-------", input, input, (guint) length_in_bytes); // XXX Expected to be NUL-terminated
  tokens = tokenize (input, length_in_bytes);
  for (guint i = 0; i < tokens->len; i ++) {
    const Token *t = &g_array_index (tokens, Token, i);
    g_debug ("Token %u: Type: %d, Length: %u, Text:%.*s", i, t->type, (guint)t->length_in_bytes,
               (int)t->length_in_bytes, t->start);
  }

  n_tokens = tokens->len;
  token_array = (const Token *)g_array_free (tokens, FALSE);

  entities = parse (token_array, n_tokens, NULL);
  for (guint i = 0; i < entities->len; i ++) {
    const TlEntity *e = &g_array_index (entities, TlEntity, i);
    g_debug ("TlEntity %u: Text: '%.*s', Type: %u, Bytes: %u, Length: %u", i, (int)e->length_in_bytes, e->start,
               e->type, (guint)e->length_in_bytes, (guint)entity_length_in_characters (e));
  }

  length = count_entities_in_characters (entities);
  g_array_free (entities, TRUE);
  g_free ((char *)token_array);

  return length;
}

/**
 * tl_extract_entities:
 * @input: The input text to extract entities from
 * @out_n_entities: (out):
 * @out_text_length: (out) (optional):
 */
TlEntity *
tl_extract_entities (const char *input,
                     gsize      *out_n_entities,
                     gsize      *out_text_length)
{
  gsize dummy;

  g_return_val_if_fail (out_n_entities != NULL, NULL);

  if (input == NULL || input[0] == '\0') {
    *out_n_entities = 0;
    return NULL;
  }

  if (out_text_length == NULL) {
    out_text_length = &dummy;
  }

  return tl_extract_entities_n (input, strlen (input), out_n_entities, out_text_length);
}

/**
 * tl_extract_entities:
 * @input: The input text to extract entities from
 * @length_in_bytes: length of @input
 * @out_n_entities: (out):
 * @out_text_length: (out) (optional):
 */
TlEntity *
tl_extract_entities_n (const char *input,
                       gsize       length_in_bytes,
                       gsize      *out_n_entities,
                       gsize      *out_text_length)
{
  GArray *tokens;
  const Token *token_array;
  gsize n_tokens;
  GArray *entities;
  gsize dummy;
  guint n_relevant_entities;
  TlEntity *result_entities;
  guint result_index = 0;

  g_return_val_if_fail (out_n_entities != NULL, NULL);

  if (input == NULL || input[0] == '\0') {
    return 0;
  }
  g_debug ("------- INPUT: %s %p (Bytes: %u)-------", input, input, (guint) length_in_bytes); // XXX Expected to be NUL-terminated

  if (out_text_length == NULL) {
    out_text_length = &dummy;
  }

  tokens = tokenize (input, length_in_bytes);

  for (guint i = 0; i < tokens->len; i ++) {
    const Token *t = &g_array_index (tokens, Token, i);
    g_debug ("Token %u: Type: %d, Length: %u, Text:%.*s, start char: %u, chars: %u", i, t->type, (guint)t->length_in_bytes,
               (int)t->length_in_bytes, t->start, (guint)t->start_character_index, (guint)t->length_in_characters);
  }

  n_tokens = tokens->len;
  token_array = (const Token *)g_array_free (tokens, FALSE);
  entities = parse (token_array, n_tokens, &n_relevant_entities);

  *out_text_length = count_entities_in_characters (entities);
  g_free ((char *)token_array);

  for (guint i = 0; i < entities->len; i ++) {
    const TlEntity *e = &g_array_index (entities, TlEntity, i);
    g_debug ("TlEntity %u: Text: '%.*s', Type: %u, Bytes: %u, Length: %u, start character: %u", i, (int)e->length_in_bytes, e->start,
               e->type, (guint)e->length_in_bytes, (guint)entity_length_in_characters (e), (guint)e->start_character_index);
  }

  // Only pass mentions, hashtags and links out
  result_entities = g_malloc (sizeof (TlEntity) * n_relevant_entities);
  for (guint i = 0; i < entities->len; i ++) {
    const TlEntity *e = &g_array_index (entities, TlEntity, i);
    switch (e->type) {
      case TL_ENT_LINK:
      case TL_ENT_HASHTAG:
      case TL_ENT_MENTION:
        memcpy (&result_entities[result_index], e, sizeof (TlEntity));
        result_index ++;
      break;

      default: {}
    }
  }

  *out_n_entities = n_relevant_entities;
  g_array_free (entities, TRUE);

  return result_entities;
}
