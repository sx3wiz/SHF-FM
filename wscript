def options(cnf):
  cnf.add_option('--optimize', action='store', default=False, help='Adds compilier optimizations.')
  cnf.load('compiler_c compiler_cxx')


def configure(cnf):
  cnf.env.CXXFLAGS = ['-std=gnu++14', '-Wall']

  if cnf.options.optimize == "True":
    cnf.env.append_value('CXXFLAGS', ['-O2'])
  else:
    cnf.env.append_value('CXXFLAGS', ['-g'])

  print(cnf.env.CXXFLAGS)

  cnf.load('compiler_c compiler_cxx')

def build(bld):
  bld.program(
      features='cxx cprogram',
      source='main.cpp',
      target='tx_shf_fm',
      lib=['boost_system', 'uhd', 'pthread', 'volk', 'gnuradio-runtime', 'gnuradio-filter', 'dl', 'asound'])
