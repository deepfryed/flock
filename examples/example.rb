#!/usr/bin/ruby

require 'flock'

data     = Array.new(13) {[]}
mask     = Array.new(13) {[]}
weights  = Array.new(13) {1.0}
clusters = Array.new(13)

data[ 0][ 0]=0.1; data[ 0][ 1]=0.0; data[ 0][ 2]=9.6; data[ 0][ 3] = 5.6;
data[ 1][ 0]=1.4; data[ 1][ 1]=1.3; data[ 1][ 2]=0.0; data[ 1][ 3] = 3.8;
data[ 2][ 0]=1.2; data[ 2][ 1]=2.5; data[ 2][ 2]=0.0; data[ 2][ 3] = 4.8;
data[ 3][ 0]=2.3; data[ 3][ 1]=1.5; data[ 3][ 2]=9.2; data[ 3][ 3] = 4.3;
data[ 4][ 0]=1.7; data[ 4][ 1]=0.7; data[ 4][ 2]=9.6; data[ 4][ 3] = 3.4;
data[ 5][ 0]=0.0; data[ 5][ 1]=3.9; data[ 5][ 2]=9.8; data[ 5][ 3] = 5.1;
data[ 6][ 0]=6.7; data[ 6][ 1]=3.9; data[ 6][ 2]=5.5; data[ 6][ 3] = 4.8;
data[ 7][ 0]=0.0; data[ 7][ 1]=6.3; data[ 7][ 2]=5.7; data[ 7][ 3] = 4.3;
data[ 8][ 0]=5.7; data[ 8][ 1]=6.9; data[ 8][ 2]=5.6; data[ 8][ 3] = 4.3;
data[ 9][ 0]=0.0; data[ 9][ 1]=2.2; data[ 9][ 2]=5.4; data[ 9][ 3] = 0.0;
data[10][ 0]=3.8; data[10][ 1]=3.5; data[10][ 2]=5.5; data[10][ 3] = 9.6;
data[11][ 0]=0.0; data[11][ 1]=2.3; data[11][ 2]=3.6; data[11][ 3] = 8.5;
data[12][ 0]=4.1; data[12][ 1]=4.5; data[12][ 2]=5.8; data[12][ 3] = 7.6;

mask[ 0][ 0]=1; mask[ 0][ 1]=1; mask[ 0][ 2]=1; mask[ 0][ 3] = 1;
mask[ 1][ 0]=1; mask[ 1][ 1]=1; mask[ 1][ 2]=0; mask[ 1][ 3] = 1;
mask[ 2][ 0]=1; mask[ 2][ 1]=1; mask[ 2][ 2]=0; mask[ 2][ 3] = 1;
mask[ 3][ 0]=1; mask[ 3][ 1]=1; mask[ 3][ 2]=1; mask[ 3][ 3] = 1;
mask[ 4][ 0]=1; mask[ 4][ 1]=1; mask[ 4][ 2]=1; mask[ 4][ 3] = 1;
mask[ 5][ 0]=0; mask[ 5][ 1]=1; mask[ 5][ 2]=1; mask[ 5][ 3] = 1;
mask[ 6][ 0]=1; mask[ 6][ 1]=1; mask[ 6][ 2]=1; mask[ 6][ 3] = 1;
mask[ 7][ 0]=0; mask[ 7][ 1]=1; mask[ 7][ 2]=1; mask[ 7][ 3] = 1;
mask[ 8][ 0]=1; mask[ 8][ 1]=1; mask[ 8][ 2]=1; mask[ 8][ 3] = 1;
mask[ 9][ 0]=1; mask[ 9][ 1]=1; mask[ 9][ 2]=1; mask[ 9][ 3] = 0;
mask[10][ 0]=1; mask[10][ 1]=1; mask[10][ 2]=1; mask[10][ 3] = 1;
mask[11][ 0]=0; mask[11][ 1]=1; mask[11][ 2]=1; mask[11][ 3] = 1;
mask[12][ 0]=1; mask[12][ 1]=1; mask[12][ 2]=1; mask[12][ 3] = 1;

require 'pp'
pp Flock.kmeans(6, data, mask)
