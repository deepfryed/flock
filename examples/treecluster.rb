#!/usr/bin/ruby

require 'pp'
require 'flock'

data = []
data << %w(apple orange)
data << %w(black white)
data << %w(white cyan)
data << %w(orange)
data << %w(apple)

pp Flock.sparse_treecluster(2, data)
