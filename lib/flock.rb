require_relative '../ext/flock'
module Flock

  def self.sparse_hash_to_data sparse_data
    dims = Hash[sparse_data.map(&:keys).flatten.uniq.map.with_index{|k,v| [k,v]}]
    data = sparse_data.map do |sv|
      vector = Array.new(dims.size) {0}
      sv.each {|k,v| vector[dims[k]] = v }
      vector
    end
    [dims,data]
  end

  def self.sparse_array_to_data sparse_data
    dims = Hash[sparse_data.flatten.uniq.map.with_index{|k,v| [k,v]}]
    data = sparse_data.map do |sv|
      vector = Array.new(dims.size) {0}
      sv.each {|k| vector[dims[k]] = 1 }
      vector
    end
    [dims,data]
  end

  def self.sparse_kmeans size, sparse_data, options={}
    dims, data = sparse_data[0].kind_of?(Array) ? sparse_array_to_data(sparse_data) : sparse_hash_to_data(sparse_data)

    if options.key?(:weights)
      weights = Array.new(dims.size) {1}
      options[:weights].each {|k,v| weights[dims[k]] = v }
      options[:weights] = weights
    end

    kmeans(size, data, nil, options)
  end
end
