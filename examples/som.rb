#!/usr/bin/ruby

require 'pp'
require 'flock'

data = []
data << %w(orange apple)
data << %w(black white)
data << %w(white cyan)
data << %w(orange)
data << %w(apple)

pp Flock.sparse_self_organizing_map(2, 2, data)
