#include <iostream>

#define ENABLE_LOG

#include "Node.h"

using namespace node;
int help (int, char **, Node *);

int
echo (int count, char **args, Node * n)
{
  char *buffer = (char *) malloc (256);	//FIXME
  for (int i = 0; i < count; i++)
    {
      strcat (buffer, args[i]);
    }
  n->writeln (buffer);
  return 0;
}


Node::job jobs[] =
{
  {
  "help", "prints this help screen", help}
  ,
  {
  "echo", "echoes the message back", echo}
};

int
  jlen = sizeof (jobs) / sizeof (Node::job);


int
help (int count, char **args, Node * n)
{
  char *
    buf = (char *) malloc (2048);
  for (int i = 0; i < jlen; i++)
    {
      char *
	buffer = (char *) malloc (2048);
      sprintf (buffer, "%s\t - %s\n", jobs[i].command, jobs[i].info);
      strcat (buf, buffer);
      for (int j = 0; j < jobs[i].opt_length; j++)
	{
	  sprintf (buffer, "  |--%s[-%c] - %s\n", jobs[i].options[j].word,
		   jobs[i].options[j].letter, jobs[i].options[j].description);
	  strcat (buf, buffer);
	}
    }
  n->writeln (buf);
  return 0;
}

int
handle_client (Node * client)
{
  printf ("Client connected\n");
  while (client->process (jlen, jobs, client->readln ()) != -1)
    {
    }
  return 0;
}

int
main (int argc, char *argv[])
{
  Node *
    n = new Node ();
  int
    pid;
  while (1)
    {				//accept infinite connections
      Node *
	client = n->accept (7300);
      if (client == NULL)
	{
	  printf ("Internal Error\n");
	  return -1;
	}
      pid = fork ();
      if (pid == 0)
	{
	  //child
	  delete
	    n;
	  return handle_client (client);
	}
      else
	{
	  //parent
	  if (pid == -1)
	    {
	      printf ("fork failed\n");
	      return -1;
	    }
	  else
	    {
	      delete
		client;
	    }
	}
    }
  return 0;
}
