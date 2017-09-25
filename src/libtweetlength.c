#include "libtweetlength.h"
#include <string.h>

#define LINK_LENGTH 23

static const char *TLDS[] = {
  "ly", "io",
  "com",  "net",  "org",    "xxx",  "sexy", "pro",
  "biz",  "name", "info",   "arpa", "gov",  "aero",
  "asia", "cat",  "coop",   "edu",  "int",  "jobs",
  "mil",  "mobi", "museum", "post", "tel",  "travel"
};
#define MAX_TLD_LENGTH 6 // Keep this up to date when changing TLDS!


enum {
  ENT_TEXT = 1,
  ENT_HASHTAG,
  ENT_LINK,
  ENT_MENTION,
  ENT_WHITESPACE,
};

typedef struct {
  guint type;
  const char *start;
  gsize length_in_bytes;
} Token;

typedef struct {
  guint type;
  const char *start;
  gsize length_in_bytes;
} Entity;

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

static inline Token *
emplace_token (GArray     *array,
               const char *token_start,
               gsize       token_length)
{
  Token *t;

  g_array_set_size (array, array->len + 1);
  t = &g_array_index (array, Token, array->len - 1);

  t->type = token_type_from_char (token_start[0]);
  t->start = token_start;
  t->length_in_bytes = token_length;

  return t;
}

static inline Entity *
emplace_entity (GArray     *array,
                guint       entity_type,
                const char *entity_start,
                gsize       entity_length)
{
  Entity *e;

  g_array_set_size (array, array->len + 1);
  e = &g_array_index (array, Entity, array->len - 1);

  e->type = entity_type;
  e->start = entity_start;
  e->length_in_bytes = entity_length;

  return e;
}

static inline gboolean
token_is_tld (const Token *t)
{
  guint i;

  if (t->length_in_bytes > MAX_TLD_LENGTH)
    return FALSE;

  for (i = 0; i < G_N_ELEMENTS (TLDS); i ++) {
    if (strncasecmp (t->start, TLDS[i], t->length_in_bytes) == 0) {
      return TRUE;
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
char_splits (guchar c)
{
  switch(c) {
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
entity_length_in_characters (const Entity *e)
{
  switch (e->type) {
    case ENT_LINK:
      return LINK_LENGTH;

    default:
      return g_utf8_strlen (e->start, e->length_in_bytes);
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

  while (p - input < length_in_bytes) {
    const char *cur_start = p;
    gunichar cur_char = g_utf8_get_char (p);
    gsize cur_length = 0;

    /* If this char already splits, it's a one-char token */
    if (char_splits (cur_char)) {
      const char *old_p = p;
      p = g_utf8_next_char (p);
      emplace_token (tokens, cur_start, p - old_p);
      continue;
    }

    do {
      const char *old_p = p;
      p = g_utf8_next_char (p);
      cur_char = g_utf8_get_char (p);
      cur_length += p - old_p;
    } while (!char_splits (cur_char) && p - input < length_in_bytes);

    emplace_token (tokens, cur_start, cur_length);
  }

  return g_steal_pointer (&tokens);
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

  // TODO: There are probably more protocols supported...
  t = &tokens[i];

  if (t->type != TOK_TEXT) {
    return FALSE;
  }

  if (token_is_protocol (t)) {
    // These are all optional!
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
    i += 2; // Skip to token after second slash
  }

  // Now read until .TLD
  guint dot_index = i;
  while (dot_index < n_tokens - 1) { // -1 so we can do +1 in the loop body!
    // TODO: We just skip all of these here, since we need it for username/port links.
    //       There are probably not all supported though, like "::" inside a url...
    if (tokens[dot_index].type != TOK_TEXT &&
        tokens[dot_index].type != TOK_DOT &&
        tokens[dot_index].type != TOK_COLON &&
        tokens[dot_index].type != TOK_NUMBER &&
        tokens[dot_index].type != TOK_AT) {
      return FALSE;
    }

    // The dot we look for is followed by a tld identifier such as "com"
    if (tokens[dot_index].type == TOK_DOT &&
        tokens[dot_index + 1].type == TOK_TEXT &&
        token_is_tld (&tokens[dot_index + 1])) {
      break;
    }
    dot_index ++;
  }
  g_debug ("dot index: %u", dot_index);
  g_debug ("n_tokens: %u", (guint)n_tokens);

  if (dot_index == n_tokens - 1) {
    return FALSE;
  }

  i = dot_index + 1;

  // If the next token is a colon, we are reading a port
  if (tokens[i + 1].type == TOK_COLON) {
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

      if (i == n_tokens - 1) {
        // The last token (slash or questionmark) was the last one!
        if (tokens[i].type == TOK_QUESTIONMARK) {
          i --; // Trailing questionmark is not part of the link...
        }

      } else {
        // There is more at the end!
        while (i < n_tokens - 1 && tokens[i].type != TOK_WHITESPACE) {
          i ++;
        }

      }
    }
  }

  end_token = i;
  g_assert (end_token < n_tokens);

  // Simply add up all the lengths
  gsize length_in_bytes = 0;
  const char *first_byte = tokens[start_token].start;
  for (i = start_token; i <= end_token; i ++) {
    length_in_bytes += tokens[i].length_in_bytes;
  }

  emplace_entity (entities, ENT_LINK, first_byte, length_in_bytes);

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
       gsize        n_tokens)
{
  GArray *entities = g_array_new (FALSE, TRUE, sizeof (Entity));
  guint i = 0;

  while (i < n_tokens) {
    const Token *token = &tokens[i];

    // We always have to do this since links can begin with whatever word
    if (parse_link (entities, tokens, n_tokens, &i)) {
      continue;
    }

    emplace_entity (entities, ENT_TEXT, token->start, token->length_in_bytes);

    i ++;
  }

  return entities;
}

static gsize
count_entities_in_characters (GArray *entities)
{
  guint i;
  gsize sum = 0;

  for (i = 0; i < entities->len; i ++) {
    const Entity *e = &g_array_index (entities, Entity, i);

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
  /*for (guint i = 0; i < tokens->len; i ++) {*/
    /*const Token *t = &g_array_index (tokens, Token, i);*/
    /*g_debug ("Token %u: Type: %d, Length: %u, Text:%.*s", i, t->type, (guint)t->length_in_bytes,*/
               /*(int)t->length_in_bytes, t->start);*/
  /*}*/

  n_tokens = tokens->len;
  token_array = (const Token *)g_array_free (tokens, FALSE);

  entities = parse (token_array, n_tokens);
  /*for (guint i = 0; i < entities->len; i ++) {*/
    /*const Entity *e = &g_array_index (entities, Entity, i);*/
    /*g_debug ("Entity %u: Text: '%.*s', Type: %u, Bytes: %u, Length: %u", i, (int)e->length_in_bytes, e->start,*/
               /*e->type, (guint)e->length_in_bytes, (guint)entity_length_in_characters (e));*/
  /*}*/

  length = count_entities_in_characters (entities);
  g_array_free (entities, TRUE);
  g_free ((char *)token_array);

  return length;
}
