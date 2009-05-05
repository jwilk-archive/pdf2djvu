/* Copyright © 2007, 2008, 2009 Jakub Wilk
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991.
 */

#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <stdexcept>

#include <fcntl.h>
#include <dirent.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef WIN32
#include <windows.h>
#endif

#include "system.hh"
#include "debug.hh"


/* constants
 * =========
 */

static const char unix_path_separator = '/';
#ifndef WIN32
static const char path_separator = unix_path_separator;
#else
static const char path_separator = '\\';
#endif


/* class POSIXError : OSError
 * ==========================
 */

std::string POSIXError::__error_message__(const std::string &context)
{
  std::string message = strerror(errno);
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
    error_log << "[Warning] " << e.what() << std::endl;
  }
}

#ifdef WIN32

/* class Win32Error : OSError
 * ==========================
 */

class Win32Error : public OSError
{
protected:
  static std::string __error_message__(const std::string &context);
public:
  explicit Win32Error(const std::string &context) 
  : OSError(__error_message__(context))
  { };
};

std::string Win32Error::__error_message__(const std::string &context)
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
    message.append("possibly memory allocation error");
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

#ifndef HAVE_PSTREAMS
static const std::string argv_to_command_line(const std::vector<std::string> &argv)
/* Translate a sequence of arguments into a command line string. */
{
  std::ostringstream buffer;
#ifdef WIN32
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
   * See <http://msdn.microsoft.com/library/en-us/vccelng/htm/progs_12.asp>.
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
    if (parg->find_first_of("\\\'") == std::string::npos) 
      buffer << *parg;
    else
      for (std::string::const_iterator pch = parg->begin(); pch != parg->end(); pch++)
      {
        if (*pch == '\\' || *pch == '\'')
          buffer << "'\"\\" << *pch << "\"'";
        else
          buffer << *pch;
      }
    buffer << "' ";
  }
#endif
  return buffer.str();
}
#endif


#if defined(HAVE_PSTREAMS)

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
    std::ostringstream message;
    message << "system(\"";
    message << this->command;
    message << " ...\") failed";
    if (WIFEXITED(status))
      message << " with exit code " << WEXITSTATUS(status);
    throw CommandFailed(message.str());
  }
}

#elif defined(WIN32)

void Command::call(std::ostream *my_stdout, bool quiet)
{
  int status = 0;
  unsigned long rc;
  HANDLE read_end, write_end, error_handle;
  SECURITY_ATTRIBUTES security_attributes;
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
    PROCESS_INFORMATION process_info;
    memset(&process_info, 0, sizeof process_info);
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
      CloseHandle(process_info.hProcess); /* ignore errors */
      CloseHandle(process_info.hThread); /* ignore errors */
      if (error_handle != INVALID_HANDLE_VALUE)
        CloseHandle(error_handle); /* ignore errors */
    }
  }
  if (status == 0)
  {
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
  }
  if (status < 0)
  {
    std::ostringstream message;
    message << "system(\"";
    message << this->command;
    message << " ...\") failed";
    throw Win32Error(message.str());
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
    status = status || pclose(file);
  }
  else
    status = -1;
  if (status < 0)
  {
    std::ostringstream message;
    message << "system(\"";
    message << this->command;
    message << " ...\") failed";
    throw POSIXError(message.str());
  }
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
#ifndef WIN32
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
#ifndef WIN32
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
  if (CreateFile(path_buffer, 0, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_TEMPORARY, NULL) == 0)
    throw_win32_error("CreateFile");
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


/* utility functions
 * =================
 */

namespace encoding
{
#ifdef WIN32
  /* The native encoding (so called ANSI character set) can differ from the
   * terminal encoding (typically: so called OEM charset).
   */
  template <>
  std::ostream &operator <<(std::ostream &stream, const proxy<native, terminal> &converter)
  {
    int rc;
    const std::string &string = converter.string;
    size_t length = converter.string.length();
    Array<char> buffer(length);
    string.copy(buffer, length);
    rc = CharToOemBuff(buffer, buffer, length);
    if (rc == 0)
    {
      /* This should actually never happen. */
      throw_win32_error("CharToOemBuff");
    }
    stream.write(buffer, length);
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

  void setup_locale()
  {
    setlocale(LC_CTYPE, "");
    /* Deliberately ignore errors. */
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

bool is_stream_a_tty(const std::ostream &ostream)
{
  if (&ostream == &std::cout)
    return isatty(STDOUT_FILENO);
  else
  {
    /* Not implemented for streams other that ``std::cout``.
     * See http://www.ginac.de/~kreckel/fileno/ for a more general
     * (although unportable, GCC-specific) solution.
     */
    throw std::invalid_argument("is_a_tty(const std::ostream &)");
  }
}

void split_path(const std::string &path, std::string &directory_name, std::string &file_name)
{
  /* POSIX-compliant ``basename()`` and ``dirname()`` would split ``/foo/bar/``
   * into ``/foo`` and ``"bar"``, instead of desired ``"foo/bar"`` and an empty
   * string. To deal with this weirdness, a trailing ``!`` character is
   * appended to the splitted path.
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

void stream_printf(std::ostream &stream, char *message, va_list args)
{
  int length = vsnprintf(NULL, 0, message, args);
  assert(length >= 0);
  if (length < 0)
    throw_posix_error("vsnprintf");
  Array<char> buffer(length + 1);
  length = vsprintf(buffer, message, args);
  assert(length >= 0);
  if (length < 0)
    throw_posix_error("vsprintf");
  stream << static_cast<char*>(buffer);
}

// vim:ts=2 sw=2 et
