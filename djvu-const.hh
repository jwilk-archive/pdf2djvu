/* Copyright © 2007-2019 Jakub Wilk <jwilk@jwilk.net>
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

#ifndef PDF2DJVU_DJVU_CONST_H
#define PDF2DJVU_DJVU_CONST_H

namespace djvu
{
  static const int min_dpi = 72;
  static const int max_dpi = 6000;
  /* ``djvumake`` used to require dpi ≥ 72.
   * The library itself enforces dpi ≤ 6000.
   *
   * See https://sourceforge.net/p/djvu/bugs/103/ for details.
   */

  static const int max_fg_colors = 4080;

  static const int max_subsample_ratio = 12;

  static const char shared_ant_file_name[] = "shared_anno.iff";

}

#endif

// vim:ts=2 sts=2 sw=2 et
