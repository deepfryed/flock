#!/usr/bin/ruby

require 'pp'
require 'flock'

data = []
data << %w(apple orange)
data << %w(black white)
data << %w(white cyan)
data << %w(apple orange)
data << %w(apple)

pp Flock.kcluster(2, data, sparse: true, seed: Flock::SEED_RANDOM)
pp Flock.kcluster(2, data, sparse: true, seed: Flock::SEED_KMEANS_PLUSPLUS)
pp Flock.kcluster(2, data, sparse: true, seed: Flock::SEED_SPREADOUT)
