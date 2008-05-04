/* Copyright © 2007, 2008 Jakub Wilk
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991.
 */

#ifndef PDF2DJVU_SYSTEM_HH
#define PDF2DJVU_SYSTEM_HH

#include <iostream>
#include <fstream>
#include <stdexcept>

#include <pstreams/pstream.h>

class OSError : public std::runtime_error
{
private:
  static std::string __error_message__(const std::string &context);
public:
  explicit OSError(const std::string &context) 
  : std::runtime_error(__error_message__(context))
  { };
};

class NoSuchFileOrDirectory : public OSError
{
public:
  NoSuchFileOrDirectory(const std::string &context) : OSError(context) {}
};

class NotADirectory : public OSError
{
public:
  NotADirectory(const std::string &context) : OSError(context) {}
};

class Command
{
private:
  std::string command;
  redi::pstreams::argv_type argv;
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
  Directory() : name(""), posix_dir(NULL) {}
public: 
  explicit Directory(const std::string &name);
  virtual ~Directory();
  friend std::ostream &operator<<(std::ostream &, const Directory &);
};

class TemporaryDirectory : public Directory
{
private:
  TemporaryDirectory(const TemporaryDirectory&); // not defined
  TemporaryDirectory& operator=(const TemporaryDirectory&); // not defined
public:
  TemporaryDirectory();
  virtual ~TemporaryDirectory();
};

class File : public std::fstream
{
private:
  File(const File&); // not defined
  File& operator=(const File&); // not defined
protected:
  std::string name;
  void open(const char* path);
  File() {}
public:
  explicit File(const std::string &name);
  File(const Directory& directory, const std::string &name);
  virtual ~File() { }
  size_t size();
  void reopen();
  operator const std::string& () const;
  friend std::ostream &operator<<(std::ostream &, const File &);
};

class TemporaryFile : public File
{
private:
  TemporaryFile(const TemporaryFile&); // not defined
  TemporaryFile& operator=(const TemporaryFile&); // not defined
protected:
  void construct();
public:
  TemporaryFile(const Directory& directory, const std::string &name) 
  : File(directory, name) 
  { }
  TemporaryFile();
  virtual ~TemporaryFile();
};

class ExistingFile : public File
{
private:
  ExistingFile(const ExistingFile&); // not defined
  ExistingFile& operator=(const ExistingFile&); // not defined
public:
  explicit ExistingFile(const std::string &name);
  ExistingFile(const Directory& directory, const std::string &name);
};

class IconvError : public std::runtime_error
{
public:
  IconvError()
  : std::runtime_error("Unable to convert encodings")
  { } 
};

void utf16_to_utf8(const char *inbuf, size_t inbuf_len, std::ostream &stream);

void copy_stream(std::istream &istream, std::ostream &ostream, bool seek);
void copy_stream(std::istream &istream, std::ostream &ostream, bool seek, std::streamsize limit);

bool is_stream_a_tty(const std::ostream &ostream);

#endif

// vim:ts=2 sw=2 et
