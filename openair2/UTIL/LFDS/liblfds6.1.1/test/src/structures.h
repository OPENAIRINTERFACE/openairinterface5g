/***** structs *****/
#pragma pack( push, LFDS611_ALIGN_DOUBLE_POINTER )

/***** abstraction tests *****/
struct abstraction_test_cas_state {
  volatile lfds611_atom_t
  *shared_counter;

  lfds611_atom_t
  local_counter;
};

struct abstraction_test_dcas_state {
  volatile lfds611_atom_t
  *shared_counter;

  lfds611_atom_t
  local_counter;
};

/***** freelist tests *****/
struct freelist_test_popping_state {
  struct lfds611_freelist_state
      *fs,
      *fs_thread_local;
};

struct freelist_test_pushing_state {
  lfds611_atom_t
  *count,
  thread_number;

  struct lfds611_freelist_state
      *source_fs,
      *fs;
};

struct freelist_test_popping_and_pushing_state {
  struct lfds611_freelist_state
      *local_fs,
      *fs;
};

struct freelist_test_counter_and_thread_number {
  lfds611_atom_t
  thread_number;

  unsigned long long int
  counter;
};

/***** queue tests *****/
struct queue_test_enqueuing_state {
  struct lfds611_queue_state
      *qs;

  lfds611_atom_t
  counter;
};

struct queue_test_dequeuing_state {
  struct lfds611_queue_state
      *qs;

  int
  error_flag;
};

struct queue_test_enqueuing_and_dequeuing_state {
  struct lfds611_queue_state
      *qs;

  lfds611_atom_t
  counter,
  thread_number,
  *per_thread_counters;

  unsigned int
  cpu_count;

  int
  error_flag;
};

struct queue_test_rapid_enqueuing_and_dequeuing_state {
  struct lfds611_queue_state
      *qs;

  lfds611_atom_t
  counter;
};

/***** ringbuffer tests *****/
struct ringbuffer_test_reading_state {
  struct lfds611_ringbuffer_state
      *rs;

  int
  error_flag;

  lfds611_atom_t
  read_count;
};

struct ringbuffer_test_writing_state {
  struct lfds611_ringbuffer_state
      *rs;

  lfds611_atom_t
  write_count;
};

struct ringbuffer_test_reading_and_writing_state {
  struct lfds611_ringbuffer_state
      *rs;

  lfds611_atom_t
  counter,
  *per_thread_counters;

  unsigned int
  cpu_count;

  int
  error_flag;
};

/***** slist tests *****/
struct slist_test_state {
  struct lfds611_slist_state
      *ss;

  size_t
  create_count,
  delete_count;

  lfds611_atom_t
  thread_and_count;
};

/***** stack tests *****/
struct stack_test_popping_state {
  struct lfds611_stack_state
      *ss,
      *ss_thread_local;
};

struct stack_test_pushing_state {
  lfds611_atom_t
  thread_number;

  struct lfds611_stack_state
      *ss;
};

struct stack_test_popping_and_pushing_state {
  struct lfds611_stack_state
      *local_ss,
      *ss;
};

struct stack_test_counter_and_thread_number {
  lfds611_atom_t
  thread_number,
  counter;
};

/***** freelist benchmarks *****/
struct lfds611_freelist_benchmark {
  struct lfds611_freelist_state
      *fs;

  lfds611_atom_t
  operation_count;
};

/***** queue benchmarks *****/
struct lfds611_queue_benchmark {
  struct lfds611_queue_state
      *qs;

  lfds611_atom_t
  operation_count;
};

/***** ringbuffer benchmarks *****/
struct lfds611_ringbuffer_benchmark {
  struct lfds611_ringbuffer_state
      *rs;

  lfds611_atom_t
  operation_count;
};

/***** stack benchmarks *****/
struct lfds611_stack_benchmark {
  struct lfds611_stack_state
      *ss;

  lfds611_atom_t
  operation_count;
};

#pragma pack( pop )

