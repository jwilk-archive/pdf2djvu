/* Copyright © 2007, 2008, 2009 Jakub Wilk
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991.
 */

#ifndef PDF2DJVU_DJVU_CONST_H
#define PDF2DJVU_DJVU_CONST_H

namespace djvu
{
  static const int min_dpi = 72;
  static const int max_dpi = 6000;
  /* ``djvumake`` used to require dpi ≥ 72.
   * The library itself enforces dpi ≤ 6000.
   *
   * See http://sourceforge.net/p/djvu/bugs/103/ for details.
   */

  static const unsigned int max_fg_colors = 4080;

  static const unsigned int max_subsample_ratio = 12;

  static const char shared_ant_file_name[] = "shared_anno.iff";

}

#endif

// vim:ts=2 sts=2 sw=2 et
