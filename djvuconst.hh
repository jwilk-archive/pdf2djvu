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

  static const unsigned int version = 1;

  static const char shared_ant_file_name[] = "shared_anno.iff";

  namespace binary
  {

    static const char version = static_cast<char>(djvu::version);

    static const char djvm_head[] = "AT&TFORM\0\0\0\0DJVMDIRM\0\0\0";

    static const char dummy_head[12] =
    { '\x41', '\x54', '\x26', '\x54', '\x46', '\x4f', '\x52', '\x4d',
      '\x00', '\x00', '\x00', '\x20'
    };

    static const char dummy_double_head[48] =
    { '\x41', '\x54', '\x26', '\x54', '\x46', '\x4f', '\x52', '\x4d',
      '\x00', '\x00', '\x00', '\x74', '\x44', '\x4a', '\x56', '\x4d',
      '\x44', '\x49', '\x52', '\x4d', '\x00', '\x00', '\x00', '\x18',
      '\x81', '\x00', '\x02', '\x00', '\x00', '\x00', '\x30', '\x00',
      '\x00', '\x00', '\x58', '\xff', '\xff', '\xf2', '\xbf', '\x34',
      '\x7b', '\xf3', '\x10', '\x74', '\x07', '\x45', '\xc5', '\x40'
    };

    static const char dummy_page_data[32] =
    { '\x44', '\x4a', '\x56', '\x55', '\x49', '\x4e', '\x46', '\x4f',
      '\x00', '\x00', '\x00', '\x0a', '\x00', '\x01', '\x00', '\x01',
      '\x18', '\x00', '\x2c', '\x01', '\x16', '\x00', '\x53', '\x6a',
      '\x62', '\x7a', '\x00', '\x00', '\x00', '\x02', '\xbb', '\x7f'
    };

  }

}

#endif

// vim:ts=2 sts=2 sw=2 et
