#!/usr/bin/ruby

require 'pp'
require 'flock'

data = []
data << { apple:  1, orange: 1 }
data << { black:  1, white:  1 }
data << { white:  1, cyan:   1 }
data << { orange: 1 }
data << { apple:  1 }

pp Flock.sparse_kmeans(2, data)

data = []
data << %w(apple orange)
data << %w(black white)
data << %w(white cyan)
data << %w(orange)
data << %w(apple)

pp Flock.sparse_kmeans(2, data)
