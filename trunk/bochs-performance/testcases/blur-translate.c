/*
 *
 * $Id: blur-translate.c,v 1.1 2002-04-17 08:12:49 bdenney Exp $
 *
 */

#include <stdio.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <assert.h>
#include <ltdl.h>       // libtool library 

#define MAX 128
int array[MAX][MAX];
int array2[MAX][MAX];
#define BLUR_WINDOW_HALF 1

#define DEFAULT_TIMES 1000

typedef enum {
  OP_MOVE_REL,           // 2 args delta_x and delta_y
  OP_SET_ACCUM,          // 1 arg, sets accum to that arg
  OP_ADD_DATA,           // add data from *load_ptr
  OP_SUBTRACT_DATA,      // sub data from *load_ptr
  OP_MULTIPLY_DATA,      // mul data from *load_ptr
  OP_STORE_DATA,         // store accum to *store_ptr
  OP_END,                // stop
  N_OPS  // must be last
} Opcode;

typedef struct {
  int x, y;
  int accum;
  int *load_ptr;
  int *store_ptr;
  int done;
} State;

typedef void (*exec_func)(State *state);

typedef struct {
  int n_opcodes;
  int *opcode_list;
  lt_dlhandle dlhandle;
  exec_func func;
} CodeBlock;

// this opcode sequence implements the blur filter, just like all the others.
int blur_instructions[] = {
  OP_MOVE_REL, -1, -1,
  OP_SET_ACCUM, 0,
  OP_ADD_DATA,
  OP_MOVE_REL, 0, 1,
  OP_ADD_DATA,
  OP_MOVE_REL, 0, 1,
  OP_ADD_DATA,
  OP_MOVE_REL, 1, -2,
  OP_ADD_DATA,
  OP_MOVE_REL, 0, 1,
  OP_ADD_DATA,
  OP_MOVE_REL, 0, 1,
  OP_ADD_DATA,
  OP_MOVE_REL, 1, -2,
  OP_ADD_DATA,
  OP_MOVE_REL, 0, 1,
  OP_ADD_DATA,
  OP_MOVE_REL, 0, 1,
  OP_ADD_DATA,
  OP_MOVE_REL, -1, -1,
  OP_STORE_DATA,
  OP_END
};

CodeBlock blur = {
  sizeof(blur_instructions),
  &blur_instructions[0],
  NULL,
  NULL
};

void print_state (State *state)
{
  printf ("state={x=%d, y=%d, accum=%d, load_ptr=%p, store_ptr=%p\n",
      state->x,
      state->y,
      state->accum,
      state->load_ptr,
      state->store_ptr);
}

void blur_simple()
{
  int sum;
  int x,y,x2,y2;
  for (x=1; x<MAX-1; x++)
    for (y=1; y<MAX-1; y++)
    {
      sum = 0;
      for (x2=x-BLUR_WINDOW_HALF; x2<=x+BLUR_WINDOW_HALF; x2++)
	for (y2=y-BLUR_WINDOW_HALF; y2<=y+BLUR_WINDOW_HALF; y2++) {
	  sum += array[x2][y2];
	}
      array2[x][y] = sum;
    }
}

void emulate_opcodes (int *opcode_list)
{
  int sum;
  int x,y,x2,y2;
  State state;
  for (x=1; x<MAX-1; x++)
    for (y=1; y<MAX-1; y++)
    {
      int *pc;
      int done = 0;
      sum = 0;
      pc = opcode_list;
      // set up state 
      state.x = x;
      state.y = y;
      state.load_ptr = &array[x][y];
      state.store_ptr = &array2[x][y];
      while (!done) {
	//print_state (&state);
	switch (*pc++)
	{
	  case OP_MOVE_REL:
	    state.x += *pc++;
	    state.y += *pc++;
	    state.load_ptr = &array[state.x][state.y];
	    break;
	  case OP_SET_ACCUM:
	    state.accum = *pc++;
	    break;
	  case OP_ADD_DATA:
	    state.accum += *state.load_ptr;
	    break;
	  case OP_SUBTRACT_DATA:
	    state.accum -= *state.load_ptr;
	    break;
	  case OP_MULTIPLY_DATA:
	    state.accum *= *state.load_ptr;
	    break;
	  case OP_STORE_DATA:
	    *state.store_ptr = state.accum;
	    break;
	  case OP_END:
	    done = 1;
	    break;
	  default:
	    assert (0);
	}
      }
    }
}

int gen_header (FILE *out)
{
  fprintf (out, "// code generated by blur-translate.c\n");
  fprintf (out, "#include \"translate2-defs.h\"\n");
  fprintf (out, "\n");
  return 0;
}

int gen_function (int *instructions, FILE *out, int fnid)
{
  int done = 0;
  int *pc = instructions;
  fprintf (out, "\nvoid translate%d (State *state) {\n", fnid);
  // the BEGIN_TRANSLATED_FUNCTION macro is defined in the 
  // translate2-defs.h file.  It creates local variables to mirror the
  // frequently used fields in the state struct.
  fprintf (out, "BEGIN_TRANSLATED_FUNCTION()\n");

  // this code is executed before the block begins, each time.
  fprintf (out, "  ST(load_ptr) =  &array[x][y];\n");
  fprintf (out, "  ST(store_ptr) =  &array2[x][y];\n");

  while (!done) {
    switch (*pc++) {
      case OP_MOVE_REL:
	fprintf (out, "DO_MOVE_REL(%d,%d);\n", *pc++, *pc++);
	break;
      case OP_SET_ACCUM:
	fprintf (out, "DO_SET_ACCUM(%d);\n", *pc++);
	break;
      case OP_ADD_DATA:
	fprintf (out, "DO_ADD_DATA();\n");
	break;
      case OP_SUBTRACT_DATA:
	fprintf (out, "DO_SUBTRACT_DATA();\n");
	break;
      case OP_MULTIPLY_DATA:
	fprintf (out, "DO_MULTIPLY_DATA();\n");
	break;
      case OP_STORE_DATA:
	fprintf (out, "DO_STORE_DATA();\n");
	break;
      case OP_END:
	done = 1;
	break;
      default:
	assert (0);
    }
  }

  // loop count updates
  fprintf (out, "  ST(y)++;                   \n");
  fprintf (out, "  if (!(ST(y)<MAX-1)) {      \n");
  fprintf (out, "    ST(y)=1;                 \n");
  fprintf (out, "    ST(x)++;                 \n");
  fprintf (out, "    if (!(ST(x)<MAX-1)) {    \n");
  fprintf (out, "	done=1;                \n");
  fprintf (out, "    }                        \n");
  fprintf (out, "  }                          \n");

  fprintf (out, "END_TRANSLATED_FUNCTION()\n");
  fprintf (out, "}  // end of translate%d\n", fnid);
  return 0;
}

static int unique_num = 1001;
int translate_block (CodeBlock *block)
{
  char buffer[1024];
  FILE *fp;
  int n;
  struct stat st;
  int id = unique_num++;
  if (block->func != NULL) return 0;
  block->dlhandle = NULL;
  block->func = NULL;
  // generate C code
  sprintf (buffer, "translate%d.c", id);
  fprintf (stderr, "building translation function in %s\n", buffer);
  fp = fopen (buffer, "w");
  assert (fp!=NULL);
  n = gen_header (fp);
  assert (n>=0);
  n = gen_function (block->opcode_list, fp, id);
  assert (n>=0);
  fclose (fp);
  // compile into a shared library
  fprintf (stderr, "compiling %s\n", buffer);
  sprintf (buffer, "./buildshared translate%d", id);
  if (system (buffer) < 0) {
    fprintf (stderr, "failed: %s\n", buffer);
    return -1;
  }
  sprintf (buffer, "translate%d.c", id);
  if (stat (buffer, &st) < 0) {
    fprintf (stderr, "stat failed\n");
    return -1;
  }
  fprintf (stderr, "shared library is %d bytes\n", (int)st.st_size);
  // open shared library and get the function pointer
  sprintf (buffer, "libtranslate%d.la", id);
  block->dlhandle = lt_dlopen (buffer);
  if (!block->dlhandle) {
    fprintf (stderr, "can't open the module %s!\n", buffer);
    fprintf (stderr, "error was: %s\n", lt_dlerror());
    return -1;
  }
  sprintf (buffer, "translate%d", id);
  block->func = (void (*)(State *)) lt_dlsym(block->dlhandle, buffer);
  if (!block->func) {
    fprintf (stderr, "can't find symbol %s\n", buffer);
    return -1;
  }
  return 0;
}

void execute_code_block (CodeBlock *block)
{
  State state;
  if (!block->func) {
    int n = translate_block (block);
    assert (n>=0);
    assert (block->func != NULL);
  }
  state.x = 1;
  state.y = 1;
  (*block->func)(&state);
}

void fill_array()
{
  int x,y;
  for (x=0; x<MAX; x++)
    for (y=0; y<MAX; y++)
      array[x][y] = (x*17+y*31)%29;
}

void dump_array (FILE *fp, int ptr[MAX][MAX])
{
  int x,y;
  for (x=0; x<MAX; x++) {
    for (y=0; y<MAX; y++) {
      fprintf (fp, "%3d ", ptr[x][y]);
    }
    fprintf (fp, "\n");
  }
}

struct timeval start, stop;
#define start_timer() gettimeofday (&start, NULL);
#define stop_timer() gettimeofday (&stop, NULL);

void report_time (FILE *fp, int iters)
{
  int usec_duration = 
    (stop.tv_sec*1000000 + stop.tv_usec)
    - (start.tv_sec*1000000 + start.tv_usec);
  double sec = (double)usec_duration / 1.0e3;
  double sec_per_iter = sec / (double)iters;
  fprintf (fp, "Total time elapsed = %f msec\n", sec);
  fprintf (fp, "Iterations = %d\n", iters);
  fprintf (fp, "Time per iteration = %f msec\n", sec_per_iter);
}

int main (int argc, char *argv[])
{
  int i;
  int times = 0;
  FILE *out;
  if (argc>1) {
    assert (sscanf (argv[1], "%d", &times) == 1);
  } else {
    times = DEFAULT_TIMES;
  }
  if (lt_dlinit () != 0) {
    fprintf (stderr, "lt_dlinit() failed\n");
    exit (1);
  }
  fill_array();
  //dump_array (stderr);
  start_timer();
  for (i=0; i<times; i++) {
#if defined USE_SIMPLE
    blur_simple ();
#elif defined USE_DYNAMIC_TRANSLATION
    execute_code_block (&blur);
#else
    emulate_opcodes (blur.opcode_list);
#endif
  }
  stop_timer();
  report_time (stdout, times);
  //fprintf (stderr, "-----------------------------------\n");
  out = fopen ("blur.out", "w");
  assert (out != NULL);
  dump_array (out, array2);
  fclose (out);
  return 0;
}
