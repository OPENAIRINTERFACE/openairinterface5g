/***** defines *****/
#define NUMBER_OF_LOWERCASE_LETTERS_IN_LATIN_ALPHABET 26

/***** enums *****/
enum util_cmdline_arg_type
{
  LIBCOMMON_CMDLINE_ARG_TYPE_INTEGER_RANGE,
  LIBCOMMON_CMDLINE_ARG_TYPE_INTEGER,
  LIBCOMMON_CMDLINE_ARG_TYPE_FLAG,
  LIBCOMMON_CMDLINE_ARG_TYPE_UNSET
};

/***** structs *****/
struct util_cmdline_arg_integer_range
{
  int long long unsigned
    integer_start,
    integer_end;
};

struct util_cmdline_arg_integer
{
  int long long unsigned
    integer;
};

struct util_cmdline_arg_flag
{
  enum flag
    flag;
};

union util_cmdline_arg_data
{
  struct util_cmdline_arg_integer_range
    integer_range;

  struct util_cmdline_arg_integer
    integer;

  struct util_cmdline_arg_flag
    flag;
};

struct util_cmdline_arg_letter_and_data
{
  enum util_cmdline_arg_type
    arg_type;

  enum flag
    processed_flag;

  union util_cmdline_arg_data
    arg_data;
};

struct util_cmdline_state
{
  struct util_cmdline_arg_letter_and_data
    args[NUMBER_OF_LOWERCASE_LETTERS_IN_LATIN_ALPHABET];
};

/***** public protoypes *****/
void util_cmdline_init( struct util_cmdline_state *cs );
void util_cmdline_cleanup( struct util_cmdline_state *cs );
void util_cmdline_add_arg( struct util_cmdline_state *cs, char arg_letter, enum util_cmdline_arg_type arg_type );
int  util_cmdline_process_args( struct util_cmdline_state *cs, int argc, char **argv );
void util_cmdline_get_arg_data( struct util_cmdline_state *cs, char arg_letter, union util_cmdline_arg_data **arg_data );

