#!/usr/bin/python
# encoding=UTF-8

# Copyright © 2009 Jakub Wilk
#
# This package is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 dated June, 1991.

import Image
import ImageColor

def rainbow(width, height):
	image = Image.new('RGB', (width, height))

	pixels = image.load()
	for x in xrange(width):
		for y in xrange(height):
			hue = 255 * x // (width - 1)
			luminance = 100 * y // height
			color = ImageColor.getrgb('hsl(%(hue)d, 100%%, %(luminance)d%%)' % locals())
			pixels[x, y] = color
	return image

# vim:ts=4 sw=4 noet
