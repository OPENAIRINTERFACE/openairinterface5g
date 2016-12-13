/***** structs *****/
struct util_thread_starter_thread_state
{
  // TRD : must be volatile or the compiler optimizes it away into a single load
  enum flag volatile
    thread_ready_flag,
    *thread_start_flag;

  void
    *thread_user_state;
};

struct util_thread_starter_state
{
  enum flag volatile
    thread_start_flag;

  lfds700_pal_uint_t
    number_thread_states;

  struct util_thread_starter_thread_state
    *tsts;
};

/***** prototypes *****/
void util_thread_starter_new( struct util_thread_starter_state **tts, lfds700_pal_uint_t number_threads );
void util_thread_starter_start( struct util_thread_starter_state *tts,
                                test_pal_thread_state_t *thread_state,
                                lfds700_pal_uint_t thread_number,
                                struct test_pal_logical_processor *lp, 
                                test_pal_thread_return_t (TEST_PAL_CALLING_CONVENTION *thread_function)( void *thread_user_state ),
                                void *thread_user_state );
void util_thread_starter_ready_and_wait( struct util_thread_starter_thread_state *tsts );
void util_thread_starter_run( struct util_thread_starter_state *tts );
void util_thread_starter_delete( struct util_thread_starter_state *tts );

void util_thread_start_wrapper( test_pal_thread_state_t *thread_state,
                                struct test_pal_logical_processor *lp,
                                test_pal_thread_return_t (TEST_PAL_CALLING_CONVENTION *thread_function)(void *thread_user_state),
                                void *thread_user_state );

