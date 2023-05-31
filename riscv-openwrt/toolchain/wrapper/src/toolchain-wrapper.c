/*
 * toolchain-wrapper.c - external GNU toolchain wrapper
 *
 * Copyright (C) 2021 Ye Holmes <yeholmes@outlook.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

/*
 * toolchain-wrapper aims to wrapper
 * following GNU toolchain executables:
 *
 * CROSS_PREFIX-gcc
 * CROSS_PREFIX-gcc-X.Y.Z
 * CROSS_PREFIX-c++
 * CROSS_PREFIX-g++
 * CROSS_PREFIX-gfortran
 *
 * If the wrapper thinks it necessay to add compilation or
 * linking flags, it will insert the arguments after argv[0]:
 *
 *      ARGV[0] { LIST OF INSERTED COMPILATION ARGS } ARGV[1] ARGV[2] ...
 *
 * No arguments are modified or removed from the original argument array.
 */

/*
 * wrapper command-line argument information
 */
struct wrapper_info {
	int              orig_argc;       /* argc from main(...) */
	char * *         orig_argv;       /* argv from main(...) */
	char *           orig_name;       /* invoked application base name of argv[0] */
	char *           real_path;       /* absolute path of real executable */
	const char *     staging_dir;     /* $ENV{STAGING_DIR} */
	const char *     output;          /* argument given by -o, points to the output file name */
	int              total_argc;      /* total number of new arguments avaiable from `new_argv */
	int              new_argc;        /* number of new arguments added in the argument array */
	char * *         new_argv;        /* new arguments added in the front of argument array */

	/* command-line arguments traits */
	unsigned int     found_l : 1;           /* -l* found in the argument array */
	unsigned int     found_L : 1;           /* -L* found in the argument array */
	unsigned int     f_nostdinc : 1;        /* -nostdinc found in the argument array */
	unsigned int     f_nostdincpp : 1;      /* -nostdinc++ found in the argument array */
	unsigned int     f_nostdlib : 1;        /* -Wl,-nostdlib found in the argument array */
	unsigned int     f_nolink : 1;          /* -c/-S/-E found in the argument array */
	unsigned int     d_kernel : 1;          /* -D__KERNEL__ found */
	unsigned int     d_uboot : 1;           /* -D__UBOOT__ found */
};

/* check for the absolute path of external toolchain */
#ifndef TCW_ROOT
#error TCW_ROOT not defined
#endif

/* sub-directory containing all the GNU toolchain binaries */
#ifndef TCW_BINDIR
#define TCW_BINDIR      "bin"
#endif

/* debug definitions for toolchain-wrapper */
#define TCW_DEBUG_NEW   1
#define TCW_DEBUG_ALL   2

/* initialize wrapper runtime information */
static int wrapper_init(struct wrapper_info * pwi,
	int argc, char * argv[]);

/* process wrapper runtime information */
static void wrapper_process(struct wrapper_info * pwi);

/* merge command-line arguments */
static int wrapper_merge(struct wrapper_info * pwi);

/* debug wrapper information */
static void wrapper_debug(struct wrapper_info * pwi);

/* destroy wrapper runtime information */
static void wrapper_destroy(struct wrapper_info * pwi);

/* emit an error message, then exit */
static void fatal_die(const char * errmsg, int val);

int main(int argc, char *argv[])
{
	struct wrapper_info wi;

	memset(&wi, 0, sizeof(wi));
	if (wrapper_init(&wi, argc, argv) < 0) {
		wrapper_destroy(&wi);
		return 1;
	}

	wrapper_process(&wi);

	if (wrapper_merge(&wi) < 0) {
		wrapper_destroy(&wi);
		return 2;
	}

	wrapper_debug(&wi);

	/* invoke the real GNU toolchain executable  */
	execv(wi.real_path, wi.new_argv);
	fprintf(stderr, "Error, failed to invoke %s: %s\n",
		wi.orig_name, strerror(errno));
	fflush(stderr);

	wrapper_destroy(&wi);
	return 3;
}

void fatal_die(const char * errmsg, int val)
{
	if (errmsg == NULL)
		errmsg = "Error, system out of memory!";
	fprintf(stderr, "Wrapper: %s\n", errmsg);
	fflush(stderr);
	if (val != 0) exit(val);
}

/* dump the command-line argument array */
static void wrapper_dump_argv(int argc, char * * argv)
{
	int i;
	size_t tlen;
	const char * arg;

	tlen = 0;
	for (i = 0; i < argc; ++i) {
		size_t arglen;
		arg = argv[i];

		if (arg == NULL)
			break;

		if (arg[0] == '\0') {
			/* zero-length argument */
			arglen = 2;
			arg = "\"\"";
		} else
			arglen = strlen(arg);

		arglen++; /* for the trailing space */
		if ((tlen + arglen) >= 80) {
			tlen = arglen = 0;
			fprintf(stderr, "%s\n%s", arg,
				(i + 1) >= argc ? "" : "\t");
		} else {
			fputs(arg, stderr);
			fputc(' ', stderr);
			tlen += arglen;
		}
	}
	if (tlen != 0)
		fputc('\n', stderr);
	fflush(stderr);
}

void wrapper_debug(struct wrapper_info * pwi)
{
	unsigned long opt;
	char * next, * semi;
	const char * debug, * old;

	opt = 0;
	debug = getenv("TCW_DEBUG");
	if (debug == NULL)
		return;

	old = debug; /* backup debug string pointer */
	semi = strchr(debug, ';');
	if (semi == debug)
		goto err0;

	if (semi != NULL) {
		size_t outlen;
		const char * bn;

		/* we're tracing specific output file */
		if (pwi->output == NULL)
			return;

		/*
		 * fetch basename for output file,
		 * do not use `basename(char * path) because, from man-page:
		 * Both dirname() and basename() may modify the contents of path...
		 */
		bn = strrchr(pwi->output, '/');
		if (bn == NULL)
			bn = pwi->output;
		else
			bn++;

		outlen = semi - debug;
		/* basename compare of output file */
		if (strncmp(semi, bn, outlen) != 0)
			return;

		/* check again */
		if (bn[outlen] != '\0')
			return;

		semi++;
		debug = semi;
	}

	errno = 0;
	next = NULL;
	/* pase the debug integer option */
	opt = strtoul(debug, &next, 0);
	if (errno != 0 || next == debug)
		goto err0;

	/* dump the command-line arguments */
	if (opt & TCW_DEBUG_ALL)
		wrapper_dump_argv(pwi->orig_argc + pwi->new_argc, pwi->new_argv);
	else if (opt & TCW_DEBUG_NEW) {
		if (pwi->new_argc == 0) {
			fprintf(stderr, "Wrapper: no arguments added: %s\n", pwi->orig_name);
			fflush(stderr);
		} else
			wrapper_dump_argv(pwi->new_argc + 1, pwi->new_argv);
	}
	return;

err0:
	fprintf(stderr, "Warning, invalid TCW_DEBUG: %s\n", old);
	fflush(stderr);
}

/* function to finally merge the argument array */
static int wrapper_merge_(struct wrapper_info * pwi)
{
	int i, argc, argt;

	argc = pwi->new_argc;
	argt = pwi->total_argc;
	if ((argt - argc) <= pwi->orig_argc) {
		int args;
		char * * new_argv;

		/* allocate new argument array */
		args = argc + pwi->orig_argc + 3;
		new_argv = (char * *) realloc((void *) pwi->new_argv,
			(size_t) (args * sizeof(char *)));
		if (new_argv == NULL)
			fatal_die(NULL, 4);
		pwi->total_argc = args;
		pwi->new_argv = new_argv;
	}

	pwi->new_argv[0] = pwi->real_path;
	/* copy the original command-line arguments */
	for (i = 1; i < pwi->orig_argc; ++i)
		pwi->new_argv[argc + i] = pwi->orig_argv[i];

	argc += pwi->orig_argc;
	/* Do not update pwi->new_argc, for debug purpose */
	/* pwi->new_argc = argc; */
	pwi->new_argv[argc] = NULL;
	return 0;
}

/* function to add a compilation argument */
static void wrapper_addarg(struct wrapper_info * pwi,
	char * new_arg, int alloc)
{
	int argc, argt;

	argc = pwi->new_argc;
	argt = pwi->total_argc;

	/* need to allocate new memory to hold new argument ? */
	if ((argc + 1) >= argt) {
		int args;
		char * * new_argv;

		args = (argt < 0x3) ? 0x3 : (argt << 1);
		new_argv = (char * *) realloc((void *) pwi->new_argv,
			(size_t) (args * sizeof(char *)));
		if (new_argv == NULL)
			fatal_die(NULL, 4);

		/* the new_argv is indexed from 1, not 0 */
		pwi->total_argc = args;
		pwi->new_argv = new_argv;
	}

	/* store a copy of the new argument */
	if (alloc == 0) {
		pwi->new_argv[argc + 1] = new_arg;
	} else {
		pwi->new_argv[argc + 1] = strdup(new_arg);
		if (pwi->new_argv[argc + 1] == NULL)
			fatal_die(NULL, 4);
	}
	pwi->new_argc = argc + 1;
}

#ifdef TCW_CFLAGS
/*
 * split a string such as "-Os -pipe -mcpu=cortex-a53",
 * add the compliation arguments one by one.
 */
static void wrapper_add_cflags(struct wrapper_info * pwi, const char * flags_)
{
	char * flags, * p0, * p1;

	/* skip empty string of compilation flags */
	if (flags_[0] == '\0')
		return;

	/* string duplication is needed becase we need to modify it */
	flags = strdup(flags_);
	if (flags == NULL)
		fatal_die(NULL, 4);

	p0 = flags;
	/* skip spaces or tabs */
	while (*p0 == ' ' || *p0 == '\t')
		p0++;

	/*
	 * naïve way to split a string of compilation arguments.
	 * TODO: find a better way to split string
	 */
	while (*p0 != '\0') {
		char cha;

		p1 = p0;
		cha = *p1;
		while (cha != ' ' && cha != '\t' && cha != '\0') {
			p1++;
			cha = *p1;
		}

		/* p0 != p1 */
		if (cha == '\0') {
			wrapper_addarg(pwi, p0, 1);
			break;
		}

		*p1++ = '\0'; /* terminate the compilation flag */
		wrapper_addarg(pwi, p0, 1);

		cha = *p1;
		/* again, skip spaces or tabs */
		while (cha == ' ' || cha == '\t') {
			p1++;
			cha = *p1;
		}
		p0 = p1;
	}
	free(flags);
}
#endif

int wrapper_merge(struct wrapper_info * pwi)
{
	/* for kernel and u-boot compilation, do not add any arguments */
	if (pwi->d_kernel || pwi->d_uboot)
		return wrapper_merge_(pwi);

#ifdef TCW_CFLAGS
	wrapper_add_cflags(pwi, TCW_CFLAGS);
#endif

	/* no standard C/C++ header inclusion */
	if (pwi->f_nostdinc || pwi->f_nostdincpp)
		goto link_dirs;

	if (pwi->staging_dir != NULL) {
		int ret;
		char idir[64], * incdir;

		/* Add compilation flag: -idirfater $(STAGING_DIR)/usr/include */
		strcpy(idir, "-idirafter");
		wrapper_addarg(pwi, idir, 1);

		incdir = NULL;
		ret = asprintf(&incdir, "%s/usr/include", pwi->staging_dir);
		if (ret <= 0) {
			fatal_die(NULL, 0);
			return -1;
		}
		wrapper_addarg(pwi, incdir, 0);
	}

link_dirs:
	if (pwi->f_nolink != 0)
		/* preprocessor needs no linker options */
		return wrapper_merge_(pwi);

	if (pwi->found_l == 0 && pwi->found_L == 0) {
		/*
		 * no extra library needed,
		 * nether -l* nor -L* found in the command-line arguments.
		 */
		return wrapper_merge_(pwi);
	}

	if (pwi->f_nostdlib == 0 && pwi->staging_dir != NULL) {
		int ret;
		char * libdir = NULL;

		/* Add linking flag: -L$(STAGING_DIR)/usr/lib */
		ret = asprintf(&libdir, "-L%s/usr/lib", pwi->staging_dir);
		if (ret <= 0) {
			fatal_die(NULL, 0);
			return -1;
		}
		wrapper_addarg(pwi, libdir, 0);

		libdir = NULL;
		/* also for the linker library searching */
		ret = asprintf(&libdir, "-Wl,-rpath-link=%s/usr/lib",
			pwi->staging_dir);
		if (ret <= 0) {
			fatal_die(NULL, 0);
			return -1;
		}
		wrapper_addarg(pwi, libdir, 0);
	}

	return wrapper_merge_(pwi);
}

void wrapper_process(struct wrapper_info * pwi)
{
	int idx, argc;
	char * * argv;

	argc = pwi->orig_argc;
	argv = pwi->orig_argv;
	for (idx = 1; idx < argc; ++idx) {
		const char * arg;

		arg = argv[idx];
		if (arg[0] != '-')
			continue;

		switch (arg[1]) {
		case 'c':
		case 'E':
		case 'S':
			if (arg[2] == '\0')
				pwi->f_nolink = 1;
			break;

		case 'l':
			pwi->found_l = 1;
			break;

		case 'L':
			pwi->found_L = 1;
			break;

		case 'n':
			if (strcmp(arg, "-nostdinc") == 0)
				pwi->f_nostdinc = 1;
			else if (strcmp(arg, "-nostdinc++") == 0)
				pwi->f_nostdincpp = 1;
			else if (strcmp(arg, "-nostdlib") == 0) {
				/* possibly: -Xlinker -nostdlib */
				pwi->f_nostdlib = 1;
			}
			break;

		case 'D':
			/*
			 * check only for -D__KERNEL__ or -D__UBOOT__,
			 * the following macro definitions are ignored:
			 * -D__KERNEL__=XXX and -D __KERNEL__[=XXX]
			 * -D__UBOOT__=XXX and -D __UBOOT__[=XXX]
			 */
			if (arg[2] == '_' && arg[3] == '_') {
				if (strcmp(arg, "-D__KERNEL__") == 0)
					pwi->d_kernel = 1;
				else if (strcmp(arg, "-D__UBOOT__") == 0)
					pwi->d_uboot = 1;
			}
			break;

		case 'o':
			if (arg[2] == '\0' && (idx + 1) < argc) {
				idx++;
				/* store output file */
				pwi->output = argv[idx];
			}
			break;

		case 'W':
			if (arg[2] == 'l' &&
				arg[3] == ',' &&
				strcmp(arg, "-Wl,-nostdlib") == 0)
				pwi->f_nostdlib = 1;
			break;

		default:
			break;
		}
	}
}

void wrapper_destroy(struct wrapper_info * pwi)
{
	if (pwi->real_path != NULL) {
		free(pwi->real_path);
		pwi->real_path = NULL;
	}

	if (pwi->new_argc > 0) {
		int idx;

		for (idx = 1; idx <= pwi->new_argc; ++idx) {
			free(pwi->new_argv[idx]);
			pwi->new_argv[idx] = NULL;
		}

		pwi->new_argc = 0;
		pwi->total_argc = 0;
		free((void *) pwi->new_argv);
		pwi->new_argv = NULL;
	}
}

int wrapper_init(struct wrapper_info * pwi,
	int argc, char * argv[])
{
	char * arg0;
	const char * prefix;

	pwi->orig_argc = argc;
	pwi->orig_argv = (char * *) argv;
	/* don't use basename(...), as it might modify argv[0] */
	arg0 = strrchr(argv[0], '/');
	if (arg0 != NULL)
		arg0++;
	else
		arg0 = argv[0];
	pwi->orig_name = arg0;

	prefix = TCW_ROOT "/" TCW_BINDIR;
	/* construct the absolute path of real executable */
	if (asprintf(&(pwi->real_path), "%s/%s", prefix, arg0) <= 0)
		fatal_die(NULL, 5);

	/* check whether the real executable exists  */
	if (access(pwi->real_path, X_OK) != 0) {
		fprintf(stderr, "Error, toolchain not found: %s\n", pwi->real_path);
		fflush(stderr);
		return -1;
	}

	/* find staging_dir via environment variable */
	pwi->staging_dir = getenv("STAGING_DIR");
	if (pwi->staging_dir == NULL) {
		fputs("Warning, STAGING_DIR not defined!\n", stderr);
		fflush(stderr);
	}
	return 0;
}
