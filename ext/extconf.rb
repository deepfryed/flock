#!/usr/bin/ruby

require 'mkmf'
$CFLAGS  = '-fPIC -Os -Wall'
create_makefile('cluster')
