/* Copyright © 2007-2015 Jakub Wilk <jwilk@jwilk.net>
 * Copyright © 2009 Mateusz Turcza
 *
 * This file is part of pdfdjvu.
 *
 * pdf2djvu is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * pdf2djvu is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include "system.hh"

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>

#include <dirent.h>
#include <libgen.h>
#include <sys/stat.h>
#include <unistd.h>

#if WIN32
#include <windows.h>
#endif

#include "array.hh"
#include "autoconf.hh"
#include "debug.hh"
#include "i18n.hh"

#if USE_MINGW_ANSI_STDIO
#define vsnprintf __mingw_vsnprintf
#endif

/* constants
 * =========
 */

#if WIN32
static const char path_separator = '\\';
#else
static const char path_separator = '/';
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

void throw_posix_error(const std::string &context)
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

void throw_win32_error(const std::string &context)
{
  throw Win32Error(context);
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

void binmode(const std::ostream &ostream)
{
#if WIN32
  if (&ostream == &std::cout)
  {
    int rc = setmode(STDOUT_FILENO, O_BINARY);
    if (rc == -1)
      throw_posix_error("setmode");
  }
  else
  {
    /* not implemented */
    throw std::invalid_argument("binmode");
  }
#endif
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
  size_t l = 0, r = 0;
  if (wlength >= 2 && wpath[1] == L':')
    l = r = 2;
  while (l < wlength && (wpath[l] == L'/' || wpath[l] == L'\\'))
  {
    l++;
    r++;
  }
  for (size_t i = l; i < wlength; i++)
  {
    if (wpath[i] == L'/' || path[i] == L'\\')
    {
      l = i;
      r = i + 1;
    }
  }
  alength = WideCharToMultiByte(
    CP_ACP, 0,
    wpath, l,
    apath, length * 2,
    NULL, NULL
  );
  if (l > 0)
  {
    if (alength == 0)
      throw_win32_error("WideCharToMultiByte");
    directory_name = std::string(apath, alength);
  }
  else
    directory_name = ".";
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
  if (path.length() == 1 || (path.length() >= 2 && (path[1] == '/' || path[1] == path_separator)))
    return dir_name + path_separator + path.substr(std::min(static_cast<size_t>(2), path.length()));
  assert(path.length() >= 2);
  if (path[1] != '.')
    return path;
  if (path.length() == 2 || (path.length() >= 3 && (path[2] == '/' || path[2] == path_separator)))
    return dir_name + path_separator + path;
  return path;
}

bool is_same_file(const std::string &path1, const std::string &path2)
{
#if WIN32
  BY_HANDLE_FILE_INFORMATION info1, info2;
  HANDLE handle;
  int ok;
  handle = CreateFile(path1.c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
  if (handle == INVALID_HANDLE_VALUE)
    return false;
  ok = GetFileInformationByHandle(handle, &info1);
  CloseHandle(handle);
  if (!ok)
    return false;
  handle = CreateFile(path2.c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
  if (handle == INVALID_HANDLE_VALUE)
    return false;
  ok = GetFileInformationByHandle(handle, &info2);
  CloseHandle(handle);
  if (!ok)
    return false;
  return
    (info1.dwVolumeSerialNumber == info2.dwVolumeSerialNumber) &&
    (info1.nFileSizeLow == info2.nFileSizeLow) &&
    (info1.nFileSizeHigh == info2.nFileSizeHigh);
#else
  struct stat st1, st2;
  int rc;
  rc = stat(path1.c_str(), &st1);
  if (rc)
    return false;
  rc = stat(path2.c_str(), &st2);
  if (rc)
    return false;
  return
    (st1.st_dev == st2.st_dev) &&
    (st1.st_ino == st2.st_ino);
#endif
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
  if (length < 0)
    throw_posix_error("vsnprintf");
  if (length == std::numeric_limits<int>::max())
  {
    errno = ENOMEM;
    throw_posix_error("vsnprintf");
  }
  Array<char> buffer(length + 1);
  length = vsprintf(buffer, message, args);
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
