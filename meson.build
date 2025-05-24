
project('priorityq', 'c',
        license: 'MIT',
        license_files: 'LICENSE',
        version: '0.0.0')
version = meson.project_version()

incdir = include_directories('include')
includes = files('include/priorityq.h')
sources = files('src/priorityq.c')

# Expected use-case is to build against static library.
priorityq = static_library('priorityq',
                    sources,
                    include_directories: incdir,
                    install: true)
install_headers(includes, subdir: 'priorityq')

# Unit tests
e_prove = executable('prove', 'test/bdd.h', 'test/prove.c', include_directories: incdir, link_with: priorityq)
test('prove library correctness', e_prove)

# Performance executables
e_calc = executable('calculate', 'test/calculate.c')
e_comp = executable('complexity', 'test/complexity.c', include_directories: incdir, link_with: priorityq, c_args: ['-O3'])

