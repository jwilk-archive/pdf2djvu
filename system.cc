/* Copyright © 2007-2015 Jakub Wilk
 * Copyright © 2009 Mateusz Turcza
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991.
 */

#include <cassert>
#include <cerrno>
#include <cstdarg>
#include <cstdlib>
#include <cstdarg>
#include <cstring>
#include <limits>
#include <sstream>
#include <stdexcept>

#include <fcntl.h>
#include <dirent.h>
#include <libgen.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>

#if WIN32
#include <climits>
#include <windows.h>
#endif

#include "system.hh"
#include "debug.hh"
#include "i18n.hh"

#if !HAVE_PSTREAMS && HAVE_FORK
#include <sys/wait.h>
#include <unistd.h>
#endif

#if USE_MINGW_ANSI_STDIO
#define vsnprintf __mingw_vsnprintf
#endif

/* constants
 * =========
 */

static const char unix_path_separator = '/';
#if !WIN32
static const char path_separator = unix_path_separator;
#else
static const char path_separator = '\\';
#endif


/* class POSIXError : OSError
 * ==========================
 */

std::string POSIXError::error_message(const std::string &context)
{
  std::string message;
#if WIN32
  /* Win32 systems tends to return POSIX-style error messages in English.
   * Translation is required.
   */
  /* L10N: Windows error message for EPERM */
  N_("Operation not permitted");
  /* L10N: Windows error message for ENOENT */
  N_("No such file or directory");
  /* L10N: Windows error message for ESRCH */
  N_("No such process");
  /* L10N: Windows error message for EINTR */
  N_("Interrupted function call");
  /* L10N: Windows error message for EINTR */
  N_("Interrupted system call");
  /* L10N: Windows error message for EIO */
  N_("Input/output error");
  /* L10N: Windows error message for ENXIO */
  N_("No such device or address");
  /* L10N: Windows error message for E2BIG */
  N_("Arg list too long");
  /* L10N: Windows error message for E2BIG */
  N_("Argument list too long");
  /* L10N: Windows error message for ENOEXEC */
  N_("Exec format error");
  /* L10N: Windows error message for EBADF */
  N_("Bad file descriptor");
  /* L10N: Windows error message for ECHILD */
  N_("No child processes");
  /* L10N: Windows error message for EAGAIN */
  N_("Resource temporarily unavailable");
  /* L10N: Windows error message for ENOMEM */
  N_("Not enough space");
  /* L10N: Windows error message for ENOMEM */
  N_("Cannot allocate memory");
  /* L10N: Windows error message for EACCES */
  N_("Permission denied");
  /* L10N: Windows error message for EFAULT */
  N_("Bad address");
  /* L10N: Windows error message for EBUSY */
  N_("Resource device");
  /* L10N: Windows error message for EBUSY */
  N_("Device or resource busy");
  /* L10N: Windows error message for EEXIST */
  N_("File exists");
  /* L10N: Windows error message for EXDEV */
  N_("Improper link");
  /* L10N: Windows error message for EXDEV */
  N_("Invalid cross-device link");
  /* L10N: Windows error message for ENODEV */
  N_("No such device");
  /* L10N: Windows error message for ENOTDIR */
  N_("Not a directory");
  /* L10N: Windows error message for EISDIR */
  N_("Is a directory");
  /* L10N: Windows error message for EINVAL */
  N_("Invalid argument");
  /* L10N: Windows error message for ENFILE */
  N_("Too many open files in system");
  /* L10N: Windows error message for EMFILE */
  N_("Too many open files");
  /* L10N: Windows error message for ENOTTY */
  N_("Inappropriate I/O control operation");
  /* L10N: Windows error message for ENOTTY */
  N_("Inappropriate ioctl for device");
  /* L10N: Windows error message for EFBIG */
  N_("File too large");
  /* L10N: Windows error message for ENOSPC */
  N_("No space left on device");
  /* L10N: Windows error message for ESEEK */
  N_("Invalid seek");
  /* L10N: Windows error message for ESEEK */
  N_("Illegal seek");
  /* L10N: Windows error message for EROFS */
  N_("Read-only file system");
  /* L10N: Windows error message for EMLINK */
  N_("Too many links");
  /* L10N: Windows error message for EPIPE */
  N_("Broken pipe");
  /* L10N: Windows error message for EDOM */
  N_("Domain error");
  /* L10N: Windows error message for EDOM */
  N_("Numerical argument out of domain");
  /* L10N: Windows error message for ERANGE */
  N_("Result too large");
  /* L10N: Windows error message for ERANGE */
  N_("Numerical result out of range");
  /* L10N: Windows error message for EDEADLK */
  N_("Resource deadlock avoided");
  /* L10N: Windows error message for ENAMETOOLONG */
  N_("Filename too long");
  /* L10N: Windows error message for ENAMETOOLONG */
  N_("File name too long");
  /* L10N: Windows error message for ENOLOCK */
  N_("No locks available");
  /* L10N: Windows error message for ENOSYS */
  N_("Function not implemented");
  /* L10N: Windows error message for ENOTEMPTY */
  N_("Directory not empty");
  /* L10N: Windows error message for EILSEQ */
  N_("Illegal byte sequence");
  /* L10N: Windows error message for EILSEQ */
  N_("Invalid or incomplete multibyte or wide character");
  message = _(strerror(errno));
#else
  /* POSIX says that ``strerror()`` returns a locale-dependent error message.
   * No need to translate. */
  message = strerror(errno);
#endif
  if (context.length())
    message = context + ": " + message;
  return message;
}

static void throw_posix_error(const std::string &context)
{
  switch (errno)
  {
  case ENOTDIR:
    throw NotADirectory(context);
  case ENOENT:
    throw NoSuchFileOrDirectory(context);
  default:
    throw POSIXError(context);
  }
}

static void warn_posix_error(const std::string &context)
{
  try
  {
    throw_posix_error(context);
  }
  catch (const POSIXError &e)
  {
    error_log << string_printf(_("Warning: %s"), e.what()) << std::endl;
  }
}

#if WIN32

/* class Win32Error : OSError
 * ==========================
 */

class Win32Error : public OSError
{
protected:
  static std::string error_message(const std::string &context);
public:
  explicit Win32Error(const std::string &context)
  : OSError(error_message(context))
  { };
};

std::string Win32Error::error_message(const std::string &context)
{
  char *buffer;
  std::string message = context + ": ";
  unsigned long error_code = GetLastError();
  unsigned long nbytes = FormatMessage(
    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
    NULL,
    error_code,
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
    (char*) &buffer,
    0,
    NULL
  );
  if (nbytes == 0)
    message.append(_("possibly memory allocation error"));
  else
  {
    message.append(buffer);
    LocalFree(buffer);
  }
  return message;
}

static void throw_win32_error(const std::string &context)
{
  throw Win32Error(context);
}

#endif


/* class Command
 * =============
 */

Command::Command(const std::string& command) : command(command)
{
  if (this->argv.size() == 0 && path_separator != unix_path_separator)
  {
    /* Convert path separators: */
    std::ostringstream stream;
    for (std::string::const_iterator it = command.begin(); it != command.end(); it++)
    {
      if (*it == unix_path_separator)
        stream << path_separator;
      else
        stream << *it;
    }
    this->argv.push_back(stream.str());
  }
  else
    this->argv.push_back(command);
}

Command &Command::operator <<(const std::string& arg)
{
  this->argv.push_back(arg);
  return *this;
}

Command &Command::operator <<(const File& arg)
{
  this->argv.push_back(arg);
  return *this;
}

Command &Command::operator <<(int i)
{
  std::ostringstream stream;
  stream << i;
  return *this << stream.str();
}

void Command::operator()(std::ostream &my_stdout, bool quiet)
{
  this->call(&my_stdout, quiet);
}

void Command::operator()(bool quiet)
{
  this->call(NULL, quiet);
}

#if !HAVE_PSTREAMS
static const std::string argv_to_command_line(const std::vector<std::string> &argv)
/* Translate a sequence of arguments into a command line string. */
{
  std::ostringstream buffer;
#if WIN32
  /* Using the same rules as the MS C runtime:
   *
   * 1) Arguments are delimited by white space, which is either a space or a
   *    tab.
   *
   * 2) A string surrounded by double quotation marks is interpreted as a
   *    single argument, regardless of white space contained within. A
   *    quoted string can be embedded in an argument.
   *
   * 3) A double quotation mark preceded by a backslash is interpreted as a
   *    literal double quotation mark.
   *
   * 4) Backslashes are interpreted literally, unless they immediately
   *    precede a double quotation mark.
   *
   * 5) If backslashes immediately precede a double quotation mark, every
   *    pair of backslashes is interpreted as a literal backslash.  If the
   *    number of backslashes is odd, the last backslash escapes the next
   *    double quotation mark as described in rule 3.
   *
   * See <http://msdn.microsoft.com/en-us/library/ms880421.aspx>.
   */
  for (std::vector<std::string>::const_iterator parg = argv.begin(); parg != argv.end(); parg++)
  {
    int backslashed = 0;
    bool need_quote = parg->find_first_of(" \t") != std::string::npos;
    if (need_quote)
      buffer << '"';
    for (std::string::const_iterator pch = parg->begin(); pch != parg->end(); pch++)
    {
      if (*pch == '\\')
      {
        backslashed++;
      }
      else if (*pch == '"')
      {
        for (int i = 0; i < backslashed; i++)
          buffer << "\\\\";
        backslashed = 0;
        buffer << "\\\"";
      }
      else
      {
        for (int i = 0; i < backslashed; i++)
          buffer << '\\';
        backslashed = 0;
        buffer << *pch;
      }
    }
    for (int i = 0; i < backslashed; i++)
      buffer << '\\';
    if (need_quote)
      buffer << '"';
    buffer << ' ';
  }
#else
  /* Assume POSIX shell. */
  for (std::vector<std::string>::const_iterator parg = argv.begin(); parg != argv.end(); parg++)
  {
    buffer << "'";
    if (parg->find("\'") == std::string::npos)
      buffer << *parg;
    else
      for (std::string::const_iterator pch = parg->begin(); pch != parg->end(); pch++)
      {
        if (*pch == '\'')
          buffer << "'\\''";
        else
          buffer << *pch;
      }
    buffer << "' ";
  }
#endif
  return buffer.str();
}
#endif


#if HAVE_PSTREAMS

void Command::call(std::ostream *my_stdout, bool quiet)
{
  int status;
  redi::ipstream xsystem(this->command, this->argv, redi::pstream::pstdout | redi::pstream::pstderr);
  if (!xsystem.rdbuf()->error())
  {
    if (my_stdout != NULL)
    {
      xsystem.out();
      copy_stream(xsystem, *my_stdout, false);
    }
    {
      xsystem.err();
      copy_stream(xsystem, quiet ? dev_null : std::cerr, false);
    }
    xsystem.close();
  }
  status = xsystem.rdbuf()->status();
  if (status != 0)
  {
    std::string message;
    if (WIFEXITED(status))
    {
      message = string_printf(
        _("External command \"%s ...\" failed with exit code %u"),
        this->command.c_str(),
        static_cast<unsigned int>(WEXITSTATUS(status))
      );
    }
    else
    {
      message = string_printf(
        _("External command \"%s ...\" failed"),
        this->command.c_str()
      );
    }
    throw CommandFailed(message);
  }
}

#elif WIN32

void Command::call(std::ostream *my_stdout, bool quiet)
{
  int status = 0;
  unsigned long rc;
  PROCESS_INFORMATION process_info;
  HANDLE read_end, write_end, error_handle;
  SECURITY_ATTRIBUTES security_attributes;
  memset(&process_info, 0, sizeof process_info);
  security_attributes.nLength = sizeof (SECURITY_ATTRIBUTES);
  security_attributes.lpSecurityDescriptor = NULL;
  security_attributes.bInheritHandle = true;
  if (CreatePipe(&read_end, &write_end, &security_attributes, 0) == 0)
    throw_win32_error("CreatePipe");
  rc = SetHandleInformation(read_end, HANDLE_FLAG_INHERIT, 0);
  if (rc == 0)
  {
    if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
      /* Presumably it's Windows 9x, so the call is not supported.
       * Punt on security and let the pipe end be inherited.
       */
    }
    else
      throw_win32_error("SetHandleInformation");
  }
  if (!quiet)
  {
    error_handle = GetStdHandle(STD_ERROR_HANDLE);;
    if (error_handle != INVALID_HANDLE_VALUE)
    {
      rc = DuplicateHandle(
        GetCurrentProcess(), error_handle,
        GetCurrentProcess(), &error_handle,
        0, true, DUPLICATE_SAME_ACCESS
      );
      if (rc == 0)
        throw_win32_error("DuplicateHandle");
    }
  }
  else
  {
    error_handle = CreateFile("nul",
      GENERIC_WRITE, FILE_SHARE_WRITE,
      &security_attributes, OPEN_EXISTING, 0, NULL);
    /* Errors can be safely ignored:
     * - For the Windows NT family, INVALID_HANDLE_VALUE does actually the right thing.
     * - For Windows 9x, spurious debug messages could be generated, tough luck!
     */
  }
  {
    const std::string &command_line = argv_to_command_line(this->argv);
    STARTUPINFO startup_info;
    memset(&startup_info, 0, sizeof startup_info);
    startup_info.cb = sizeof startup_info;
    startup_info.hStdOutput = write_end;
    startup_info.hStdError = error_handle;
    startup_info.dwFlags = STARTF_USESTDHANDLES;
    char *c_command_line = strdup(command_line.c_str());
    if (c_command_line == NULL)
      throw_posix_error("strdup");
    rc = CreateProcess(
      NULL, c_command_line,
      NULL, NULL,
      true, 0,
      NULL, NULL,
      &startup_info,
      &process_info
    );
    free(c_command_line);
    if (rc == 0)
      status = -1;
    else
    {
      if (error_handle != INVALID_HANDLE_VALUE)
        CloseHandle(error_handle); /* ignore errors */
    }
  }
  if (status == 0)
  {
    unsigned long exit_code;
    CloseHandle(write_end); /* ignore errors */
    while (true)
    {
      char buffer[BUFSIZ];
      unsigned long nbytes;
      bool success = ReadFile(read_end, buffer, sizeof buffer, &nbytes, NULL);
      if (!success)
      {
        status = -(GetLastError() != ERROR_BROKEN_PIPE);
        break;
      }
      if (my_stdout != NULL)
        my_stdout->write(buffer, nbytes);
    }
    CloseHandle(read_end); /* ignore errors */
    rc = WaitForSingleObject(process_info.hProcess, INFINITE);
    if (rc == WAIT_FAILED)
      throw_win32_error("WaitForSingleObject");
    rc = GetExitCodeProcess(process_info.hProcess, &exit_code);
    CloseHandle(process_info.hProcess); /* ignore errors */
    CloseHandle(process_info.hThread); /* ignore errors */
    if (rc == 0)
      status = -1;
    else if (exit_code != 0)
    {
      std::string message = string_printf(
        _("External command \"%s ...\" failed with exit code %u"),
        this->command.c_str(),
        static_cast<unsigned int>(exit_code)
      );
      throw CommandFailed(message);
    }
  }
  if (status < 0)
  {
    std::string message = string_printf(
      _("External command \"%s ...\" failed"),
      this->command.c_str()
    );
    throw Win32Error(message);
  }
}

#else

void Command::call(std::ostream *my_stdout, bool quiet)
{
  int status;
  FILE *file;
  {
    const std::string &command_line = argv_to_command_line(this->argv);
    file = ::popen(command_line.c_str(), "r");
  }
  if (file != NULL)
  {
    char buffer[BUFSIZ];
    size_t nbytes;
    status = 0;
    while (!feof(file))
    {
      nbytes = fread(buffer, 1, sizeof buffer, file);
      if (ferror(file))
      {
        status = -1;
        break;
      }
      if (my_stdout != NULL)
        my_stdout->write(buffer, nbytes);
    }
    int wait_status = pclose(file);
    if (wait_status == -1)
      throw_posix_error("pclose");
    else if (status == 0)
    {
      if (WIFEXITED(wait_status))
      {
        unsigned int exit_code = WEXITSTATUS(wait_status);
        if (exit_code != 0)
        {
          std::string message = string_printf(
            _("External command \"%s ...\" failed with exit code %u"),
            this->command.c_str(),
            exit_code
          );
          throw CommandFailed(message);
        }
      }
      else
        status = -1;
    }
  }
  else
    status = -1;
  if (status < 0)
  {
    std::string message = string_printf(
      _("External command \"%s ...\" failed"),
      this->command.c_str()
    );
    throw_posix_error(message);
  }
}

#endif

#if WIN32

class FilterWriterData
{
public:
  HANDLE handle;
  const std::string &string;
  FilterWriterData(HANDLE handle, const std::string &string)
  : handle(handle),
    string(string)
  { }
};

unsigned long WINAPI filter_writer(void *data_)
{
  bool success;
  FilterWriterData *data = reinterpret_cast<FilterWriterData*>(data_);
  success = WriteFile(data->handle, data->string.c_str(), data->string.length(), NULL, NULL);
  if (!success)
    throw_win32_error("WriteFile");
  success = CloseHandle(data->handle);
  if (!success)
    throw_win32_error("CloseHandle");
  return 0;
}

std::string Command::filter(const std::string &command_line, const std::string string)
{
  int status = 0;
  unsigned long rc;
  HANDLE stdin_read, stdin_write, stdout_read, stdout_write, error_handle;
  PROCESS_INFORMATION process_info;
  SECURITY_ATTRIBUTES security_attributes;
  memset(&process_info, 0, sizeof process_info);
  security_attributes.nLength = sizeof (SECURITY_ATTRIBUTES);
  security_attributes.lpSecurityDescriptor = NULL;
  security_attributes.bInheritHandle = true;
  if (CreatePipe(&stdin_read, &stdin_write, &security_attributes, 0) == 0)
    throw_win32_error("CreatePipe");
  if (CreatePipe(&stdout_read, &stdout_write, &security_attributes, 0) == 0)
    throw_win32_error("CreatePipe");
  rc =
    SetHandleInformation(stdout_read, HANDLE_FLAG_INHERIT, 0) &&
    SetHandleInformation(stdin_write, HANDLE_FLAG_INHERIT, 0);
  if (rc == 0)
  {
    if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
      /* Presumably it's Windows 9x, so the call is not supported.
       * Punt on security and let the pipe end be inherited.
       */
    }
    else
      throw_win32_error("SetHandleInformation");
  }
  error_handle = GetStdHandle(STD_ERROR_HANDLE);;
  if (error_handle != INVALID_HANDLE_VALUE)
  {
    rc = DuplicateHandle(
      GetCurrentProcess(), error_handle,
      GetCurrentProcess(), &error_handle,
      0, true, DUPLICATE_SAME_ACCESS
    );
    if (rc == 0)
      throw_win32_error("DuplicateHandle");
  }
  {
    STARTUPINFO startup_info;
    memset(&startup_info, 0, sizeof startup_info);
    startup_info.cb = sizeof startup_info;
    startup_info.hStdInput = stdin_read;
    startup_info.hStdOutput = stdout_write;
    startup_info.hStdError = error_handle;
    startup_info.dwFlags = STARTF_USESTDHANDLES;
    char *c_command_line = strdup(command_line.c_str());
    if (c_command_line == NULL)
      throw_posix_error("strdup");
    rc = CreateProcess(
      NULL, c_command_line,
      NULL, NULL,
      true, 0,
      NULL, NULL,
      &startup_info,
      &process_info
    );
    free(c_command_line);
    if (rc == 0)
      status = -1;
    else
    {
      if (error_handle != INVALID_HANDLE_VALUE)
        CloseHandle(error_handle); /* ignore errors */
    }
  }
  if (status == 0)
  {
    unsigned long exit_code;
    std::ostringstream stream;
    CloseHandle(stdin_read); /* ignore errors */
    CloseHandle(stdout_write); /* ignore errors */
    FilterWriterData writer_data(stdin_write, string);
    HANDLE thread_handle = CreateThread(NULL, 0, filter_writer, &writer_data, 0, NULL);
    if (thread_handle == NULL)
      throw_win32_error("CreateThread");
    while (true)
    {
      char buffer[BUFSIZ];
      unsigned long nbytes;
      bool success = ReadFile(stdout_read, buffer, sizeof buffer, &nbytes, NULL);
      if (!success)
      {
        status = -(GetLastError() != ERROR_BROKEN_PIPE);
        break;
      }
      stream.write(buffer, nbytes);
    }
    CloseHandle(stdout_read); /* ignore errors */
    rc = WaitForSingleObject(thread_handle, INFINITE);
    if (rc == WAIT_FAILED)
      throw_win32_error("WaitForSingleObject");
    CloseHandle(thread_handle); /* ignore errors */
    rc = WaitForSingleObject(process_info.hProcess, INFINITE);
    if (rc == WAIT_FAILED)
      throw_win32_error("WaitForSingleObject");
    rc = GetExitCodeProcess(process_info.hProcess, &exit_code);
    CloseHandle(process_info.hProcess); /* ignore errors */
    CloseHandle(process_info.hThread); /* ignore errors */
    if (rc == 0)
      status = -1;
    else if (exit_code != 0)
    {
      std::string message = string_printf(
        _("External command \"%s\" failed with exit code %u"),
        command_line.c_str(),
        static_cast<unsigned int>(exit_code)
      );
      throw CommandFailed(message);
    }
    return stream.str();
  }
  if (status < 0)
  {
    std::string message = string_printf(
      _("External command \"%s\" failed"),
      command_line.c_str()
    );
    throw Win32Error(message);
  }
  return string; /* Should not really happen. */
}

#elif HAVE_FORK

std::string Command::filter(const std::string &command_line, const std::string string)
{
  int rc;
  int pipe_fds[2];
  rc = pipe(pipe_fds);
  if (rc != 0)
    throw_posix_error("pipe");
  pid_t writer_pid = fork();
  if (writer_pid == -1)
    throw_posix_error("fork");
  else if (writer_pid == 0)
  {
    /* Writer: */
    close(pipe_fds[0]); /* Deliberately ignore errors. */
    rc = dup2(pipe_fds[1], STDOUT_FILENO);
    if (rc == -1)
      throw_posix_error("dup2");
    FILE *fp = popen(command_line.c_str(), "w");
    if (fp == NULL)
      throw_posix_error("popen");
    if (fputs(string.c_str(), fp) == EOF)
      throw_posix_error("fputs");
    rc = pclose(fp);
    if (rc == -1)
      throw_posix_error("pclose");
    else if (WIFEXITED(rc))
      exit(WEXITSTATUS(rc));
    else
      exit(-1);
    exit(rc);
  }
  else
  {
    /* Main process: */
    close(pipe_fds[1]); /* Deliberately ignore errors. */
    std::ostringstream stream;
    while (1)
    {
      char buffer[BUFSIZ];
      ssize_t n = read(pipe_fds[0], buffer, sizeof buffer);
      if (n < 0)
        throw_posix_error("read");
      else if (n == 0)
        break;
      else
        stream.write(buffer, n);
    }
    int status;
    writer_pid = wait(&status);
    if (writer_pid == static_cast<pid_t>(-1))
      throw_posix_error("wait");
    if (status != 0)
    {
      std::string message;
      if (WIFEXITED(status))
      {
        message = string_printf(
          _("External command \"%s\" failed with exit code %u"),
          command_line.c_str(),
          static_cast<unsigned int>(WEXITSTATUS(status))
        );
      }
      else
      {
        message = string_printf(
          _("External command \"%s\" failed"),
          command_line.c_str()
        );
      }
      throw CommandFailed(message);
    }
    return stream.str();
  }
  return string; /* Should not really happen. */
}

#else

std::string Command::filter(const std::string &command_line, const std::string string)
{
  /* Should not really happen. */
  errno = ENOSYS;
  throw_posix_error("fork");
}

#endif


/* class Directory
 * ===============
 */

Directory::Directory(const std::string &name)
: name(name), posix_dir(NULL)
{
  this->open(name.c_str());
}

Directory::~Directory() throw ()
{
  this->close();
}

void Directory::open(const char* path)
{
  this->posix_dir = opendir(path);
  if (this->posix_dir == NULL)
    throw_posix_error(path);
}

void Directory::close(void)
{
  if (this->posix_dir == NULL)
    return;
  if (closedir(static_cast<DIR*>(this->posix_dir)) != 0)
    throw_posix_error(this->name);
}

/* class Array<tp>
 * ===============
 */

template <typename tp>
class Array
{
private:
  Array(const Array &); // not defined
  Array& operator=(const Array &); // not defined
protected:
  tp *buffer;
public:
  explicit Array(size_t size)
  {
    buffer = new tp[size];
  }

  operator tp * ()
  {
    return this->buffer;
  }

  ~Array() throw ()
  {
    delete[] this->buffer;
  }
};


/* class TemporaryPathTemplate : Array<char>
 * =========================================
 */


class TemporaryPathTemplate : public Array<char>
{
protected:
  static const char *temporary_directory()
  {
    const char *result = getenv("TMPDIR");
    if (result == NULL)
      result = P_tmpdir;
    return result;
  }
public:
  TemporaryPathTemplate()
  : Array<char>(strlen(this->temporary_directory()) + strlen(PACKAGE_NAME) + 9)
  {
    sprintf(*this, "%s%c%s.XXXXXX", this->temporary_directory(), path_separator, PACKAGE_NAME);
  }
};


/* class TemporaryDirectory : Directory
 * ====================================
 */

TemporaryDirectory::TemporaryDirectory() : Directory()
{
#if !WIN32
  TemporaryPathTemplate path_buffer;
  if (mkdtemp(path_buffer) == NULL)
    throw_posix_error(static_cast<char*>(path_buffer));
#else
  char base_path_buffer[PATH_MAX];
  char path_buffer[PATH_MAX];
  if (GetTempPath(PATH_MAX, base_path_buffer) == 0)
    throw_win32_error("GetTempPath");
  if (GetTempFileName(base_path_buffer, PACKAGE_NAME, 0, path_buffer) == 0)
    throw_win32_error("GetTempFileName");
  if (unlink(path_buffer) < 0)
    throw_posix_error(path_buffer);
  if (mkdir(path_buffer) < 0)
    throw_posix_error(path_buffer);
#endif
  this->name += path_buffer;
}

TemporaryDirectory::~TemporaryDirectory() throw ()
{
  if (rmdir(this->name.c_str()) == -1)
    warn_posix_error(this->name);
}


/* class File : std::fstream
 * =========================
 */

void File::open(const char* path, bool truncate)
{
  std::fstream::openmode mode = std::fstream::in | std::fstream::out | std::fstream::binary;
  if (truncate)
    mode |= std::fstream::trunc;
  this->exceptions(std::ifstream::failbit | std::ifstream::badbit);
  if (path == NULL)
    this->std::fstream::open(this->name.c_str(), mode);
  else
  {
    this->name = path;
    this->std::fstream::open(path, mode);
  }
  this->exceptions(std::ifstream::badbit);
}

File::File(const std::string &name)
{
  this->open(name.c_str());
}

File::File(const Directory& directory, const std::string &name)
{
  std::ostringstream stream;
  this->base_name = name;
  stream << directory << path_separator << name;
  this->open(stream.str().c_str());
}

size_t File::size()
{
  this->seekg(0, std::ios::end);
  return this->tellg();
}

void File::reopen(bool truncate)
{
  if (this->is_open())
    this->close();
  this->open(NULL, truncate);
}

const std::string& File::get_basename() const
{
  return this->base_name;
}

File::operator const std::string& () const
{
  return this->name;
}

std::ostream &operator<<(std::ostream &out, const Directory &directory)
{
  return out << directory.name;
}

std::ostream &operator<<(std::ostream &out, const File &file)
{
  return out << file.name;
}


/* class TemporaryFile : File
 * ==========================
 */

void TemporaryFile::construct()
{
#if !WIN32
  TemporaryPathTemplate path_buffer;
  int fd = mkstemp(path_buffer);
  if (fd == -1)
    throw_posix_error(static_cast<char*>(path_buffer));
  if (::close(fd) == -1)
    throw_posix_error(static_cast<char*>(path_buffer));
#else
  char base_path_buffer[PATH_MAX];
  char path_buffer[PATH_MAX];
  if (GetTempPath(PATH_MAX, base_path_buffer) == 0)
    throw_win32_error("GetTempPath");
  if (GetTempFileName(base_path_buffer, PACKAGE_NAME, 0, path_buffer) == 0)
    throw_win32_error("GetTempFileName");
  HANDLE handle = CreateFile(path_buffer, 0, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_TEMPORARY, NULL);
  if (handle == 0)
    throw_win32_error("CreateFile");
  CloseHandle(handle); /* ignore errors */
#endif
  this->open(path_buffer);
}

TemporaryFile::TemporaryFile()
{
  this->construct();
}

TemporaryFile::~TemporaryFile() throw ()
{
  if (this->is_open())
    this->close();
  if (unlink(this->name.c_str()) == -1)
    warn_posix_error(this->name);
}


/* class ExistingFile : File
 * =========================
 */

ExistingFile::ExistingFile(const std::string &name)
: File()
{
  this->name = name;
  this->open(NULL, false);
}

ExistingFile::ExistingFile(const Directory& directory, const std::string &name)
: File()
{
  std::ostringstream stream;
  stream << directory << "/" << name;
  this->name = stream.str();
  this->open(NULL, false);
}

#if WIN32

/* class Cwd
 * =========
 */

Cwd::Cwd(const std::string &path)
{
  int rc;
  size_t size = 32;
  while (1)
  {
    Array<char> buffer(size);
    rc = getcwd(buffer, size) == NULL;
    if (rc != 0)
    {
      if (errno == ERANGE && size < std::numeric_limits<size_t>::max() / 2)
      {
        size *= 2;
        continue;
      }
      throw_posix_error("getcwd");
    }
    this->previous_cwd = buffer;
    break;
  }
  rc = chdir(path.c_str());
  if (rc != 0)
    throw_posix_error("chdir");
}

Cwd::~Cwd()
{
  if (this->previous_cwd.length())
  {
    int rc = chdir(this->previous_cwd.c_str());
    if (rc != 0)
    {
      warn_posix_error("chdir");
      abort();
    }
  }
}

#endif

#if WIN32

/* class ProgramDir
 * ================
 */

ProgramDir::ProgramDir()
{
  char buffer[PATH_MAX];
  size_t n = GetModuleFileName(NULL, buffer, sizeof buffer);
  if (n == 0)
    throw_win32_error("GetModuleFileName");
  if (n >= sizeof buffer)
  {
    errno = ENAMETOOLONG;
    throw_posix_error("GetModuleFileName");
  }
  std::string dirname, basename;
  split_path(buffer, dirname, basename);
  (*this) += dirname;
}

#endif

/* utility functions
 * =================
 */

namespace encoding
{
#if WIN32
  /* The native encoding (so called ANSI character set) can differ from the
   * terminal encoding (typically: so called OEM charset).
   */
  template <>
  std::ostream &operator <<(std::ostream &stream, const proxy<native, terminal> &converter)
  {
    const std::string &string = converter.string;
    size_t wide_length, new_length, length = converter.string.length();
    if (length == 0)
      return stream;
    Array<char> buffer(length * 2);
    Array<wchar_t> wide_buffer(length);
    wide_length = MultiByteToWideChar(
      CP_ACP, 0,
      string.c_str(), length,
      wide_buffer, length
    );
    if (wide_length == 0)
      throw_win32_error("MultiByteToWideChar");
    new_length = WideCharToMultiByte(
      GetConsoleOutputCP(), 0,
      wide_buffer, wide_length,
      buffer, length * 2,
      NULL, NULL
    );
    if (new_length == 0)
      throw_win32_error("WideCharToMultiByte");
    stream.write(buffer, new_length);
    return stream;
  }
#else
  template <>
  std::ostream &operator <<(std::ostream &stream, const proxy<native, terminal> &converter)
  {
    stream << converter.string;
    return stream;
  }
#endif

  template <>
  std::ostream &operator <<(std::ostream &stream, const proxy<native, utf8> &converter)
  {
    /* The following code assumes that wchar_t strings are UTF-32 or UTF-16.
     * This is not necessarily the case for every system.
     *
     * See
     * http://unicode.org/faq/utf_bom.html
     * for description of both UTF-16 and UTF-8.
     */
    const std::string &string = converter.string;
    size_t length = string.length();
    Array<wchar_t> wstring(length + 1);
    length = mbstowcs(wstring, string.c_str(), length + 1);
    if (length == static_cast<size_t>(-1))
      throw_posix_error("mbstowcs");
    uint32_t code, code_shift = 0;
    for (size_t i = 0; i < length; i++)
    {
      code = wstring[i];
      if (code_shift)
      {
        /* A leading surrogate has been encountered. */
        if (code >= 0xdc00 && code < 0xe000)
        {
          /* A trailing surrogate is encountered. */
          code = code_shift + (code & 0x3ff);
        }
        else
        {
          /* An unpaired surrogate is encountered. */
          errno = EILSEQ;
          throw_posix_error(__FUNCTION__);
        }
        code_shift = 0;
      }
      else if (code >= 0xd800 && code < 0xdc00)
      {
        /* A leading surrogate is encountered. */
        code_shift = 0x10000 + ((code & 0x3ff) << 10);
        continue;
      }
      if (code >= 0x110000 || (code & 0xfffe) == 0xfffe)
      {
        /* Code is out of range or a non-character is encountered. */
        errno = EILSEQ;
        throw_posix_error(__FUNCTION__);
      }
      if (code < 0x80)
      {
        char ascii = code;
        stream << ascii;
      }
      else
      {
        char buffer[4];
        size_t nbytes;
        for (nbytes = 2; nbytes < 4; nbytes++)
          if (code < (1U << (5 * nbytes + 1)))
            break;
        buffer[0] = (0xff00 >> nbytes) & 0xff;
        for (size_t i = nbytes - 1; i; i--)
        {
          buffer[i] = 0x80 | (code & 0x3f);
          code >>= 6;
        }
        buffer[0] |= code;
        stream.write(buffer, nbytes);
      }
    }
    return stream;
  }
}

void copy_stream(std::istream &istream, std::ostream &ostream, bool seek)
{
  if (seek)
    istream.seekg(0, std::ios::beg);
  char buffer[BUFSIZ];
  while (!istream.eof())
  {
    istream.read(buffer, sizeof buffer);
    ostream.write(buffer, istream.gcount());
  }
}

void copy_stream(std::istream &istream, std::ostream &ostream, bool seek, std::streamsize limit)
{
  if (seek)
    istream.seekg(0, std::ios::beg);
  char buffer[BUFSIZ];
  while (!istream.eof() && limit > 0)
  {
    std::streamsize chunk_size = std::min(static_cast<std::streamsize>(sizeof buffer), limit);
    istream.read(buffer, chunk_size);
    ostream.write(buffer, istream.gcount());
    limit -= chunk_size;
  }
}

bool isatty(const std::ostream &ostream)
{
  if (&ostream == &std::cout)
    return isatty(STDOUT_FILENO);
  else
  {
    /* Not implemented for streams other that ``std::cout``.
     * See http://www.ginac.de/~kreckel/fileno/ for a more general
     * (although unportable, GCC-specific) solution.
     */
    throw std::invalid_argument("isatty(const std::ostream &)");
  }
}

void split_path(const std::string &path, std::string &directory_name, std::string &file_name)
{
#ifdef __MINGW32__
  /* MinGW32 implementations of ``basename()`` and ``dirname()`` are broken:
   * https://bugs.debian.org/625918
   * Therefore, we cannot use the generic code. This implementation is less
   * sophisticated than MinGW32 one, yet it should be sufficient for our
   * purposes.
   */
  size_t wlength, alength, length = path.length();
  Array<wchar_t> wpath(length);
  Array<char> apath(length);
  wlength = MultiByteToWideChar(
    CP_ACP, 0,
    path.c_str(), length,
    wpath, length
  );
  if (wlength == 0)
    throw_win32_error("MultiByteToWideChar");
  bool append_dot = true;
  size_t l = 0, r = 0;
  if (wlength >= 2 && wpath[1] == L':')
    l = r = 2;
  while (l < wlength && (wpath[l] == L'/' || wpath[l] == L'\\'))
  {
    l++, r++;
    append_dot = false;
  }
  for (size_t i = l; i < wlength; i++)
  {
    if (wpath[i] == L'/' || path[i] == L'\\')
    {
      l = i;
      r = i + 1;
      append_dot = false;
    }
  }
  alength = WideCharToMultiByte(
    GetConsoleOutputCP(), 0,
    wpath, l,
    apath, length * 2,
    NULL, NULL
  );
  if (alength == 0)
    throw_win32_error("WideCharToMultiByte");
  directory_name = std::string(apath, alength);
  if (append_dot)
    directory_name += ".";
  alength = WideCharToMultiByte(
    CP_ACP, 0,
    wpath + r, wlength - r,
    apath, length * 2,
    NULL, NULL
  );
  if (alength == 0 && r < wlength)
    throw_win32_error("WideCharToMultiByte");
  file_name = std::string(apath, alength);
#else
  /* POSIX-compliant ``basename()`` and ``dirname()`` would split ``/foo/bar/``
   * into ``/foo`` and ``bar``, instead of desired ``/foo/bar`` and an empty
   * string. To deal with this weirdness, a trailing ``!`` character is
   * appended to the split path.
   */
  {
    Array<char> buffer(path.length() + 2);
    sprintf(buffer, "%s!", path.c_str());
    directory_name = ::dirname(buffer);
  }
  {
    Array<char> buffer(path.length() + 2);
    sprintf(buffer, "%s!", path.c_str());
    file_name = ::basename(buffer);
    size_t length = file_name.length();
    assert(length > 0);
    assert(file_name[length - 1] == '!');
    file_name.erase(length - 1);
  }
#endif
}

std::string absolute_path(const std::string &path, const std::string &dir_name)
{
  if (path.length() == 0)
    return path;
  if (path[0] != '.')
    return path;
  if (path.length() == 1 || (path.length() >= 2 && (path[1] == unix_path_separator || path[1] == path_separator)))
    return dir_name + path_separator + path.substr(std::min(static_cast<size_t>(2), path.length()));
  assert(path.length() >= 2);
  if (path[1] != '.')
    return path;
  if (path.length() == 2 || (path.length() >= 3 && (path[2] == unix_path_separator || path[2] == path_separator)))
    return dir_name + path_separator + path;
  return path;
}

#if defined(va_copy)
#elif defined(__va_copy)
#define va_copy __va_copy
#else
#define va_copy(dest, src) memcpy((dest), (src), sizeof (va_list))
#endif

std::string string_vprintf(const char *message, va_list args)
{
  va_list args_copy;
  va_copy(args_copy, args);
  int length = vsnprintf(NULL, 0, message, args_copy);
  va_end(args_copy);
  assert(length >= 0);
  if (length < 0)
    throw_posix_error("vsnprintf");
  Array<char> buffer(length + 1);
  length = vsprintf(buffer, message, args);
  assert(length >= 0);
  if (length < 0)
    throw_posix_error("vsprintf");
  return static_cast<char*>(buffer);
}

std::string string_printf(const char *message, ...)
{
  va_list args;
  va_start(args, message);
  std::string result = string_vprintf(message, args);
  va_end(args);
  return result;
}


void prevent_pop_out(void)
{
#if WIN32
  /* GetConsoleProcessList() function is not available for some systems (e.g.,
   * Wine, Windows 98), so it's not desirable to import it at link time.
   */
  typedef DWORD (WINAPI *get_console_process_list_fn)(LPDWORD, DWORD);
  get_console_process_list_fn get_console_process_list;
  HMODULE dll = GetModuleHandle("kernel32");
  if (dll == NULL)
    return;
  get_console_process_list = (get_console_process_list_fn) GetProcAddress(dll, "GetConsoleProcessList");
  if (get_console_process_list != NULL)
  {
    unsigned long pid, rc;
    rc = get_console_process_list(&pid, 1);
    if (rc == 1)
      MessageBox(NULL, _("pdf2djvu is intended to be run from the command prompt."), PACKAGE_NAME, MB_OK | MB_ICONINFORMATION);
  }
#endif
}

// vim:ts=2 sts=2 sw=2 et
