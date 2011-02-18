require 'rake'

begin
  require 'jeweler'
  Jeweler::Tasks.new do |gem|
    gem.name        = 'flock'
    gem.summary     = %q{Ruby bindings to Cluster 3.0.}
    gem.description = %q{A thin ruby binding to Cluster 3.0}
    gem.email       = %w{deepfryed@gmail.com}
    gem.homepage    = 'http://github.com/deepfryed/flock'
    gem.authors     = ["Bharanee Rathna"]
    gem.extensions = FileList['ext/extconf.rb']
    gem.files.reject!{|f| f =~ %r{\.gitignore|examples/.*}}
  end
  Jeweler::GemcutterTasks.new
rescue LoadError
  puts "Jeweler (or a dependency) not available. Install it with: gem install jeweler"
end
