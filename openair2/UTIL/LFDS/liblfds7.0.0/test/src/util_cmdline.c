/***** includes *****/
#include "internal.h"





/****************************************************************************/
void util_cmdline_init( struct util_cmdline_state *cs )
{
  lfds700_pal_uint_t
    loop;

  assert( cs != NULL );

  for( loop = 0 ; loop < NUMBER_OF_LOWERCASE_LETTERS_IN_LATIN_ALPHABET ; loop++ )
  {
    cs->args[loop].arg_type = LIBCOMMON_CMDLINE_ARG_TYPE_UNSET;
    cs->args[loop].processed_flag = LOWERED;
  }

  return;
}





/****************************************************************************/
#pragma warning( disable : 4100 )

void util_cmdline_cleanup( struct util_cmdline_state *cs )
{
  assert( cs != NULL );

  return;
}

#pragma warning( default : 4100 )





/****************************************************************************/
void util_cmdline_add_arg( struct util_cmdline_state *cs, char arg_letter, enum util_cmdline_arg_type arg_type )
{
  lfds700_pal_uint_t
    index;

  assert( cs != NULL );
  assert( arg_letter >= 'a' and arg_letter <= 'z' );
  // TRD : arg_type can be any value in its range

  index = arg_letter - 'a';

  cs->args[index].arg_type = arg_type;

  if( arg_type == LIBCOMMON_CMDLINE_ARG_TYPE_FLAG )
    cs->args[index].arg_data.flag.flag = LOWERED;

  return;
}





/****************************************************************************/
int util_cmdline_process_args( struct util_cmdline_state *cs, int argc, char **argv )
{
  char
    *arg;

  int
    arg_letter,
    cc,
    loop,
    rv = 1;

  lfds700_pal_uint_t
    index;

  assert( cs != NULL );
  assert( argc >= 1 );
  assert( argv != NULL );

  for( loop = 1 ; loop < argc ; loop++ )
  {
    arg = *(argv+loop);

    switch( *arg )
    {
      case '-':
        arg_letter = tolower( *(arg+1) );

        if( arg_letter >= 'a' and arg_letter <= 'z' )
        {
          index = arg_letter - 'a';

          switch( cs->args[index].arg_type )
          {
            case LIBCOMMON_CMDLINE_ARG_TYPE_INTEGER_RANGE:
              if( loop+1 >= argc )
                rv = 0;

              if( loop+1 < argc )
              {
                cc = sscanf( *(argv+loop+1), "%llu-%llu", &cs->args[index].arg_data.integer_range.integer_start, &cs->args[index].arg_data.integer_range.integer_end );

                if( cc != 2 )
                  rv = 0;

                if( cc == 2 )
                {
                  cs->args[index].processed_flag = RAISED;
                  loop++;
                }
              }
            break;

            case LIBCOMMON_CMDLINE_ARG_TYPE_INTEGER:
              if( loop+1 >= argc )
                rv = 0;

              if( loop+1 < argc )
              {
                cc = sscanf( *(argv+loop+1), "%llu", &cs->args[index].arg_data.integer.integer );

                if( cc != 1 )
                  rv = 0;

                if( cc == 1 )
                {
                  cs->args[index].processed_flag = RAISED;
                  loop++;
                }
              }
            break;

            case LIBCOMMON_CMDLINE_ARG_TYPE_FLAG:
              cs->args[index].arg_data.flag.flag = RAISED;
              cs->args[index].processed_flag = RAISED;
            break;

            case LIBCOMMON_CMDLINE_ARG_TYPE_UNSET:
            break;
          }
        }
      break;

      default:
        rv = 0;
      break;
    }
  }

  return( rv );
}





/****************************************************************************/
void util_cmdline_get_arg_data( struct util_cmdline_state *cs, char arg_letter, union util_cmdline_arg_data **arg_data )
{
  lfds700_pal_uint_t
    index;

  assert( cs != NULL );
  assert( arg_letter >= 'a' and arg_letter <= 'z' );
  assert( arg_data != NULL );

  index = arg_letter - 'a';

  if( cs->args[index].processed_flag == RAISED )
    *arg_data = &cs->args[index].arg_data;
  else
    *arg_data = NULL;

  return;
}

