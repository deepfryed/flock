require_relative '../ext/flock'

# Ruby bindings to data clustering algorithms provided by
# {Cluster 3.0}[http://bonsai.hgc.jp/~mdehoon/software/cluster/software.htm]
#
# == Algorithms implemented
#
# * K-Means, K-Medians, K-Means++
# * Self-Organizing Maps
# * Tree Cluster or Hierarchical Clustering
#
# == Synopsis
#
#   require 'pp'
#   require 'flock'
#
#   # sparse data.
#   data = []
#   data << %w(apple orange)
#   data << %w(black white)
#   data << %w(white cyan)
#   data << %w(apple orange)
#   data << %w(apple)
#
#   pp Flock.kcluster(2, data, sparse: true, seed: Flock::SEED_RANDOM)
#   pp Flock.kcluster(2, data, sparse: true, seed: Flock::SEED_KMEANS_PLUSPLUS)
#   pp Flock.kcluster(2, data, sparse: true, seed: Flock::SEED_SPREADOUT)
#
#   # dense data.
#   data     = Array.new(13) {[]}
#   mask     = Array.new(13) {[]}
#   weights  = Array.new(13) {1.0}
#
#   data[0][0] = 0.1; data[0][1] = 0.0;
#   data[1][0] = 1.4; data[1][1] = 1.3;
#   data[2][0] = 1.2; data[2][1] = 2.5;
#   data[3][0] = 2.3; data[3][1] = 1.5;
#   data[4][0] = 1.7; data[4][1] = 0.7;
#   data[5][0] = 0.0; data[5][1] = 3.9;
#   data[6][0] = 6.7; data[6][1] = 3.9;
#
#   mask[0][0] = 1;   mask[0][1] = 1;
#   mask[1][0] = 1;   mask[1][1] = 1;
#   mask[2][0] = 1;   mask[2][1] = 1;
#   mask[3][0] = 1;   mask[3][1] = 1;
#   mask[4][0] = 1;   mask[4][1] = 1;
#   mask[5][0] = 0;   mask[5][1] = 1;
#   mask[6][0] = 1;   mask[6][1] = 1;
#
#   pp Flock.kcluster(2, data, mask: mask, weights: weights)
#
#
# == See
# * examples/* for more examples.
# * README.rdoc for more details.
# * API.rdoc is a public API overview.
module Flock

  # Cluster using k-means and k-medians.
  #
  # @example
  #
  #   data = []
  #   data << %w(apple orange)
  #   data << %w(black white)
  #   data << %w(white cyan)
  #   data << %w(apple orange)
  #   data << %w(apple)
  #   result = Flock.kcluster(2, data, sparse: true, seed: Flock::SEED_RANDOM)
  #
  # @param [Fixnum] size  number of clusters the data points are grouped into.
  # @param [Array]  data  An array of arrays of sparse or dense data, or an array of hashes of sparse data. Dense data
  #                       should always be in numeric form. Sparse data values are converted to a dense row format
  #                       by looking at the unique values and then converting each data point into a numeric vector
  #                       that represents the presence or absence of a value in that data point.
  # @option options [Array]       :mask       An array of arrays of 1s and 0s denoting if an element in the datapoint is
  #                                           to be used for computing distance (defaults to: all 1 vectors).
  # @option options [Array]       :weights    Numeric weight for each data point (defaults to: all 1 vector).
  # @option options [true, false] :transpose  Transpose the dense data matrix (defaults to: false).
  # @option options [Fixnum]      :iterations Number of iterations to be run (defaults to: 100).
  # @option options [Fixnum]      :method     Clustering method
  #                                             - Flock::METHOD_AVERAGE (default)
  #                                             - Flock::METHOD_MEDIAN
  # @option options [Fixnum]      :metric     Distance measure, one of the following
  #                                             - Flock::METRIC_EUCLIDIAN (default)
  #                                             - Flock::METRIC_CITY_BLOCK
  #                                             - Flock::METRIC_CORRELATION
  #                                             - Flock::METRIC_ABSOLUTE_CORRELATION
  #                                             - Flock::METRIC_UNCENTERED_CORRELATION
  #                                             - Flock::METRIC_ABSOLUTE_UNCENTERED_CORRELATION
  #                                             - Flock::METRIC_SPEARMAN
  #                                             - Flock::METRIC_KENDALL
  # @option options [Fixnum]      :seed       Initial seeding of clusters
  #                                             - Flock::SEED_RANDOM (default)
  #                                             - Flock::SEED_KMEANS_PLUSPLUS
  #                                             - Flock::SEED_SPREADOUT
  # @return [Hash]
  #   {
  #     :cluster  => [Array],
  #     :centroid => [Array<Array>],
  #     :error    => [Numeric],
  #     :repeated => [Fixnum]
  #   }
  def self.kcluster size, data, options = {}
    options[:sparse] = true if sparse?(data[0])
    if options[:sparse]
      data, options[:weights] = densify(data, options[:weights])
      options[:mask]          = nil
    end
    do_kcluster(size, data, options)
  end

  # Arranges data points on a 2D grid without having to specify a fixed cluster size. So in theory you could have
  # a maximum of nxm clusters.
  #
  # @example
  #
  #   data = []
  #   data << %w(apple orange)
  #   data << %w(black white)
  #   data << %w(white cyan)
  #   data << %w(apple orange)
  #   data << %w(apple)
  #   result = Flock.self_organizing_map(2, 2, data, sparse: true)
  #
  # @param  [Fixnum]  nx          Grid size in 1st dimension (x)
  # @param  [Fixnum]  ny          Grid size in 2nd dimension (y)
  # @param  [Array]   data        See Flock#kcluster
  # @option options   [Array]       :mask       See Flock#kcluster
  # @option options   [true, false] :transpose  See Flock#kcluster
  # @option options   [Fixnum]      :iterations See Flock#kcluster
  # @option options   [Fixnum]      :metric     See Flock#kcluster
  # @option options   [Numeric]     :tau        Initial tau value for distance metric.
  # @return [Hash]
  #   {
  #     :cluster  => [Array<Array>],
  #     :centroid => [Array<Array>]
  #   }
  def self.self_organizing_map nx, ny, data, options = {}
    options[:sparse] = true if sparse?(data[0])
    if options[:sparse]
      data, options[:weights] = densify(data, options[:weights])
      options[:mask]          = nil
    end
    do_self_organizing_map(nx, ny, data, options)
  end

  # Clusters data into hierarchies and then returns the clusters required using cut-tree.
  #
  # @example
  #
  #   data = []
  #   data << %w(apple orange)
  #   data << %w(black white)
  #   data << %w(white cyan)
  #   data << %w(apple orange)
  #   data << %w(apple)
  #   result = Flock.treecluster(2, data, sparse: true)
  #
  # @param  [Fixnum]  size        Number of clusters required. (See Flock#kcluster)
  # @param  [Array]   data        See Flock#kcluster
  # @option options   [Array]       :mask       See Flock#kcluster
  # @option options   [true, false] :transpose  See Flock#kcluster
  # @option options   [Fixnum]      :iterations See Flock#kcluster
  # @option options   [Fixnum]      :metric     See Flock#kcluster
  # @option options   [Fixnum]      :method     Method to use for treecluster
  #                                               - Flock::METHOD_SINGLE_LINKAGE
  #                                               - Flock::METHOD_MAXIMUM_LINKAGE
  #                                               - Flock::METHOD_AVERAGE_LINKAGE (default)
  #                                               - Flock::METHOD_CENTROID_LINKAGE
  # @return [Hash]
  #   {
  #     :cluster => [Array]
  #   }
  def self.treecluster size, data, options = {}
    options[:sparse] = true if sparse?(data[0])
    if options[:sparse]
      data, options[:weights] = densify(data, options[:weights])
      options[:mask]          = nil
    end
    do_treecluster(size, data, options)
  end

  # @deprecated use {kcluster} instead.
  def self.kmeans size, data, options = {}
    kcluster(size, data, options)
  end

  # @deprecated use {kcluster}(size, data, sparse: true, ...) instead.
  def self.sparse_kmeans size, data, options = {}
    kcluster(size, data, options.merge(sparse: true))
  end

  # @deprecated use {treecluster}(size, data, sparse: true, ...) instead.
  def self.sparse_treecluster size, data, options = {}
    treecluster(size, data, options.merge(sparse: true))
  end

  # @deprecated use {self_organizing_map}(nx, ny, data, sparse: true, ...) instead.
  def self.sparse_self_organizing_map nx, ny, data, options = {}
    self_organizing_map(nx, ny, data, options.merge(sparse: true))
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
