/* Copyright © 2007, 2008, 2009 Jakub Wilk
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991.
 */

#ifndef PDF2DJVU_SYSTEM_HH
#define PDF2DJVU_SYSTEM_HH

#include <fstream>
#include <iostream>
#include <stdexcept>

#include "version.hh"

#ifdef HAVE_PSTREAMS
#  ifdef HAVE_PSTREAM_H
#    include <pstream.h>
#  endif
#  ifdef HAVE_PSTREAMS_PSTREAM_H
#    include <pstreams/pstream.h>
#  endif
#else
#include <vector>
#endif


class OSError : public std::runtime_error
{
protected:
  explicit OSError(const std::string &message) 
  : std::runtime_error(message)
  { };
};

class POSIXError : public OSError
{
protected:
  static std::string __error_message__(const std::string &context);
public:
  explicit POSIXError(const std::string &context) 
  : OSError(__error_message__(context))
  { };
};

class NoSuchFileOrDirectory : public POSIXError
{
public:
  NoSuchFileOrDirectory(const std::string &context)
  : POSIXError(context)
  { };
};

class NotADirectory : public POSIXError
{
public:
  NotADirectory(const std::string &context)
  : POSIXError(context)
  { };
};

class Command
{
protected:
  std::string command;
#ifdef HAVE_PSTREAMS  
  redi::pstreams::argv_type argv;
#else
  std::vector<std::string> argv;
#endif
  void call(std::ostream *my_stdout, bool quiet = false);
public:
  class CommandFailed : public std::runtime_error
  {
  public:
    CommandFailed(const std::string &message) 
    : std::runtime_error(message)
    { }
  };
  explicit Command(const std::string& command);
  Command &operator <<(const std::string& arg);
  Command &operator <<(int i);
  void operator()(std::ostream &my_stdout, bool quiet = false);
  void operator()(bool quiet = false);
};

class Directory
{
protected:
  std::string name;
  void *posix_dir;
  void open(const char *name);
  void close();
  Directory()
  : name(""), posix_dir(NULL)
  { }
public: 
  explicit Directory(const std::string &name);
  virtual ~Directory() throw ();
  friend std::ostream &operator<<(std::ostream &, const Directory &);
};

class TemporaryDirectory : public Directory
{
private:
  TemporaryDirectory(const TemporaryDirectory&); // not defined
  TemporaryDirectory& operator=(const TemporaryDirectory&); // not defined
public:
  TemporaryDirectory();
  virtual ~TemporaryDirectory() throw ();
};

class File : public std::fstream
{
private:
  File(const File&); // not defined
  File& operator=(const File&); // not defined
protected:
  std::string name;
  std::string base_name;
  void open(const char* path, bool truncate = true);
  File()
  { }
public:
  explicit File(const std::string &name);
  File(const Directory& directory, const std::string &name);
  virtual ~File() throw ()
  { }
  size_t size();
  void reopen(bool truncate = false);
  const std::string& get_basename() const;
  operator const std::string& () const;
  friend std::ostream &operator<<(std::ostream &, const File &);
};

class TemporaryFile : public File
{
private:
  TemporaryFile(const TemporaryFile &); // not defined
  TemporaryFile& operator=(const TemporaryFile &); // not defined
protected:
  void construct();
public:
  TemporaryFile(const Directory& directory, const std::string &name) 
  : File(directory, name) 
  { }
  TemporaryFile(const std::string &name)
  : File(name)
  { }
  TemporaryFile();
  virtual ~TemporaryFile() throw ();
};

class ExistingFile : public File
{
private:
  ExistingFile(const ExistingFile &); // not defined
  ExistingFile& operator=(const ExistingFile &); // not defined
public:
  explicit ExistingFile(const std::string &name);
  ExistingFile(const Directory& directory, const std::string &name);
};

namespace encoding
{

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

  void setup_locale();

}

void copy_stream(std::istream &istream, std::ostream &ostream, bool seek);
void copy_stream(std::istream &istream, std::ostream &ostream, bool seek, std::streamsize limit);

void stream_printf(std::ostream &stream, const char *message, va_list args);
std::string string_printf(const char *message, ...);

bool is_stream_a_tty(const std::ostream &ostream);

void split_path(const std::string &path, std::string &directory_name, std::string &file_name);

std::string absolute_path(const std::string &path, const std::string &dir_name);

void prevent_pop_out();

#endif

// vim:ts=2 sw=2 et
