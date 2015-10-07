# encoding=UTF-8

# Copyright © 2009-2015 Jakub Wilk <jwilk@jwilk.net>
#
# This file is part of pdf2djvu.
#
# pdf2djvu is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# pdf2djvu is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.

from __future__ import print_function

import collections
import codecs
import inspect
import itertools
import locale
import os
import pipes
import re
import subprocess as ipc
import sys

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

def _get_locale_for_encoding(encoding):
    encoding = codecs.lookup(encoding).name
    candidates = {
        'utf-8': ['C.UTF-8', 'en_US.UTF-8'],
        'iso8859-1': ['en_US.ISO8859-1'],
    }[encoding]
    old_locale = locale.setlocale(locale.LC_ALL)
    try:
        for new_locale in candidates:
            try:
                locale.setlocale(locale.LC_ALL, new_locale)
            except locale.Error:
                continue
            return new_locale
    finally:
        locale.setlocale(locale.LC_ALL, old_locale)
    raise SkipTest(
        'locale {loc} is required'.format(loc=' or '.join(candidates))
    )

class case(object):

    _pdf2djvu_command = os.getenv('pdf2djvu') or 'pdf2djvu'
    _feature_cache = {}

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

    def run(self, *commandline, **kwargs):
        env = dict(os.environ,
            MALLOC_CHECK_='3',
            MALLOC_PERTURB_=str(0xA5),
        )
        for key, value in kwargs.items():
            if key.isupper():
                env[key] = value
                continue
            if key == 'encoding':
                env['LC_ALL'] = _get_locale_for_encoding(value)
                continue
            raise TypeError('{key!r} is an invalid keyword argument for this function'.format(key=key))
        print('$', ' '.join(map(pipes.quote, commandline)))
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

    def _pdf2djvu(self, *args, **kwargs):
        args = self.get_pdf2djvu_command() + ('-q', self.get_pdf_path()) + args
        result = self.run(*args, **kwargs)
        if os.getenv('pdf2djvu_win32'):
            result.stderr = result.stderr.replace('\r\n', '\n')
        if sys.platform.startswith('openbsd'):
            # FIXME: https://bitbucket.org/jwilk/pdf2djvu/issues/108
            result.stderr = re(
                r'Magick: Failed to close module [(]"\w*: Invalid handle\"[)].\n'
            ).sub('', result.stderr)
        return result

    def pdf2djvu(self, *args, **kwargs):
        return self._pdf2djvu('-o', self.get_djvu_path(), *args, **kwargs)

    def pdf2djvu_indirect(self, *args):
        return self._pdf2djvu('-i', self.get_djvu_path(), *args)

    def djvudump(self, *args):
        return self.run('djvudump', self.get_djvu_path(), *args)

    def djvused(self, expr, **kwargs):
        return self.run(
            'djvused',
            '-e', expr,
            self.get_djvu_path(),
            **kwargs
        )

    def print_text(self):
        return self.run('djvutxt', self.get_djvu_path())

    def print_outline(self):
        return self.djvused('print-outline')

    def print_ant(self, page):
        return self.djvused('select {0}; print-ant'.format(page))

    def print_meta(self):
        return self.djvused('print-meta')

    def ls(self):
        return self.djvused('ls', encoding='UTF-8')

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
        try:
            feature_enabled = self._feature_cache[feature]
        except KeyError:
            if feature == 'POSIX':
                feature_enabled = not os.getenv('pdf2djvu_win32')
            else:
                r = self.pdf2djvu('--version')
                r.assert_(stderr=re('^pdf2djvu '), rc=0)
                feature_enabled = feature in r.stderr
            self._feature_cache[feature] = feature_enabled
        if not feature_enabled:
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

_ppm_re = re(r'P6\s+\d+\s+\d+\s+255\s(.*)\Z', re.DOTALL)
def count_ppm_colors(b):
    match = _ppm_re.match(b)
    assert_is_not_none(match)
    pixels = match.group(1)
    ipixels = iter(pixels)
    result = collections.defaultdict(int)
    for pixel in itertools.izip(ipixels, ipixels, ipixels):
        result[pixel] += 1
    return result

xml_ns = dict(
    dc='http://purl.org/dc/elements/1.1/',
    xmpMM='http://ns.adobe.com/xap/1.0/mm/',
)

def xml_find_text(xml, tag):
    [elem] = xml.findall('.//' + tag, xml_ns)
    return elem.text

_uuid_regex = (
    r'\Aurn:uuid:XXXXXXXX-XXXX-4XXX-[89ab]XXX-XXXXXXXXXXXX\Z'
    .replace('X', '[0-9a-f]')
)

def assert_uuid_urn(uuid):
    return assert_regex(
        uuid,
        _uuid_regex,
    )

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
    # helper classes:
    'ipc_result',
    'case',
    # image handling:
    'rainbow',
    'checkboard',
    'count_ppm_colors',
    # XMP:
    'assert_uuid_urn',
    'xml_find_text',
    'xml_ns',
]

# vim:ts=4 sts=4 sw=4 et
