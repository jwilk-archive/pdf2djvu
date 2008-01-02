/* Copyright © 2007, 2008 Jakub Wilk
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991.
 */

#include <iostream>

#ifndef PDF2DJVU_DEBUG_H
#define PDF2DJVU_DEBUG_H

std::ostream &debug(int n);

class DevNull : public std::ostream
{
public:
  DevNull() : std::ostream(0) { }
};

extern DevNull dev_null;

class Error
{
public:
  Error() : message("Unknown error") {};
  explicit Error(const std::string &message) : message(message) {};
  friend std::ostream &operator<<(std::ostream &, const Error &);
protected:
  std::string message;
};

#endif

// vim:ts=2 sw=2 et
