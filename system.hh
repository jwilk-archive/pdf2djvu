/* Copyright © 2007-2022 Jakub Wilk <jwilk@jwilk.net>
 *
 * This file is part of pdf2djvu.
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

#ifndef PDF2DJVU_SYSTEM_HH
#define PDF2DJVU_SYSTEM_HH

#include <cstddef>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

class OSError : public std::runtime_error
{
protected:
  explicit OSError(const std::string &message)
  : std::runtime_error(message)
  { }
};

class POSIXError : public OSError
{
public:
  static std::string error_message(const std::string &context);
  explicit POSIXError(const std::string &context)
  : OSError(error_message(context))
  { }
};

[[noreturn]]
void throw_posix_error(const std::string &context);
#ifdef WIN32
[[noreturn]]
void throw_win32_error(const std::string &context);
#endif

class NoSuchFileOrDirectory : public POSIXError
{
public:
  explicit NoSuchFileOrDirectory(const std::string &context)
  : POSIXError(context)
  { }
};

class NotADirectory : public POSIXError
{
public:
  explicit NotADirectory(const std::string &context)
  : POSIXError(context)
  { }
};

class File;

class Command
{
protected:
  std::string command;
  std::vector<std::string> argv;
  std::string repr();
  void call(std::istream *stdin_, std::ostream *stdout_, bool stderr_);
public:
  class CommandFailed : public std::runtime_error
  {
  public:
    explicit CommandFailed(const std::string &message)
    : std::runtime_error(message)
    { }
  };
  explicit Command(const std::string& command);
  Command &operator <<(const std::string& arg);
  Command &operator <<(const File& arg);
  Command &operator <<(int i);
  void operator()(std::ostream &stdout_, bool quiet=false)
  {
    this->call(nullptr, &stdout_, !quiet);
  }
  void operator()(bool quiet=false)
  {
    this->call(nullptr, nullptr, !quiet);
  }
  static std::string filter(const std::string &command_line, const std::string &string);
};

class Directory
{
protected:
  std::string name;
  void *posix_dir;
  void open(const char *name);
  void close();
  Directory()
  : name(""), posix_dir(nullptr)
  { }
public:
  explicit Directory(const std::string &name);
  virtual ~Directory();
  friend std::ostream &operator<<(std::ostream &, const Directory &);
};

class TemporaryDirectory : public Directory
{
private:
  TemporaryDirectory(const TemporaryDirectory&) = delete;
  TemporaryDirectory& operator=(const TemporaryDirectory&) = delete;
public:
  TemporaryDirectory();
  virtual ~TemporaryDirectory();
};

class File : public std::fstream
{
private:
  File(const File&) = delete;
  File& operator=(const File&) = delete;
protected:
  std::string name;
  std::string base_name;
  virtual File::openmode get_default_open_mode();
  void open(const std::string &path, File::openmode mode);
  File()
  { }
public:
  explicit File(const std::string &path);
  File(const Directory& directory, const std::string &name);
  virtual ~File()
  { }
  std::streamoff size();
  void reopen(std::fstream::openmode mode = std::fstream::openmode());
  const std::string& get_basename() const;
  operator const std::string& () const;
  friend std::ostream &operator<<(std::ostream &, const File &);
};

class TemporaryFile : public File
{
private:
  TemporaryFile(const TemporaryFile &) = delete;
  TemporaryFile& operator=(const TemporaryFile &) = delete;
protected:
  void construct();
public:
  TemporaryFile(const Directory& directory, const std::string &name)
  : File(directory, name)
  { }
  explicit TemporaryFile(const std::string &name)
  : File(name)
  { }
  TemporaryFile();
  virtual ~TemporaryFile();
};

class ExistingFile : public File
{
private:
  ExistingFile(const ExistingFile &) = delete;
  ExistingFile& operator=(const ExistingFile &) = delete;
protected:
  virtual File::openmode get_default_open_mode();
public:
  explicit ExistingFile(const std::string &path)
  : File(path)
  { }
  ExistingFile(const Directory& directory, const std::string &name)
  : File(directory, name)
  { }
  virtual ~ExistingFile()
  { }
};

#if WIN32

class ProgramDir
: public std::string
{
public:
  ProgramDir();
};

const static ProgramDir program_dir;

#endif

namespace encoding
{

  class Error : public POSIXError
  {
  public:
    Error()
    : POSIXError("")
    { }
  };

  enum encoding
  {
    native,
    terminal,
    utf8,
  };

  template <enum encoding from, enum encoding to>
  class proxy;

  template <enum encoding from, enum encoding to>
  std::ostream &operator << (std::ostream &, const proxy<from, to> &);

  template <enum encoding from, enum encoding to>
  class proxy
  {
  protected:
    const std::string &string;
  public:
    explicit proxy<from, to>(const std::string &string)
    : string(string)
    { }
    friend std::ostream &operator << <>(std::ostream &, const proxy<from, to> &);
  };

}

void copy_stream(std::istream &istream, std::ostream &ostream, bool seek);
void copy_stream(std::istream &istream, std::ostream &ostream, bool seek, std::streamsize limit);

bool isatty(const std::ostream &ostream);
void binmode(const std::ostream &ostream);

void split_path(const std::string &path, std::string &directory_name, std::string &file_name);

std::string absolute_path(const std::string &path, const std::string &dir_name);

bool is_same_file(const std::string &path1, const std::string &path2);

void prevent_pop_out();

#endif

// vim:ts=2 sts=2 sw=2 et
