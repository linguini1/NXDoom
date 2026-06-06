//
// Copyright(C) 2005-2014 Simon Howard
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//

// Code for invoking Doom

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>

#include <sys/wait.h>
#include <unistd.h>

#include "textscreen.h"

#include "config.h"
#include "execute.h"
#include "m_argv.h"
#include "m_config.h"
#include "m_misc.h"
#include "mode.h"

struct execute_context_s
{
  char *response_file;
  FILE *stream;
};

// Returns the path to a temporary file of the given name, stored
// inside the system temporary directory.

static char *TempFile(const char *s)
{
  const char *tempdir;

  // Check the $TMPDIR environment variable to find the location.

  tempdir = getenv("TMPDIR");

  if (tempdir == NULL)
    {
      tempdir = "/tmp";
    }

  return m_string_join(tempdir, DIR_SEPARATOR_S, s, NULL);
}

static int ArgumentNeedsEscape(const char *arg)
{
  const char *p;

  for (p = arg; *p != '\0'; ++p)
    {
      if (isspace(*p))
        {
          return 1;
        }
    }

  return 0;
}

// Arguments passed to the setup tool should be passed through to the
// game when launching a game.  Calling this adds all arguments from
// myargv to the output context.

void PassThroughArguments(execute_context_t *context)
{
  int i;

  for (i = 1; i < myargc; ++i)
    {
      if (ArgumentNeedsEscape(myargv[i]))
        {
          AddCmdLineParameter(context, "\"%s\"", myargv[i]);
        }
      else
        {
          AddCmdLineParameter(context, "%s", myargv[i]);
        }
    }
}

execute_context_t *NewExecuteContext(void)
{
  execute_context_t *result;

  result = malloc(sizeof(execute_context_t));

  result->response_file = TempFile("chocolat.rsp");
  result->stream = fopen(result->response_file, "w");

  if (result->stream == NULL)
    {
      fprintf(stderr, "Error opening response file\n");
      exit(-1);
    }

  return result;
}

void AddCmdLineParameter(execute_context_t *context, const char *s, ...)
{
  va_list args;

  va_start(args, s);

  vfprintf(context->stream, s, args);
  fprintf(context->stream, "\n");

  va_end(args);
}

boolean open_folder(const char *path)
{
#if 0
    char *cmd;
    int result;

    cmd = m_string_join("xdg-open \"", path, "\"", NULL);
    result = system(cmd);
    free(cmd);

    return result == 0;
#endif
  return 0;
}

// Given the specified program name, get the full path to the program,
// assuming that it is in the same directory as this program is.

static char *GetFullExePath(const char *program)
{
  char *result;
  char *sep;
  size_t result_len;
  unsigned int path_len;

  sep = strrchr(myargv[0], DIR_SEPARATOR);

  if (sep == NULL)
    {
      result = m_string_duplicate(program);
    }
  else
    {
      path_len = sep - myargv[0] + 1;
      result_len = strlen(program) + path_len + 1;
      result = malloc(result_len);

      m_str_copy(result, myargv[0], result_len);
      result[path_len] = '\0';

      m_string_concat(result, program, result_len);
    }

  return result;
}

static int ExecuteCommand(const char *program, const char *arg)
{
  pid_t childpid;
  int result;
  const char *argv[3];

  childpid = fork();

  if (childpid == 0)
    {
      // This is the child.  Execute the command.

      argv[0] = GetFullExePath(program);
      argv[1] = arg;
      argv[2] = NULL;

      execvp(argv[0], (char **)argv);

      exit(0x80);
    }
  else
    {
      // This is the parent.  Wait for the child to finish, and return
      // the status code.

      waitpid(childpid, &result, 0);

      if (WIFEXITED(result) && WEXITSTATUS(result) != 0x80)
        {
          return WEXITSTATUS(result);
        }
      else
        {
          return -1;
        }
    }
}

int ExecuteDoom(execute_context_t *context)
{
  char *response_file_arg;
  int result;

  fclose(context->stream);

  // Build the command line

  response_file_arg = m_string_join("@", context->response_file, NULL);

  // Run Doom

  result = ExecuteCommand(get_executable_name(), response_file_arg);

  free(response_file_arg);

  // Destroy context
  remove(context->response_file);
  free(context->response_file);
  free(context);

  return result;
}

static void TestCallback(TXT_UNCAST_ARG(widget), TXT_UNCAST_ARG(data))
{
  execute_context_t *exec;
  char *main_cfg;
  char *extra_cfg;
  txt_window_t *testwindow;

  testwindow = txt_message_box("Starting Doom", "Starting Doom to test the\n"
                                                "settings.  Please wait.");
  TXT_DrawDesktop();

  // Save temporary configuration files with the current configuration

  main_cfg = TempFile("tmp.cfg");
  extra_cfg = TempFile("extratmp.cfg");

  m_save_defaultsAlternate(main_cfg, extra_cfg);

  // Run with the -testcontrols parameter

  exec = NewExecuteContext();
  AddCmdLineParameter(exec, "-testcontrols");
  AddCmdLineParameter(exec, "-config \"%s\"", main_cfg);
  AddCmdLineParameter(exec, "-extraconfig \"%s\"", extra_cfg);
  ExecuteDoom(exec);

  TXT_CloseWindow(testwindow);

  // Delete the temporary config files

  remove(main_cfg);
  remove(extra_cfg);
  free(main_cfg);
  free(extra_cfg);
}

txt_window_action_t *TestConfigAction(void)
{
  txt_window_action_t *test_action;

  test_action = txt_new_window_action('t', "Test");
  txt_signal_connect(test_action, "pressed", TestCallback, NULL);

  return test_action;
}
