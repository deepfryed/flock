require_relative '../ext/flock'
module Flock

  def self.sparse_hash_to_data sparse_data
    dims = Hash[sparse_data.map(&:keys).flatten.uniq.map.with_index{|k,v| [k,v]}]
    data = sparse_data.map do |sv|
      vector = Array.new(dims.size) {0}
      sv.each {|k,v| vector[dims[k]] = v }
      vector
    end
    [dims, data]
  end

  def self.sparse_array_to_data sparse_data
    dims = Hash[sparse_data.flatten.uniq.map.with_index{|k,v| [k,v]}]
    data = sparse_data.map do |sv|
      vector = Array.new(dims.size) {0}
      sv.each {|k| vector[dims[k]] = 1 }
      vector
    end
    [dims, data]
  end

  def self.densify sparse_data, weights = nil
    dims, data = sparse_data[0].kind_of?(Array) ? sparse_array_to_data(sparse_data) : sparse_hash_to_data(sparse_data)

    if weights
      resampled = Array.new(dims.size) {1}
      weights.each {|k,v| resampled[dims[k]] = v }
      weights   = resampled
    end

    [data, weights]
  end

  def self.sparse_kmeans size, sparse_data, options = {}
    data, options[:weights] = densify(sparse_data, options[:weights])
    kmeans(size, data, data, options)
  end

  def self.sparse_self_organizing_map nx, ny, sparse_data, options = {}
    data, options[:weights] = densify(sparse_data, options[:weights])
    self_organizing_map(nx, ny, data, data, options)
  end

  def self.sparse_treecluster size, sparse_data, options = {}
    data, options[:weights] = densify(sparse_data, options[:weights])
    treecluster(size, data, data, options)
  end
end
