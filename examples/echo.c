#include <stdio.h>
#include <syscall.h>

int
main (int argc, char **argv)
{
  int i;

  for (i = 0; i < argc; i++)
    printf ("%s ", argv[i]);
  printf ("\n");
//  CHECK (create ("quux.dat", 0), "create quux.dat");
  return EXIT_SUCCESS;
}
