
LIB=	c

VPATH=	$(SRCDIR) $(SRCDIR)cpu/$(CPU)

SRCS=	abort.c abs.c assert.c atexit.c atoi.c atol.c atoll.c bsearch.c \
	btowc.c chdir.c clearerr.c clock.c close.c closedir.c creat.c crypt.c \
	div.c dma_memcpy.c __dtostr.c errlistu.c errno_list.c \
	errno_location.c execl.c execlp.c execv.c execve.c execvp.c fclose.c \
	fdglue2.c fdglue.c fdopen.c fdprintf.c feof.c ferror.c fflush.c ffs.c \
	fgetc_unlocked.c fgetpos.c fgets.c fileno.c fopen.c fork.c fprintf.c \
	fputc_unlocked.c fputs.c fread.c freopen.c fscanf.c fseek.c fseeko.c \
	fsetpos.c ftell.c ftello.c fwrite.c getcwd.c getenv.c getopt.c \
	getopt_data.c getopt_long.c getopt_long_only.c getpid.c gettimeofday.c \
	heap_manager.c iprintf.c isalnum.c isalpha.c isascii.c isatty.c \
	isblank.c iscntrl.c isdigit.c isgraph.c islower.c isprint.c ispunct.c \
	isspace.c isupper.c iswalnum.c iswalpha.c iswblank.c iswcntrl.c \
	iswdigit.c iswgraph.c iswlower.c iswprint.c iswpunct.c iswspace.c \
	iswupper.c iswxdigit.c isxdigit.c kill.c ldiv.c llabs.c lldiv.c \
	__lltostr.c lseek.c __ltostr.c madvise.c mblen.c mbrlen.c mbrtowc.c \
	mbsinit.c mbsrtowcs.c mbstowcs.c mbtowc.c mcntl.c md5.c md5crypt.c \
	memccpy.c memchr.c memcmp.c memcpy.c memmem.c memmove.c memrchr.c \
	memset.c mkdir.c mkfifo.c mkstemp.c mmap.c munmap.c open.c opendir.c \
	perror.c pipe.c printf.c putchar.c putenv.c puts.c qsort.c rand48.c \
	rand.c rand_r.c read.c readdir.c rx.c scanf.c setenv.c setlinebuf.c \
	setvbuf.c signal.c sleep.c sprintf.c sscanf.c stat.c stderr.c stdin.c \
	stdout.c strcasecmp.c strcat.c strchr.c strcmp.c strcpy.c strcspn.c \
	strdup.c strerror.c strlcat.c strlcpy.c strlen.c strncasecmp.c \
	strncat.c strncmp.c strncpy.c strpbrk.c strrchr.c strsep.c strspn.c \
	strstr.c strtod.c strtof.c strtok.c strtok_r.c strtol.c strtold.c \
	strtoll.c strtoul.c strtoull.c strxfrm.c swab.c sysconf.c \
	sys_errlist.c tmpfile.c tolower.c toupper.c towlower.c towupper.c \
	ungetc.c unlink.c unsetenv.c vfdprintf.c vfprintf.c vfscanf.c \
	__v_printf.c vprintf.c __v_scanf.c vscanf.c vsnprintf.c vsprintf.c \
	vsscanf.c wcrtomb.c wcscat.c wcschr.c wcscmp.c wcscpy.c wcslen.c \
	wcsncat.c wcsncpy.c wcsrchr.c wcsstr.c wctomb.c wctype.c wcwidth.c \
	wmemcmp.c wmemcpy.c wmemset.c write.c crt0.c rewind.c snprintf.c rmdir.c \
	remove.c getwd.c __time.c ftime.c chmod.c fsync.c getcid.c

SRCS+=	__cpu_jmp.S  cpu_syscall.c

INCFLAGS= 	-I$(SRCDIR)/include -I$(SRCDIR)cpu/$(CPU) \
		-I$(SRCDIR)../libpthread/include -I$(SRCDIR)../libgomp/include \
		-I$(SRCDIR)../libm/include

include $(SRCDIR)../lib.mk

all:
	cp crt0.o $(LIBDIR)/.
