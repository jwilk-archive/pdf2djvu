/* Copyright © 2007, 2008 Jakub Wilk
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991.
 */

namespace djvu
{
  /* XXX
   * csepdjvu requires 25 <= dpi <= 144 000
   * djvumake requires 72 <= dpi <= 144 000
   * cpaldjvu requires 25 <= dpi <=   1 200 (but we don't use it)
   */
  static const int MIN_DPI = 72;
  static const int MAX_DPI = 144000;
}

// vim:ts=2 sw=2 et
