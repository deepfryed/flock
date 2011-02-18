#!/usr/bin/ruby

require 'pp'
require 'flock'

data = []
data << { 1 => 0.5, 2 => 0.5 }
data << { 3 => 1, 4 => 1 }
data << { 4 => 1, 5 => 0.3 }
data << { 2 => 0.75 }
data << { 1 => 0.60 }

pp Flock.sparse_kmeans(2, data)

data = []
data << %w(apple orange)
data << %w(black white)
data << %w(white cyan)
data << %w(orange)
data << %w(apple)

pp Flock.sparse_kmeans(2, data)
