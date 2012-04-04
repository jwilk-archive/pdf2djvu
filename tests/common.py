#!/usr/bin/python
# encoding=UTF-8

# Copyright © 2009, 2012 Jakub Wilk
#
# This package is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 dated June, 1991.

import inspect
import os
import re
import subprocess as ipc
import xml.etree.cElementTree as etree

re.compile.M = re.M
re = re.compile
re.type = type(re(''))

from nose.tools import (
    assert_true,
    assert_equal,
    assert_not_equal,
)

try:
    from nose.tools import assert_multi_line_equal
except ImportError:
    assert_multi_line_equal = assert_equal
else:
    assert_multi_line_equal.im_class.maxDiff = None

try:
    from nose.tools import assert_regexp_matches as assert_grep
except ImportError:
    def assert_grep(text, expected_regexp, msg=None):
        '''Fail the test unless the text matches the regular expression.'''
        if not isinstance(expected_regexp, re.type):
            expected_regexp = re(expected_regexp)
        if expected_regexp.search(text):
            return
        if msg is None:
            msg = "Regexp didn't match"
        msg = '%s: %r not found in %r' % (msg, expected_regexp.pattern, text)
        raise AssertionError(msg)

def assert_well_formed_xml(xml):
    try:
        etree.fromstring(xml)
    except SyntaxError, ex:
        raise AssertionError(ex)

class ipc_result(object):


    def __init__(self, stdout, stderr, rc):
        self.stdout = stdout
        self.stderr = stderr
        self.rc = rc

    def assert_(self, stdout='', stderr='', rc=0):
        if stderr is None:
            pass
        elif isinstance(stderr, re.type):
            assert_grep(self.stderr, stderr)
        else:
            assert_multi_line_equal(self.stderr, stderr)
        if rc is not None:
            assert_equal(self.rc, rc)
        if stdout is None:
            pass
        elif isinstance(stdout, re.type):
            assert_grep(self.stdout, stdout)
        else:
            assert_multi_line_equal(self.stdout, stdout)

class case(object):

    _pdf2djvu_command = os.getenv('pdf2djvu') or 'pdf2djvu'

    def get_pdf2djvu_command(self):
        return self._pdf2djvu_command

    def get_source_file(self, strip_py=False):
        result = inspect.getsourcefile(type(self))
        if strip_py and result.endswith('.py'):
            return result[:-3]
        else:
            return result

    def get_pdf_file(self):
        return self.get_source_file(strip_py=True) + '.pdf'

    def get_djvu_file(self):
        return self.get_source_file(strip_py=True) + '.djvu'

    def run(self, *commandline):
        child = ipc.Popen(list(commandline), stdout=ipc.PIPE, stderr=ipc.PIPE)
        stdout, stderr = child.communicate()
        return ipc_result(stdout, stderr, child.returncode)

    def _pdf2djvu(self, *args):
        return self.run(
            self.get_pdf2djvu_command(),
            '-q',
            self.get_pdf_file(),
            *args
        )

    def pdf2djvu(self, *args):
        return self._pdf2djvu('-o', self.get_djvu_file(), *args)

    def pdf2djvu_indirect(self, *args):
        return self._pdf2djvu('-i', self.get_djvu_file(), *args)

    def djvudump(self, *args):
        return self.run('djvudump', self.get_djvu_file(), *args)

    def djvused(self, expr):
        return self.run(
            'djvused',
            '-e', expr,
            self.get_djvu_file()
        )

    def print_pure_txt(self):
        return self.djvused('print-pure-txt')

    def print_outline(self):
        return self.djvused('print-outline')

    def print_ant(self, page):
        return self.djvused('select %d; print-ant' % page)

    def print_meta(self):
        return self.djvused('print-meta')

    def decode(self):
        return self.run(
            'ddjvu',
            self.get_djvu_file()
        )

    def extract_xmp(self):
        r = self.djvused('output-ant')
        assert_equal(r.stderr, '')
        assert_equal(r.rc, 0)
        xmp_lines = [line for line in r.stdout.splitlines() if line.startswith('(xmp "')]
        if len(xmp_lines) == 0:
            return
        [xmp_line] = xmp_lines
        assert_equal(xmp_line[-2:], '")')
        xmp = xmp_line[5:-1]
        xmp = eval(xmp)
        return xmp

def rainbow(width, height):
    import Image
    import ImageColor
    image = Image.new('RGB', (width, height))
    pixels = image.load()
    for x in xrange(width):
        for y in xrange(height):
            hue = 255 * x // (width - 1)
            luminance = 100 * y // height
            color = ImageColor.getrgb('hsl(%(hue)d, 100%%, %(luminance)d%%)' % locals())
            pixels[x, y] = color
    return image

# vim:ts=4 sw=4 et
