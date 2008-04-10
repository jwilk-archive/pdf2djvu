/* Copyright © 2007, 2008 Jakub Wilk
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991.
 */

#include "system.hh"

#include <cerrno>
#include <sstream>
#include <stdexcept>

#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <iconv.h>

OSError::OSError() : Error("")
{
  message = strerror(errno);
}

Command::Command(const std::string& command) : command(command)
{
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

void Directory::_open(const char* path)
{
  this->posix_dir = opendir(path);
  if (this->posix_dir == NULL)
    throw OSError();
}

void Directory::_close(void)
{
  if (this->posix_dir == NULL)
    return;
  if (closedir(static_cast<DIR*>(this->posix_dir)) != 0)
    throw OSError();
}

void File::_open(const char* path)
{
  this->exceptions(std::ifstream::failbit | std::ifstream::badbit);
  if (path == NULL)
    this->open(this->name.c_str(), std::fstream::in | std::fstream::out);
  else
  {
    this->name = path;
    this->open(path, std::fstream::in | std::fstream::out | std::fstream::trunc);
  }
  this->exceptions(std::ifstream::badbit);
}

File::File(const std::string &name)
{
  this->_open(name.c_str());
}

File::File(const Directory& directory, const std::string &name)
{
  std::ostringstream stream;
  stream << directory << "/" << name;
  this->_open(stream.str().c_str());
}

size_t File::size()
{
  this->seekg(0, std::ios::end);
  return this->tellg();
}

void File::reopen()
{
  if (this->is_open())
    this->close();
  this->_open(NULL);
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

void Command::call(std::ostream *my_stdout, bool quiet)
{
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
  int status = xsystem.rdbuf()->status();
  if (status != 0)
  {
    std::ostringstream message;
    message << "system(\"";
    message << this->command;
    message << " ...\") failed";
    if (WIFEXITED(status))
      message << " with exit code " << WEXITSTATUS(status);
    throw Error(message.str());
  }
}

void utf16_to_utf8(const char *inbuf, size_t inbuf_len, std::ostream &stream)
{
  static char outbuf[1 << 10];
  char *outbuf_ptr = outbuf;
  size_t outbuf_len = sizeof outbuf;
  iconv_t cd = iconv_open("UTF-8", "UTF-16");
  if (cd == reinterpret_cast<iconv_t>(-1))
    throw OSError();
  while (inbuf_len > 0)
  {
    struct iconv_adapter 
    {
      // http://wang.yuxuan.org/blog/2007/7/9/deal_with_2_versions_of_iconv_h
      iconv_adapter(const char** s) : s(s) {}
      iconv_adapter(char** s) : s(const_cast<const char**>(s)) {}
      operator char**() const
      {
        return const_cast<char**>(s);
      }
      operator const char**() const
      {
        return const_cast<const char**>(s);
      }
      const char** s;
    };

    size_t n = iconv(cd, iconv_adapter(&inbuf), &inbuf_len, &outbuf_ptr, &outbuf_len);
    if (n == (size_t) -1 && errno == E2BIG)
    {
      stream.write(outbuf, outbuf_ptr - outbuf);
      outbuf_ptr = outbuf;
      outbuf_len = sizeof outbuf;
    }
    else if (n == (size_t) -1)
      throw IconvError();
  }
  stream.write(outbuf, outbuf_ptr - outbuf);
  if (iconv_close(cd) == -1)
    throw OSError();
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
    // Not implemented for `ostream != std::count`.
    // See <http://www.ginac.de/~kreckel/fileno/> for a more general
    // (although terribly unportable) solution.
    throw std::invalid_argument("is_a_tty(const std::ostream &)");
  }
}

// vim:ts=2 sw=2 et
