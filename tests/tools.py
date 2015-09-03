# encoding=UTF-8

# Copyright © 2009-2015 Jakub Wilk <jwilk@jwilk.net>
#
# This file is part of pdfdjvu.
#
# pdf2djvu is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# pdf2djvu is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.

import collections
import inspect
import itertools
import os
import re
import subprocess as ipc
import sys
import xml.etree.cElementTree as etree

re.compile.M = re.M
re.compile.DOTALL = re.DOTALL
re = re.compile
re.type = type(re(''))

import nose

from nose import SkipTest

from nose.tools import (
    assert_equal,
    assert_not_equal,
    assert_true,
)

def noseimport(vmaj, vmin, name=None):
    def wrapper(f):
        if sys.version_info >= (vmaj, vmin):
            return getattr(nose.tools, name or f.__name__)
        return f
    return wrapper

@noseimport(2, 7)
def assert_in(x, c):
    assert_true(
        x in c,
        msg='{0!r} not found in {1!r}'.format(x, c)
    )

@noseimport(2, 7)
def assert_is(x, y):
    assert_true(
        x is y,
        msg='{0!r} is not {1!r}'.format(x, y)
    )

@noseimport(2, 7)
def assert_is_not(x, y):
    assert_true(
        x is not y,
        msg='{0!r} is {1!r}'.format(x, y)
    )

@noseimport(2, 7)
def assert_is_none(obj):
    assert_is(obj, None)

@noseimport(2, 7)
def assert_is_not_none(obj):
    assert_is_not(obj, None)

@noseimport(2, 7)
def assert_multi_line_equal(x, y):
    return assert_equal(x, y)
if sys.version_info >= (2, 7):
    type(assert_multi_line_equal.__self__).maxDiff = None

@noseimport(2, 7, 'assert_regexp_matches')
def assert_regex(text, regex):
    if isinstance(regex, basestring):
        regex = re(regex)
    if not regex.search(text):
        message = "Regex didn't match: {0!r} not found in {1!r}".format(regex.pattern, text)
        assert_true(False, msg=message)

def assert_well_formed_xml(xml):
    try:
        etree.fromstring(xml)
    except SyntaxError as ex:
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
            assert_regex(self.stderr, stderr)
        else:
            assert_multi_line_equal(self.stderr, stderr)
        if rc is not None:
            assert_equal(self.rc, rc)
        if stdout is None:
            pass
        elif isinstance(stdout, re.type):
            assert_regex(self.stdout, stdout)
        else:
            assert_multi_line_equal(self.stdout, stdout)

class case(object):

    _pdf2djvu_command = os.getenv('pdf2djvu') or 'pdf2djvu'

    def get_pdf2djvu_command(self):
        if re(r'\A[[a-zA-Z0-9_+/=.,:%-]+\Z').match(self._pdf2djvu_command):
            return (self._pdf2djvu_command,)
        return ('sh', '-c', self._pdf2djvu_command + ' "$@"', 'sh')

    def get_source_path(self, strip_py=False):
        result = inspect.getsourcefile(type(self))
        if strip_py and result.endswith('.py'):
            return result[:-3]
        else:
            return result

    def get_pdf_path(self):
        return self.get_source_path(strip_py=True) + '.pdf'

    def get_djvu_path(self):
        return self.get_source_path(strip_py=True) + '.djvu'

    def run(self, *commandline):
        env = dict(os.environ,
            MALLOC_CHECK_='3',
            MALLOC_PERTURB_=str(0xA5),
        )
        try:
            child = ipc.Popen(list(commandline),
                stdout=ipc.PIPE,
                stderr=ipc.PIPE,
                env=env,
            )
        except OSError as exc:
            exc.filename = commandline[0]
            raise
        stdout, stderr = child.communicate()
        return ipc_result(stdout, stderr, child.returncode)

    def _pdf2djvu(self, *args):
        args = self.get_pdf2djvu_command() + ('-q', self.get_pdf_path()) + args
        return self.run(*args)

    def pdf2djvu(self, *args):
        return self._pdf2djvu('-o', self.get_djvu_path(), *args)

    def pdf2djvu_indirect(self, *args):
        return self._pdf2djvu('-i', self.get_djvu_path(), *args)

    def djvudump(self, *args):
        return self.run('djvudump', self.get_djvu_path(), *args)

    def djvused(self, expr):
        return self.run(
            'djvused',
            '-e', expr,
            self.get_djvu_path()
        )

    def print_text(self):
        return self.run('djvutxt', self.get_djvu_path())

    def print_outline(self):
        return self.djvused('print-outline')

    def print_ant(self, page):
        return self.djvused('select {0}; print-ant'.format(page))

    def print_meta(self):
        return self.djvused('print-meta')

    def decode(self, mode=None):
        args = []
        if mode is not None:
            args += ['-mode={m}'.format(m=mode)]
        return self.run(
            'ddjvu',
            self.get_djvu_path(),
            '-format=ppm',
            '-subsample=1',
            *args
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

    def require_feature(self, feature):
        r = self.pdf2djvu('--version')
        r.assert_(stderr=re('^pdf2djvu '), rc=0)
        if feature not in r.stderr:
            raise SkipTest(feature + ' support missing')

def rainbow(width, height):
    from PIL import Image
    from PIL import ImageColor
    image = Image.new('RGB', (width, height))
    pixels = image.load()
    for x in xrange(width):
        for y in xrange(height):
            hue = 255 * x // (width - 1)
            luminance = 100 * y // height
            color = ImageColor.getrgb('hsl({hue}, 100%, {lum}%)'.format(hue=hue, lum=luminance))
            pixels[x, y] = color
    return image

def checkboard(width, height):
    from PIL import Image
    image = Image.new('1', (width, height))
    pixels = image.load()
    for x in xrange(width):
        for y in xrange(height):
            color = 0xff * ((x ^ y) & 1)
            pixels[x, y] = color
    return image

_ppm_re = re('P6\s+\d+\s+\d+\s+255\s(.*)\Z', re.DOTALL)
def count_ppm_colors(b):
    match = _ppm_re.match(b)
    assert_is_not_none(match)
    pixels = match.group(1)
    ipixels = iter(pixels)
    result = collections.defaultdict(int)
    for pixel in itertools.izip(ipixels, ipixels, ipixels):
        result[pixel] += 1
    return result

__all__ = [
    # nose:
    'assert_equal',
    'assert_not_equal',
    'assert_true',
    'assert_in',
    'assert_is',
    'assert_is_none',
    'assert_is_not_none',
    'assert_multi_line_equal',
    'assert_regex',
    # our own asserts:
    'assert_well_formed_xml',
    # helper classes:
    'ipc_result',
    'case',
    # image handling:
    'rainbow',
    'checkboard',
    'count_ppm_colors',
]

# vim:ts=4 sts=4 sw=4 et
