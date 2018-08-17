/*
 * To disable the checks done by this program, see below at the beginning
 * of the function 'main'.
 */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "database.h"

#define T_TRACER 1
#include "T.h"

/* VCD specific defines and includes
 * If the codebase changes, it may need to be updated
 */
#define ENB_MODE
#define ENABLE_USE_CPU_EXECUTION_TIME
#include "../LOG/vcd_signal_dumper.c"

void err(char *fmt, ...) __attribute__ ((format (printf, 1, 2)));
void err(char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);

  printf("\x1b[31m");
  printf("error: ");
  vprintf(fmt, ap);
  printf("\n"
"\x1b[33m\n"
"You probably added a VCD trace (variable or function) but you did not\n"
"update T_messages.txt and/or T_defs.h in common/utils/T/\n"
"\n"
"Be sure to add the new trace to T_messages.txt, at the right place in the\n"
"file. Do not forget to define VCD_NAME with an identical value as found\n"
"in the array eurecomVariablesNames or eurecomFunctionsNames.\n"
"\n"
"Be sure to update VCD_NUM_FUNCTIONS, VCD_NUM_VARIABLES, VCD_FIRST_FUNCTION\n"
"and VCD_FIRST_VARIABLE in T_defs.h\n"
"\n"
"The same procedure has to be followed when you delete a VCD trace.\n"
"Delete it in T_messages.txt as well and update T_defs.h\n"
"\n"
"You can disable those VCD checks at development time.\n"
"To disable the VCD checks see the file common/utils/T/check_vcd.c\n"
"Do not push any modification that disables the VCD checks to the\n"
"main repository.\n");

  printf("\x1b[m\n");

  va_end(ap);

  exit(1);
}

int main(void)
{
  /* to disable the checks done by this program, uncomment the following
   * line, ie. remove the leading '//'
   */
  //return 0;

  void *database = parse_database("T_messages.txt");
  int number_of_events;
  int first_var = -1;
  int last_var;
  int first_fun = -1;
  int last_fun;
  char *prefix;
  int prefix_len;
  char *name;
  char *vcd_name;
  int i;
  FILE *in;
  char *l = NULL;
  size_t lsize;

  if (database == NULL) err("something wrong with T_messages.txt");

  /* check the value of VCD_NUM_FUNCTIONS */
  if (VCD_NUM_FUNCTIONS != sizeof(eurecomFunctionsNames) / sizeof(char *))
    err("VCD_NUM_FUNCTIONS (%d) must be equal to %zd",
        VCD_NUM_FUNCTIONS,
        sizeof(eurecomFunctionsNames) / sizeof(char *));

  /* check the value of VCD_NUM_VARIABLES */
  if (VCD_NUM_VARIABLES != sizeof(eurecomVariablesNames) / sizeof(char *))
    err("VCD_NUM_VARIABLES (%d) must be equal to %zd",
        VCD_NUM_VARIABLES,
        sizeof(eurecomVariablesNames) / sizeof(char *));

  number_of_events = number_of_ids(database);
  if (number_of_events == 0) err("no event defined in T_messages.txt");

  /* T_messages.txt ends with VCD VARIABLES followed by VCD FUNCTIONS
   * followed by nothing.
   * Let's check.
   */

  /* check VCD VARIABLES traces in T_messages.txt */
  prefix = "VCD_VARIABLE_";
  prefix_len = strlen(prefix);

  for (i = 0; i < number_of_events; i++) {
    name = event_name_from_id(database, i);
    if (strncmp(name, prefix, prefix_len)) continue;
    first_var = i;
    break;
  }
  if (first_var == -1)
    err("no VCD_VARIABLE_ found in T_messages.txt");
  for (; i < number_of_events; i++) {
    name = event_name_from_id(database, i);
    if (strncmp(name, prefix, prefix_len)) break;
  }
  last_var = i-1;

  /* check VCD FUNCTIONS traces in T_messages.txt */
  if (i == number_of_events)
    err("no VCD_FUNCTION_ found in T_messages.txt");

  prefix = "VCD_FUNCTION_";
  prefix_len = strlen(prefix);

  first_fun = i;

  name = event_name_from_id(database, i);
  if (strncmp(name, prefix, prefix_len))
    err("last VCD_VARIABLE_ not followed by a VCD_FUNCTION_ in T_messages.txt");

  for (; i < number_of_events; i++) {
    name = event_name_from_id(database, i);
    if (strncmp(name, prefix, prefix_len)) break;
  }

  if (i != number_of_events)
    err("T_messages.txt does not end with a VCD_FUNCTION_ trace");

  last_fun = i-1;

  if (first_var != (unsigned)VCD_FIRST_VARIABLE)
    err("VCD_FIRST_VARIABLE is not correct in T_defs.h");
  if (first_fun != (unsigned)VCD_FIRST_FUNCTION)
    err("VCD_FIRST_FUNCTION is not correct in T_defs.h");
  if (last_var-first_var+1 != VCD_NUM_VARIABLES)
    err("VCD_NUM_VARIABLES is not correct in T_defs.h");
  if (last_fun-first_fun+1 != VCD_NUM_FUNCTIONS)
    err("VCD_NUM_FUNCTIONS is not correct in T_defs.h");

  /* check that VCD_NAME is identical to
   * eurecomVariablesNames[x]/eurecomFunctionsNames[x]
   */
  prefix = "VCD_VARIABLE_";
  prefix_len = strlen(prefix);

  for (i = 0; i < number_of_events; i++) {
    name = event_name_from_id(database, i);
    if (strncmp(name, prefix, prefix_len)) continue;
    vcd_name = event_vcd_name_from_id(database, i);
    if (vcd_name == NULL)
      err("%s has no VCD_NAME in T_messages.txt", name);
    if (strcmp(vcd_name, eurecomVariablesNames[i - first_var]))
      err("%s has a wrong VCD_NAME in T_messages.txt", name);
  }

  prefix = "VCD_FUNCTION_";
  prefix_len = strlen(prefix);

  for (i = 0; i < number_of_events; i++) {
    name = event_name_from_id(database, i);
    if (strncmp(name, prefix, prefix_len)) continue;
    vcd_name = event_vcd_name_from_id(database, i);
    if (vcd_name == NULL)
      err("%s has no VCD_NAME in T_messages.txt", name);
    if (strcmp(vcd_name, eurecomFunctionsNames[i - first_fun]))
      err("%s has a wrong VCD_NAME in T_messages.txt", name);
  }

  /* check IDs - these checks are difficult because we parse
   * common/utils/LOG/vcd_signal_dumper.h which is expected to
   * be formatted as is:
   * - define vcd_signal_dump_variables then vcd_signal_dump_functions
   * - one VCD_XXXX per line starting with two spaces
   *   followed by ',' or '=' with no space in between
   * - no #ifdef / #if is taken into account
   * - we require VCD_SIGNAL_DUMPER_VARIABLES_END and
   *   VCD_SIGNAL_DUMPER_FUNCTIONS_END at the end of each array,
   *   each on a line of its own with two spaces before and nothing after.
   *
   * If these checks fail, consider formatting
   * common/utils/LOG/vcd_signal_dumper.h as expected here, if
   * it makes sense of course. Otherwise, change the code below.
   *
   * In common/utils/LOG/vcd_signal_dumper.h a valid name is
   * either VCD_SIGNAL_DUMPER_VARIABLES_ABC or
   * VCD_SIGNAL_DUMPER_FUNCTIONS_XYZ
   * and in T_messages.txt the corresponding name has to be
   * VCD_VARIABLE_ABC or VCD_FUNCTION_XYZ
   */
  i = first_var;
  in = fopen("../LOG/vcd_signal_dumper.h", "r");
  if (in == NULL) err("could not open ../LOG/vcd_signal_dumper.h");
  while (1) {
    char *x = "  VCD_SIGNAL_DUMPER_VARIABLES_";
    ssize_t r;
    free(l);
    l = NULL;
    lsize = 0;
    r = getline(&l, &lsize, in);
    if (r == -1) break;
    if (!strcmp(l, "  VCD_SIGNAL_DUMPER_VARIABLES_END\n")) break;
    /* remove ',' or '=' if found */
    { char *s=l; while (*s) { if (*s==','||*s=='=') { *s=0; break; } s++; } }
    if (strncmp(l, x, strlen(x))) continue;
    if (!(i >= first_var && i <= last_var))
      err("T_messages.txt is not correct with respect to VCD VARIABLES");
    name = event_name_from_id(database, i);
    if (strcmp(l+strlen(x), name+strlen("VCD_VARIABLE_")))
      err("%s is not correct in T_messages.txt", name);
    i++;
  }
  if (i != last_var + 1) err("VCD VARIABLES wrong in T_messages.txt");
  while (1) {
    char *x = "  VCD_SIGNAL_DUMPER_FUNCTIONS_";
    ssize_t r;
    free(l);
    l = NULL;
    lsize = 0;
    r = getline(&l, &lsize, in);
    if (r == -1) break;
    if (!strcmp(l, "  VCD_SIGNAL_DUMPER_FUNCTIONS_END\n")) break;
    /* remove ',' or '=' if found */
    { char *s=l; while (*s) { if (*s==','||*s=='=') { *s=0; break; } s++; } }
    if (strncmp(l, x, strlen(x))) continue;
    if (!(i >= first_fun && i <= last_fun))
      err("T_messages.txt is not correct with respect to VCD FUNCTIONS");
    name = event_name_from_id(database, i);
    if (strcmp(l+strlen(x), name+strlen("VCD_FUNCTION_")))
      err("%s is not correct in T_messages.txt", name);
    i++;
  }
  fclose(in);
  if (i != last_fun + 1) err("VCD FUNCTIONS wrong in T_messages.txt");

  return 0;
}
