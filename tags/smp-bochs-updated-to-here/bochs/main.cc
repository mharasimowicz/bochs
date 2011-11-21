//  Copyright (C) 2001  MandrakeSoft S.A.
//
//    MandrakeSoft S.A.
//    43, rue d'Aboukir
//    75002 Paris - France
//    http://www.linux-mandrake.com/
//    http://www.mandrakesoft.com/
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA



#include "bochs.h"
#include <assert.h>
#include "state_file.h"

extern "C" {
#include <signal.h>
}

#ifdef __MINGW32__
void alarm(int);
#endif


#if BX_PROVIDE_DEVICE_MODELS==1
// some prototypes from iodev/
// I want to stay away from including iodev/iodev.h here
Bit32u bx_unmapped_io_read_handler(Bit32u address, unsigned io_len);
void   bx_unmapped_io_write_handler(Bit32u address, Bit32u value,
                                    unsigned io_len);
void   bx_close_harddrive(void);
#endif



void bx_init_debug(void);
void bx_emulate_hga_dumps_timer(void);

/* typedefs */


#if ( BX_PROVIDE_DEVICE_MODELS==1 )
bx_pc_system_c bx_pc_system;
class state_file state_stuff("state_file.out", "options");
#endif

bx_debug_t bx_dbg;

bx_options_t bx_options = {
  { "", BX_FLOPPY_NONE, BX_EJECTED },
  { "", BX_FLOPPY_NONE, BX_EJECTED },
  { 0, "", 0, 0, 0 },
  { 0, "", 0, 0, 0 },
  { 0, "", 0 },
  { NULL, 0 },
  { NULL },
  { BX_DEFAULT_MEM_MEGS },
  { NULL, NULL, NULL, 0, 0, 0, 0 },     // SB16 options
  "a",
  300000,
  20000,  // default keyboard serial path delay (usec)
  50000,  // default floppy command delay (usec)
  500000,  // default ips (instructions-per-second)
  0,       // default mouse_enabled
  0,       // default private_colormap
  0,          // default i440FXSupport
  {NULL, 0},  // cmos path, cmos image boolean
  { 0, 0, 0, {0,0,0,0,0,0}, NULL, NULL }, // ne2k
  1,          // newHardDriveSupport
  { 0, NULL, NULL, NULL }, // load32bitOSImage hack stuff
  { 0, 1, 1, 2 }  // ignore debugs, report infos and errors, crash on panics.
  };

static char bochsrc_path[512];
static char logfilename[512] = "-";


static void parse_line_unformatted(char *line);
static void parse_line_formatted(int num_params, char *params[]);
static void parse_bochsrc(int argc);


// Just for the iofunctions

#define LOG_THIS this->log->

int Allocio=0;

void
iofunctions::flush(void) {
	if(logfd && magic == MAGIC_LOGNUM) {
		fflush(logfd);
	}
}

void
iofunctions::init(void) {
	// iofunctions methods must not be called before this magic
	// number is set.
	magic=MAGIC_LOGNUM;
	showtick = 1;
	init_log(stderr);
	log = new logfunc_t(this);
	LOG_THIS setprefix("[IO  ]");
	LOG_THIS settype(IOLOG);
	BX_DEBUG(("Init(log file: '%s').\n",logfn));
}

void
iofunctions::init_log(char *fn)
{
	assert (magic==MAGIC_LOGNUM);
	// use newfd/newfn so that we can log the message to the OLD log
	// file descriptor.
	FILE *newfd = stderr;
	char *newfn = "/dev/stderr";
	if( strcmp( fn, "-" ) != 0 ) {
		newfd = fopen(fn, "w");
		if(newfd != NULL) {
			newfn = strdup(fn);
			BX_DEBUG(("Opened log file '%s'.\n", fn ));
		} else {
			BX_DEBUG(("Log file '%s' not there?\n", fn));
			newfd = NULL;
			logfn = "(none)";
		}
	}
	logfd = newfd;
	logfn = newfn;
}

void
iofunctions::init_log(FILE *fs)
{
	assert (magic==MAGIC_LOGNUM);
	logfd = fs;

	if(fs == stderr) {
		logfn = "/dev/stderr";
	} else if(fs == stdout) { 
		logfn = "/dev/stdout";
	} else {
		logfn = "(unknown)";
	}
}

void
iofunctions::init_log(int fd)
{
	assert (magic==MAGIC_LOGNUM);
	FILE *tmpfd;
	if( (tmpfd = fdopen(fd,"w")) == NULL ) {
		fprintf(stderr, "Couldn't open fd %d as a stream for writing\n",
			fd);
		return;
	}

	init_log(tmpfd);
	return;
};

//  iofunctions::out( class, level, prefix, fmt, ap)
//  DO NOT nest out() from ::info() and the like.
//    fmt and ap retained for direct printinf from iofunctions only!

void
iofunctions::out(int f, int l, char *prefix, char *fmt, va_list ap)
{
	assert (magic==MAGIC_LOGNUM);
	assert (this != NULL);
	assert (logfd != NULL);

	if( showtick )
		fprintf(logfd, "%011lld ", bx_pc_system.time_ticks());

	if(prefix != NULL)
		fprintf(logfd, "%s ", prefix);

	if(l==LOGLEV_PANIC)
		fprintf(logfd, ">>PANIC<< ");

	vfprintf(logfd, fmt, ap);
	fflush(logfd);

	return;
}

iofunctions::iofunctions(FILE *fs)
{
	init();
	init_log(fs);
}

iofunctions::iofunctions(char *fn)
{
	init();
	init_log(fn);
}

iofunctions::iofunctions(int fd)
{
	init();
	init_log(fd);
}

iofunctions::iofunctions(void)
{
	this->init();
}

iofunctions::~iofunctions(void)
{
	// flush before erasing magic number, or flush does nothing.
	this->flush();
	this->magic=0;
}

#undef LOG_THIS
#define LOG_THIS genlog->

logfunctions::logfunctions(void)
{
	setprefix("[GEN ]");
	settype(GENLOG);
	if(io == NULL && Allocio == 0) {
		Allocio = 1;
		io = new iofunc_t(stderr);
	}
	setio(io);
	// BUG: unfortunately this can be called before the bochsrc is read,
	// which means that the bochsrc has no effect on the actions.
	for (int i=0; i<MAX_LOGLEV; i++)
	  onoff[i] = bx_options.log_actions[i];
}

logfunctions::logfunctions(iofunc_t *iofunc)
{
	setprefix("[GEN ]");
	settype(GENLOG);
	setio(iofunc);
	// BUG: unfortunately this can be called before the bochsrc is read,
	// which means that the bochsrc has no effect on the actions.
	for (int i=0; i<MAX_LOGLEV; i++)
	  onoff[i] = bx_options.log_actions[i];
}

logfunctions::~logfunctions(void)
{
}

void
logfunctions::setio(iofunc_t *i)
{
	this->logio = i;
}

void
logfunctions::setprefix(char *p)
{
	this->prefix=strdup(p);
}

void
logfunctions::settype(int t)
{
	type=t;
}

void
logfunctions::info(char *fmt, ...)
{
	va_list ap;
	FILE *fs;

	assert (this != NULL);
	assert (this->logio != NULL);

	if(!onoff[LOGLEV_INFO])
		return;

	va_start(ap, fmt);
	this->logio->out(this->type,LOGLEV_INFO,this->prefix, fmt, ap);
	va_end(ap);

	if (onoff[LOGLEV_INFO] > 1) crash ();
}

void
logfunctions::error(char *fmt, ...)
{
	va_list ap;
	FILE *fs;

	assert (this != NULL);
	assert (this->logio != NULL);

	if(!onoff[LOGLEV_ERROR])
		return;

	va_start(ap, fmt);
	this->logio->out(this->type,LOGLEV_ERROR,this->prefix, fmt, ap);
	va_end(ap);
	if (onoff[LOGLEV_ERROR] > 1) crash ();
}

void
logfunctions::panic(char *fmt, ...)
{
	va_list ap;
	FILE *fs;

	assert (this != NULL);
	assert (this->logio != NULL);

	if(onoff[LOGLEV_PANIC]) { // XXX to return or not to return?

		va_start(ap, fmt);
		this->logio->out(this->type,LOGLEV_PANIC,this->prefix, fmt, ap);
		va_end(ap);

	}
	if (onoff[LOGLEV_PANIC] > 1) crash ();
}

void
logfunctions::ldebug(char *fmt, ...)
{
	va_list ap;
	FILE *fs;

	assert (this != NULL);
	assert (this->logio != NULL);

	if(!onoff[LOGLEV_DEBUG])
		return;

	va_start(ap, fmt);
	this->logio->out(this->type,LOGLEV_DEBUG,this->prefix, fmt, ap);
	va_end(ap);
	if (onoff[LOGLEV_DEBUG] > 1) crash ();
}

void
logfunctions::crash (void)
{
  bx_atexit();

#if !BX_DEBUGGER
  exit(1);
#else
  static Boolean dbg_exit_called = 0;
  if (dbg_exit_called == 0) {
    dbg_exit_called = 1;
    bx_dbg_exit(1);
    }
#endif
}

iofunc_t *io = NULL;
logfunc_t *genlog = NULL;

void bx_center_print (FILE *file, char *line, int maxwidth)
{
  int imax;
  imax = (maxwidth - strlen(line)) >> 1;
  for (int i=0; i<imax; i++) fputc (' ', file);
  fputs (line, file);
}

static char *divider = "========================================================================";

void bx_print_header ()
{
  fprintf (stderr, "%s\n", divider);
  char buffer[128];
  sprintf (buffer, "Bochs x86 Emulator %s\n", VER_STRING);
  bx_center_print (stderr, buffer, 72);
  sprintf (buffer, "%s\n", REL_STRING);
  bx_center_print (stderr, buffer, 72);
  fprintf (stderr, "%s\n", divider);
}

void bx_print_footer ()
{
  fprintf (stderr, "%s\n", divider);
  fprintf (stderr, "Bochs is finished.\n");
}

int
main(int argc, char *argv[])
{
  // To deal with initialization order problems inherent in C++, use
  // the macros SAFE_GET_IOFUNC and SAFE_GET_GENLOG to retrieve "io" and "genlog"
  // in all constructors or functions called by constructors.  The macros
  // test for NULL and create the object if necessary, then return it.
  // Ensure that io and genlog get created, by making one reference to
  // each macro right here.  All other code can call them directly.
  SAFE_GET_IOFUNC();
  SAFE_GET_GENLOG();

  bx_print_header ();

#if BX_DEBUGGER
  // If using the debugger, it will take control and call
  // bx_bochs_init() and cpu_loop()
  bx_dbg_main(argc, argv);
#else
  // If not using the debugger, pass control on normally
  bx_bochs_init(argc, argv);

  if (bx_options.load32bitOSImage.whichOS) {
    void bx_load32bitOSimagehack(void);
    bx_load32bitOSimagehack();
    }

  BX_CPU.cpu_loop();
#endif

  fprintf(stderr,"genlog is at 0x%x\n",genlog);
  return(0);
}


  int
bx_bochs_init(int argc, char *argv[])
{
  int n;

#ifdef MAGIC_BREAKPOINT
  bx_dbg.magic_break_enabled = 0;
#endif

  /* read the .bochsrc file */
  parse_bochsrc(argc);

//#if BX_PROVIDE_CPU_MEMORY==1
//    else if (!strcmp(argv[n-1], "-sanity-check")) {
//      BX_CPU.sanity_checks();
//      n += 1;
//      exit(0);
//      }
//#endif

  // Pass all command line options to be parsed,
  // just like they came from the .bochsrc.  Thus
  // command line options will override .bochsrc options.

  n = 2;
  while (n <= argc) {
    parse_line_unformatted(argv[n-1]);
    n++;
    }

  bx_pc_system.init_ips(bx_options.ips);

  if(logfilename[0]!='-') {
    BX_INFO (("all further messages will go to %s\n", logfilename));
    io->init_log(logfilename);
  }

#if BX_DEBUGGER == 0
  // debugger will do this work, if enabled
  BX_CPU.reset(BX_RESET_HARDWARE);
  BX_MEM.init_memory(bx_options.memory.megs * 1024*1024);
  BX_MEM.load_ROM(bx_options.rom.path, bx_options.rom.address);
  BX_MEM.load_ROM(bx_options.vgarom.path, 0xc0000);
#endif

  bx_init_debug();

#if BX_DEBUGGER == 0
  bx_devices.init();
  bx_gui.init_signal_handlers ();
  bx_pc_system.start_timers();
#endif

  BX_INFO(("bx_bochs_init is setting signal handlers\n"));
// if not using debugger, then we can take control of SIGINT.
// If using debugger, it needs control of this.
#if BX_DEBUGGER==0
  signal(SIGINT, bx_signal_handler);
#endif

#if BX_SHOW_IPS
#ifndef __MINGW32__
  signal(SIGALRM, bx_signal_handler);
#endif
  alarm( 1 );
#endif

  return(0);
}



  void
bx_init_debug(void)
{
  bx_dbg.floppy = 0;
  bx_dbg.keyboard = 0;
  bx_dbg.video = 0;
  bx_dbg.disk = 0;
  bx_dbg.pit = 0;
  bx_dbg.pic = 0;
  bx_dbg.bios = 0;
  bx_dbg.cmos = 0;
  bx_dbg.a20 = 0;
  bx_dbg.interrupts = 0;
  bx_dbg.exceptions = 0;
  bx_dbg.unsupported = 0;
  bx_dbg.temp = 0;
  bx_dbg.reset = 0;
  bx_dbg.mouse = 0;
  bx_dbg.io = 0;
  bx_dbg.debugger = 0;
  bx_dbg.xms = 0;
  bx_dbg.v8086 = 0;
  bx_dbg.paging = 0;
  bx_dbg.creg = 0;
  bx_dbg.dreg = 0;
  bx_dbg.dma = 0;
  bx_dbg.unsupported_io = 0;
  bx_dbg.record_io = 0;
  bx_dbg.serial = 0;
  bx_dbg.cdrom = 0;
}


  void
bx_atexit(void)
{
  static Boolean been_here = 0;


#if BX_PROVIDE_DEVICE_MODELS==1
  if (been_here == 0) {
    bx_pc_system.exit();
    }
#endif

#if BX_DEBUGGER == 0
  BX_CPU.atexit();
#endif

#if BX_PCI_SUPPORT
    if (bx_options.i440FXSupport) {
      bx_devices.pci->print_i440fx_state();
      }
#endif
  BX_INFO(("bochs exited, log file was '%s'\n", logfilename));
  bx_print_footer ();
}

#if (BX_PROVIDE_CPU_MEMORY==1) && (BX_EMULATE_HGA_DUMPS>0)
  void
bx_emulate_hga_dumps_timer(void)
{
  void bx_hga_set_video_memory(Bit8u *ptr);

  bx_hga_set_video_memory(&bx_phy_memory[0xb0000]);
}
#endif


#if BX_PROVIDE_MAIN
  static void
parse_bochsrc(int argc)
{
  FILE *fd;
  char *ret;
  char line[512];
  Bit32u retry = 0, found = 0;

  // try several possibilities for the bochsrc before giving up
  while (!found) {
    bochsrc_path[0] = 0;
    switch (retry++) {
    case 0: strcpy (bochsrc_path, ".bochsrc"); break;
    case 1: strcpy (bochsrc_path, "bochsrc"); break;
    case 2: strcpy (bochsrc_path, "bochsrc.txt"); break;
    case 3:
#if (!defined(WIN32) && !defined(macintosh))
      // only try this on unix
      {
      char *ptr = getenv("HOME");
      if (ptr) sprintf (bochsrc_path, "%s/.bochsrc", ptr);
      }
#endif
      break;
    default:
      // no bochsrc used.  This is still legal since they may have 
      // everything on the command line.  However if they have no
      // arguments then give them some friendly advice.
      BX_INFO(( "could not find a bochsrc file\n"));
      if (argc==1) {
	fprintf (stderr, "%s\n", divider);
	fprintf (stderr, "Before running Bochs, you should cd to a directory which contains\n");
	fprintf (stderr, "a .bochsrc file and a disk image.  If you downloaded a binary package,\n");
	fprintf (stderr, "all the necessary files are already on your disk.\n");
#if defined(WIN32)
	fprintf (stderr, "\nFor Windows installations, go to the dlxlinux direectory and\n");
	fprintf (stderr, "double-click on the start.bat script.\n");
#elif !defined(macintosh)
	fprintf (stderr, "\nFor UNIX installations, try running \"bochs-dlx\" for a demo.  This script\n");
	fprintf (stderr, "is basically equivalent to typing:\n");
	fprintf (stderr, "   cd /usr/local/bochs/dlxlinux\n");
	fprintf (stderr, "   bochs\n");
#endif
	exit(1);
      }
      return;
    }
    if (bochsrc_path[0]) {
      BX_INFO (("looking for configuration in %s\n", bochsrc_path));
      fd = fopen(bochsrc_path, "r");
      if (fd) found = 1;
    }
  }
  assert (fd != NULL && bochsrc_path[0] != 0);

  BX_INFO(("reading configuration from %s\n", bochsrc_path));

  do {
    ret = fgets(line, sizeof(line)-1, fd);
    line[sizeof(line) - 1] = '\0';
    line[strlen(line) - 1] = '\0';
    if ((ret != NULL) && strlen(line)) {
      parse_line_unformatted(line);
      }
    } while (!feof(fd));
}

  static void
parse_line_unformatted(char *line)
{
  char *ptr;
  unsigned i, string_i;
  char string[512];
  char *params[40];
  int num_params;
  Boolean inquotes = 0;

  if (line == NULL) return;

  // if passed nothing but whitespace, just return
  for (i=0; i<strlen(line); i++) {
    if (!isspace(line[i])) break;
    }
  if (i>=strlen(line))
    return;

  num_params = 0;

  ptr = strtok(line, ":");
  while (ptr) {
    string_i = 0;
    for (i=0; i<strlen(ptr); i++) {
      if (ptr[i] == '"')
        inquotes = !inquotes;
      else
        if (!isspace(ptr[i]) || inquotes) {
          string[string_i++] = ptr[i];
          }
      }
    string[string_i] = '\0';
    strcpy(ptr, string);
    params[num_params++] = ptr;
    ptr = strtok(NULL, ",");
    }
  parse_line_formatted(num_params, &params[0]);
}

  static void
parse_line_formatted(int num_params, char *params[])
{
  int i;

  if (num_params < 1) return;

  if (params[0][0] == '#') return; /* comment */
  else if (!strcmp(params[0], "floppya")) {
    for (i=1; i<num_params; i++) {
      if (!strncmp(params[i], "2_88=", 5)) {
        strcpy(bx_options.floppya.path, &params[i][5]);
        bx_options.floppya.type = BX_FLOPPY_2_88;
        }
      else if (!strncmp(params[i], "1_44=", 5)) {
        strcpy(bx_options.floppya.path, &params[i][5]);
        bx_options.floppya.type = BX_FLOPPY_1_44;
        }
      else if (!strncmp(params[i], "1_2=", 4)) {
        strcpy(bx_options.floppya.path, &params[i][4]);
        bx_options.floppya.type = BX_FLOPPY_1_2;
        }
      else if (!strncmp(params[i], "720k=", 5)) {
        strcpy(bx_options.floppya.path, &params[i][5]);
        bx_options.floppya.type = BX_FLOPPY_720K;
        }
      else if (!strncmp(params[i], "status=ejected", 14)) {
        bx_options.floppya.initial_status = BX_EJECTED;
        }
      else if (!strncmp(params[i], "status=inserted", 15)) {
        bx_options.floppya.initial_status = BX_INSERTED;
        }
      else {
        fprintf(stderr, ".bochsrc: floppya attribute '%s' not understood.\n",
          params[i]);
        exit(1);
        }
      }
    }

  else if (!strcmp(params[0], "floppyb")) {
    for (i=1; i<num_params; i++) {
      if (!strncmp(params[i], "2_88=", 5)) {
        strcpy(bx_options.floppyb.path, &params[i][5]);
        bx_options.floppyb.type = BX_FLOPPY_2_88;
        }
      else if (!strncmp(params[i], "1_44=", 5)) {
        strcpy(bx_options.floppyb.path, &params[i][5]);
        bx_options.floppyb.type = BX_FLOPPY_1_44;
        }
      else if (!strncmp(params[i], "1_2=", 4)) {
        strcpy(bx_options.floppyb.path, &params[i][4]);
        bx_options.floppyb.type = BX_FLOPPY_1_2;
        }
      else if (!strncmp(params[i], "720k=", 5)) {
        strcpy(bx_options.floppyb.path, &params[i][5]);
        bx_options.floppyb.type = BX_FLOPPY_720K;
        }
      else if (!strncmp(params[i], "status=ejected", 14)) {
        bx_options.floppyb.initial_status = BX_EJECTED;
        }
      else if (!strncmp(params[i], "status=inserted", 15)) {
        bx_options.floppyb.initial_status = BX_INSERTED;
        }
      else {
        fprintf(stderr, ".bochsrc: floppyb attribute '%s' not understood.\n",
          params[i]);
        exit(1);
        }
      }
    }

  else if (!strcmp(params[0], "diskc")) {
    if (num_params != 5) {
      fprintf(stderr, ".bochsrc: diskc directive malformed.\n");
      exit(1);
      }
    if (strncmp(params[1], "file=", 5) ||
        strncmp(params[2], "cyl=", 4) ||
        strncmp(params[3], "heads=", 6) ||
        strncmp(params[4], "spt=", 4)) {
      fprintf(stderr, ".bochsrc: diskc directive malformed.\n");
      exit(1);
      }
    strcpy(bx_options.diskc.path, &params[1][5]);
    bx_options.diskc.cylinders = atol( &params[2][4] );
    bx_options.diskc.heads     = atol( &params[3][6] );
    bx_options.diskc.spt       = atol( &params[4][4] );
    bx_options.diskc.present = 1;
    }
  else if (!strcmp(params[0], "diskd")) {
    if (num_params != 5) {
      fprintf(stderr, ".bochsrc: diskd directive malformed.\n");
      exit(1);
      }
    if (strncmp(params[1], "file=", 5) ||
        strncmp(params[2], "cyl=", 4) ||
        strncmp(params[3], "heads=", 6) ||
        strncmp(params[4], "spt=", 4)) {
      fprintf(stderr, ".bochsrc: diskd directive malformed.\n");
      exit(1);
      }
    strcpy(bx_options.diskd.path, &params[1][5]);
    bx_options.diskd.cylinders = atol( &params[2][4] );
    bx_options.diskd.heads     = atol( &params[3][6] );
    bx_options.diskd.spt       = atol( &params[4][4] );
    bx_options.diskd.present = 1;
    }

  else if (!strcmp(params[0], "cdromd")) {
    if (num_params != 3) {
      fprintf(stderr, ".bochsrc: cdromd directive malformed.\n");
      exit(1);
      }
    if (strncmp(params[1], "dev=", 4) || strncmp(params[2], "status=", 7)) {
      fprintf(stderr, ".bochsrc: cdromd directive malformed.\n");
      exit(1);
      }
    strcpy(bx_options.cdromd.dev, &params[1][4]);
    if (!strcmp(params[2], "status=inserted"))
      bx_options.cdromd.inserted = 1;
    else if (!strcmp(params[2], "status=ejected"))
      bx_options.cdromd.inserted = 0;
    else {
      fprintf(stderr, ".bochsrc: cdromd directive malformed.\n");
      exit(1);
      }
    bx_options.cdromd.present = 1;
    }

  else if (!strcmp(params[0], "boot")) {
    if (!strcmp(params[1], "a") ||
        !strcmp(params[1], "c")) {
      strcpy(bx_options.bootdrive, params[1]);
      }
    else {
      fprintf(stderr, ".bochsrc: boot directive with unknown boot device '%s'.\n",
        params[1]);
      fprintf(stderr, "          use 'a', or 'c'.\n");
      exit(1);
      }
    }
  else if (!strcmp(params[0], "log")) {
    if (num_params != 2) {
      fprintf(stderr, ".bochsrc: log directive has wrong # args.\n");
      exit(1);
      }
    strcpy(logfilename, params[1]);
    }
  else if (!strcmp(params[0], "panic")) {
    if (num_params != 2) {
      fprintf(stderr, ".bochsrc: panic directive malformed.\n");
      exit(1);
      }
    if (strncmp(params[1], "action=", 7)) {
      fprintf(stderr, ".bochsrc: panic directive malformed.\n");
      exit(1);
      }
    char *action = 7 + params[1];
    if (!strcmp(action, "crash"))
      bx_options.log_actions[LOGLEV_PANIC] = 2;
    else if (!strcmp (action, "report"))
      bx_options.log_actions[LOGLEV_PANIC] = 1;
    else if (!strcmp (action, "ignore"))
      bx_options.log_actions[LOGLEV_PANIC] = 0;
    else {
      fprintf(stderr, ".bochsrc: panic directive malformed.\n");
      exit(1);
      }
    }
  else if (!strcmp(params[0], "error")) {
    if (num_params != 2) {
      fprintf(stderr, ".bochsrc: error directive malformed.\n");
      exit(1);
      }
    if (strncmp(params[1], "action=", 7)) {
      fprintf(stderr, ".bochsrc: error directive malformed.\n");
      exit(1);
      }
    char *action = 7 + params[1];
    if (!strcmp(action, "crash"))
      bx_options.log_actions[LOGLEV_ERROR] = 2;
    else if (!strcmp (action, "report"))
      bx_options.log_actions[LOGLEV_ERROR] = 1;
    else if (!strcmp (action, "ignore"))
      bx_options.log_actions[LOGLEV_ERROR] = 0;
    else {
      fprintf(stderr, ".bochsrc: error directive malformed.\n");
      exit(1);
      }
    }
  else if (!strcmp(params[0], "info")) {
    if (num_params != 2) {
      fprintf(stderr, ".bochsrc: info directive malformed.\n");
      exit(1);
      }
    if (strncmp(params[1], "action=", 7)) {
      fprintf(stderr, ".bochsrc: info directive malformed.\n");
      exit(1);
      }
    char *action = 7 + params[1];
    if (!strcmp(action, "crash"))
      bx_options.log_actions[LOGLEV_INFO] = 2;
    else if (!strcmp (action, "report"))
      bx_options.log_actions[LOGLEV_INFO] = 1;
    else if (!strcmp (action, "ignore"))
      bx_options.log_actions[LOGLEV_INFO] = 0;
    else {
      fprintf(stderr, ".bochsrc: info directive malformed.\n");
      exit(1);
      }
    }
  else if (!strcmp(params[0], "debug")) {
    if (num_params != 2) {
      fprintf(stderr, ".bochsrc: debug directive malformed.\n");
      exit(1);
      }
    if (strncmp(params[1], "action=", 7)) {
      fprintf(stderr, ".bochsrc: debug directive malformed.\n");
      exit(1);
      }
    char *action = 7 + params[1];
    if (!strcmp(action, "crash"))
      bx_options.log_actions[LOGLEV_DEBUG] = 2;
    else if (!strcmp (action, "report"))
      bx_options.log_actions[LOGLEV_DEBUG] = 1;
    else if (!strcmp (action, "ignore"))
      bx_options.log_actions[LOGLEV_DEBUG] = 0;
    else {
      fprintf(stderr, ".bochsrc: debug directive malformed.\n");
      exit(1);
      }
    }
  else if (!strcmp(params[0], "romimage")) {
    if (num_params != 3) {
      fprintf(stderr, ".bochsrc: romimage directive: wrong # args.\n");
      exit(1);
      }
    if (strncmp(params[1], "file=", 5)) {
      fprintf(stderr, ".bochsrc: romimage directive malformed.\n");
      exit(1);
      }
    if (strncmp(params[2], "address=", 8)) {
      fprintf(stderr, ".bochsrc: romimage directive malformed.\n");
      exit(1);
      }
    bx_options.rom.path    = strdup(&params[1][5]);
    if ( (params[2][8] == '0') && (params[2][9] == 'x') )
      bx_options.rom.address = strtoul(&params[2][8], NULL, 16);
    else
      bx_options.rom.address = strtoul(&params[2][8], NULL, 10);
    }
  else if (!strcmp(params[0], "vgaromimage")) {
    if (num_params != 2) {
      fprintf(stderr, ".bochsrc: vgaromimage directive: wrong # args.\n");
      exit(1);
      }
    bx_options.vgarom.path = strdup(params[1]);
    }
  else if (!strcmp(params[0], "vga_update_interval")) {
    if (num_params != 2) {
      fprintf(stderr, ".bochsrc: vga_update_interval directive: wrong # args.\n");
      exit(1);
      }
    bx_options.vga_update_interval = atol(params[1]);
    if (bx_options.vga_update_interval < 50000) {
      fprintf(stderr, ".bochsrc: vga_update_interval not big enough!\n");
      exit(1);
      }
    }
  else if (!strcmp(params[0], "keyboard_serial_delay")) {
    if (num_params != 2) {
      fprintf(stderr, ".bochsrc: keyboard_serial_delay directive: wrong # args.\n");
      exit(1);
      }
    bx_options.keyboard_serial_delay = atol(params[1]);
    if (bx_options.keyboard_serial_delay < 5) {
      fprintf(stderr, ".bochsrc: keyboard_serial_delay not big enough!\n");
      exit(1);
      }
    }
  else if (!strcmp(params[0], "megs")) {
    if (num_params != 2) {
      fprintf(stderr, ".bochsrc: megs directive: wrong # args.\n");
      exit(1);
      }
    bx_options.memory.megs = atol(params[1]);
    }
  else if (!strcmp(params[0], "floppy_command_delay")) {
    if (num_params != 2) {
      fprintf(stderr, ".bochsrc: floppy_command_delay directive: wrong # args.\n");
      exit(1);
      }
    bx_options.floppy_command_delay = atol(params[1]);
    if (bx_options.floppy_command_delay < 100) {
      fprintf(stderr, ".bochsrc: floppy_command_delay not big enough!\n");
      exit(1);
      }
    }
  else if (!strcmp(params[0], "ips")) {
    if (num_params != 2) {
      fprintf(stderr, ".bochsrc: ips directive: wrong # args.\n");
      exit(1);
      }
    bx_options.ips = atol(params[1]);
    if (bx_options.ips < 200000) {
      fprintf(stderr, ".bochsrc: WARNING: ips is AWFULLY low!\n");
      }
    }

  else if (!strcmp(params[0], "mouse")) {
    if (num_params != 2) {
      fprintf(stderr, ".bochsrc: mouse directive malformed.\n");
      exit(1);
      }
    if (strncmp(params[1], "enabled=", 8)) {
      fprintf(stderr, ".bochsrc: mouse directive malformed.\n");
      exit(1);
      }
    if (params[1][8] == '0')
      bx_options.mouse_enabled = 0;
    else if (params[1][8] == '1')
      bx_options.mouse_enabled = 1;
    else {
      fprintf(stderr, ".bochsrc: mouse directive malformed.\n");
      exit(1);
      }
    }
  else if (!strcmp(params[0], "private_colormap")) {
    if (num_params != 2) {
      fprintf(stderr, ".bochsrc: private_colormap directive malformed.\n");
      exit(1);
      }
    if (strncmp(params[1], "enabled=", 8)) {
      fprintf(stderr, ".bochsrc: private_colormap directive malformed.\n");
      exit(1);
      }
    if (params[1][8] == '0')
      bx_options.private_colormap = 0;
    else if (params[1][8] == '1')
      bx_options.private_colormap = 1;
    else {
      fprintf(stderr, ".bochsrc: private_colormap directive malformed.\n");
      exit(1);
      }
    }

  else if (!strcmp(params[0], "sb16")) {
    for (i=1; i<num_params; i++) {
      if (!strncmp(params[i], "midi=", 5)) {
	bx_options.sb16.midifile = strdup(&params[i][5]);
        }
      else if (!strncmp(params[i], "midimode=", 9)) {
	bx_options.sb16.midimode = atol(&params[i][9]);
        }
      else if (!strncmp(params[i], "wave=", 5)) {
	bx_options.sb16.wavefile = strdup(&params[i][5]);
        }
      else if (!strncmp(params[i], "wavemode=", 9)) {
	bx_options.sb16.wavemode = atol(&params[i][9]);
        }
      else if (!strncmp(params[i], "log=", 4)) {
	bx_options.sb16.logfile = strdup(&params[i][4]);
        }
      else if (!strncmp(params[i], "loglevel=", 9)) {
	bx_options.sb16.loglevel = atol(&params[i][9]);
        }
      else if (!strncmp(params[i], "dmatimer=", 9)) {
	bx_options.sb16.dmatimer = atol(&params[i][9]);
        }
      }
    }

  else if (!strcmp(params[0], "i440fxsupport")) {
    if (num_params != 2) {
      fprintf(stderr, ".bochsrc: i440FXSupport directive malformed.\n");
      exit(1);
      }
    if (strncmp(params[1], "enabled=", 8)) {
      fprintf(stderr, ".bochsrc: i440FXSupport directive malformed.\n");
      exit(1);
      }
    if (params[1][8] == '0')
      bx_options.i440FXSupport = 0;
    else if (params[1][8] == '1')
      bx_options.i440FXSupport = 1;
    else {
      fprintf(stderr, ".bochsrc: i440FXSupport directive malformed.\n");
      exit(1);
      }
    }
  else if (!strcmp(params[0], "newharddrivesupport")) {
    if (num_params != 2) {
      fprintf(stderr, ".bochsrc: newharddrivesupport directive malformed.\n");
      exit(1);
      }
    if (strncmp(params[1], "enabled=", 8)) {
      fprintf(stderr, ".bochsrc: newharddrivesupport directive malformed.\n");
      exit(1);
      }
    if (params[1][8] == '0')
      bx_options.newHardDriveSupport = 0;
    else if (params[1][8] == '1')
      bx_options.newHardDriveSupport = 1;
    else {
      fprintf(stderr, ".bochsrc: newharddrivesupport directive malformed.\n");
      exit(1);
      }
    }
  else if (!strcmp(params[0], "cmosimage")) {
    if (num_params != 2) {
      fprintf(stderr, ".bochsrc: cmosimage directive: wrong # args.\n");
      exit(1);
      }
    bx_options.cmos.path = strdup(params[1]);
    bx_options.cmos.cmosImage = 1;                // CMOS Image is true
    }
  else if (!strcmp(params[0], "time0")) {
    if (num_params != 2) {
      fprintf(stderr, ".bochsrc: time0 directive: wrong # args.\n");
      exit(1);
      }
    bx_options.cmos.time0 = atoi(params[1]);
    }
#ifdef MAGIC_BREAKPOINT
  else if (!strcmp(params[0], "magic_break")) {
    if (num_params != 2) {
      fprintf(stderr, ".bochsrc: magic_break directive: wrong # args.\n");
      exit(1);
      }
    if (strncmp(params[1], "enabled=", 8)) {
      fprintf(stderr, ".bochsrc: magic_break directive malformed.\n");
      exit(1);
      }
    if (params[1][8] == '0') {
      fprintf(stderr, "Ignoring magic break points\n");
      bx_dbg.magic_break_enabled = 0;
      }
    else if (params[1][8] == '1') {
      fprintf(stderr, "Stopping on magic break points\n");
      bx_dbg.magic_break_enabled = 1;
      }
    else {
      fprintf(stderr, ".bochsrc: magic_break directive malformed.\n");
      exit(1);
      }
    }
#endif
  else if (!strcmp(params[0], "ne2k")) {
    int tmp[6];
    bx_options.ne2k.valid = 0;
    if ((num_params < 4) || (num_params > 6)) {
      fprintf(stderr, ".bochsrc: ne2k directive malformed.\n");
      exit(1);
      }
    bx_options.ne2k.ethmod = "null";
    if (strncmp(params[1], "ioaddr=", 7)) {
      fprintf(stderr, ".bochsrc: ne2k directive malformed.\n");
      exit(1);
      }
    if (strncmp(params[2], "irq=", 4)) {
      fprintf(stderr, ".bochsrc: ne2k directive malformed.\n");
      exit(1);
      }
    if (strncmp(params[3], "mac=", 4)) {
      fprintf(stderr, ".bochsrc: ne2k directive malformed.\n");
      exit(1);
      }
    bx_options.ne2k.ioaddr = strtoul(&params[1][7], NULL, 16);
    bx_options.ne2k.irq = atol(&params[2][4]);
    i = sscanf(&params[3][4], "%x:%x:%x:%x:%x:%x",
             &tmp[0],&tmp[1],&tmp[2],&tmp[3],&tmp[4],&tmp[5]);
    if (i != 6) {
      fprintf(stderr, ".bochsrc: ne2k mac address malformed.\n");
      exit(1);
      }
    for (i=0;i<6;i++)
      bx_options.ne2k.macaddr[i] = tmp[i];
    if (num_params > 4) {
      if (strncmp(params[4], "ethmod=", 7)) {
      fprintf(stderr, ".bochsrc: ne2k directive malformed.\n");
      exit(1);
        }
      bx_options.ne2k.ethmod = strdup(&params[4][7]);
      if (num_params == 6) {
      if (strncmp(params[5], "ethdev=", 7)) {
      fprintf(stderr, ".bochsrc: ne2k directive malformed.\n");
      exit(1);
          }
      bx_options.ne2k.ethdev = strdup(&params[5][7]);
        }
      }
    bx_options.ne2k.valid = 1;
    }

  else if (!strcmp(params[0], "load32bitOSImage")) {
    if ( (num_params!=4) && (num_params!=5) ) {
      fprintf(stderr, ".bochsrc: load32bitOSImage directive: wrong # args.\n");
      exit(1);
      }
    if (strncmp(params[1], "os=", 3)) {
      fprintf(stderr, ".bochsrc: load32bitOSImage: directive malformed.\n");
      exit(1);
      }
    if (!strcmp(&params[1][3], "nullkernel")) {
      bx_options.load32bitOSImage.whichOS = Load32bitOSNullKernel;
      }
    else if (!strcmp(&params[1][3], "linux")) {
      bx_options.load32bitOSImage.whichOS = Load32bitOSLinux;
      }
    else {
      fprintf(stderr, ".bochsrc: load32bitOSImage: unsupported OS.\n");
      exit(1);
      }
    if (strncmp(params[2], "path=", 5)) {
      fprintf(stderr, ".bochsrc: load32bitOSImage: directive malformed.\n");
      exit(1);
      }
    if (strncmp(params[3], "iolog=", 6)) {
      fprintf(stderr, ".bochsrc: load32bitOSImage: directive malformed.\n");
      exit(1);
      }
    bx_options.load32bitOSImage.path = strdup(&params[2][5]);
    bx_options.load32bitOSImage.iolog = strdup(&params[3][6]);
    if (num_params == 5) {
      if (strncmp(params[4], "initrd=", 7)) {
        fprintf(stderr, ".bochsrc: load32bitOSImage: directive malformed.\n");
        exit(1);
        }
      bx_options.load32bitOSImage.initrd = strdup(&params[4][7]);
      }
    }

  else {
    BX_PANIC(( ".bochsrc: directive '%s' not understood\n", params[0]));
    }

  if (bx_options.diskd.present && bx_options.cdromd.present)
    BX_PANIC(("At present, using both diskd and cdromd at once is not supported."));
}
#endif // #if BX_PROVIDE_MAIN

  void
bx_signal_handler( int signum)
{
#if BX_GUI_SIGHANDLER
  // GUI signal handler gets first priority, if the mask says it's wanted
  if ((1<<signum) & bx_gui.get_sighandler_mask ()) {
    bx_gui.sighandler (signum);
    return;
  }
#endif

#if BX_SHOW_IPS
  extern unsigned long ips_count;

  if (signum == SIGALRM ) {
    BX_INFO((("ips = %lu\n", ips_count));
    ips_count = 0;
#ifndef __MINGW32__
    signal(SIGALRM, bx_signal_handler);
    alarm( 1 );
#endif
    return;
    }
#endif

  BX_PANIC(("SIGNAL %u caught\n", signum));
}
