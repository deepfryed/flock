require_relative '../ext/flock'

module Flock
  def self.kcluster size, data, options = {}
    options[:sparse] = true if sparse?(data[0])
    if options[:sparse]
      data, options[:weights] = densify(data, options[:weights])
      options[:mask]          = nil
    end
    do_kcluster(size, data, options)
  end

  def self.self_organizing_map nx, ny, data, options = {}
    options[:sparse] = true if sparse?(data[0])
    if options[:sparse]
      data, options[:weights] = densify(data, options[:weights])
      options[:mask]          = nil
    end
    do_self_organizing_map(nx, ny, data, options)
  end

  def self.treecluster size, data, options = {}
    options[:sparse] = true if sparse?(data[0])
    if options[:sparse]
      data, options[:weights] = densify(data, options[:weights])
      options[:mask]          = nil
    end
    do_treecluster(size, data, options)
  end

  private

    def self.sparse? row
      row.kind_of?(Hash) or !row[0].kind_of?(Numeric)
    end

    def self.sparse_array? row
      !row.kind_of?(Hash)
    end

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
      dims, data = sparse_array?(sparse_data[0]) ? sparse_array_to_data(sparse_data) : sparse_hash_to_data(sparse_data)

      if weights
        resampled = Array.new(dims.size) {1}
        weights.each {|k,v| resampled[dims[k]] = v }
        weights   = resampled
      end

      [data, weights]
    end
end # Flock
