#include "libtweetlength.h"


static void
mentions (void)
{
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

#ifdef DEBUG
  g_setenv ("G_MESSAGES_DEBUG", "libtl", TRUE);
#endif

  g_test_add_func ("/entities/mentions", mentions);

  return g_test_run ();
}
