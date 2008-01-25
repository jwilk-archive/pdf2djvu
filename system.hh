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
#include <pstreams/pstream.h>

#include "debug.hh"

class OSError : public Error
{
public:
  OSError();
};

class Command
{
private:
  std::string command;
  redi::pstreams::argv_type argv;
  void call(std::ostream *my_stdout, bool quiet = false);
public:
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
public: 
  explicit Directory(const std::string &name)
  : name(name) 
  { }
  virtual ~Directory() {}
  friend std::ostream &operator<<(std::ostream &, const Directory &);
};

class TemporaryDirectory : public Directory
{
private:
  TemporaryDirectory(const TemporaryDirectory&); // not defined
  TemporaryDirectory& operator=(const TemporaryDirectory&); // not defined
public:
  TemporaryDirectory() : Directory("")
  {
    char path_buffer[] = "/tmp/pdf2djvu.XXXXXX";
    if (mkdtemp(path_buffer) == NULL)
      throw OSError();
    this->name += path_buffer;
  }

  virtual ~TemporaryDirectory()
  {
    if (rmdir(this->name.c_str()) == -1)
      throw OSError();
  }
};

class File : public std::fstream
{
protected:
  std::string name;
  void _open(const char* path);
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
  void construct()
  {
    char path_buffer[] = "/tmp/pdf2djvu.XXXXXX";
    int fd = mkstemp(path_buffer);
    if (fd == -1)
      throw OSError();
    if (::close(fd) == -1)
      throw OSError();
    _open(path_buffer);
  }

public:
  TemporaryFile(const Directory& directory, const std::string &name) 
  : File(directory, name) 
  { }

  TemporaryFile()
  {
    this->construct();
  }

  virtual ~TemporaryFile()
  {
    if (this->is_open())
      this->close();
    if (unlink(this->name.c_str()) == -1)
      throw OSError();
  }
};

class IconvError : public Error
{
public:
  IconvError() : Error("Unable to convert encodings") {} 
};

void utf16_to_utf8(const char *inbuf, size_t inbuf_len, std::ostream &stream);

void copy_stream(std::istream &istream, std::ostream &ostream, bool seek);
void copy_stream(std::istream &istream, std::ostream &ostream, bool seek, std::streamsize limit);

bool is_stream_a_tty(const std::ostream &ostream);

#endif

// vim:ts=2 sw=2 et
