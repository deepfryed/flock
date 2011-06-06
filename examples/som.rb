#!/usr/bin/ruby

require 'pp'
require 'flock'

data = []
data << %w(orange apple)
data << %w(black white)
data << %w(white cyan)
data << %w(orange)
data << %w(apple)

data, weights = Flock.densify(data)
pp Flock.self_organizing_map(2, 2, data)
