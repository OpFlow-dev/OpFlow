#!/usr/bin/env python

import sys
import setuptools
from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext
from git_version import git_version

class get_pybind_include(object):
    """Helper class to determine the pybind11 include path

    The purpose of this class is to postpone importing pybind11
    until it is actually installed, so that the ``get_include()``
    method can be invoked. """

    def __init__(self, user=False):
        self.user = user

    def __str__(self):
        import pybind11
        return pybind11.get_include(self.user)


# As of Python 3.6, CCompiler has a `has_flag` method.
# cf http://bugs.python.org/issue26689
def has_flag(compiler, flagname):
    """Return a boolean indicating whether a flag name is supported on
    the specified compiler.
    """
    import tempfile
    with tempfile.NamedTemporaryFile('w', suffix='.cpp') as f:
        f.write('int main (int argc, char **argv) { return 0; }')
        try:
            compiler.compile([f.name], extra_postargs=[flagname])
        except setuptools.distutils.errors.CompileError:
            return False
    return True


def cpp_flag(compiler):
    """Return the -std=c++[11/14] compiler flag.

    The c++14 is prefered over c++11 (when it is available).
    """
    if has_flag(compiler, '-std=c++14'):
        return '-std=c++14'
    elif has_flag(compiler, '-std=c++11'):
        return '-std=c++11'
    else:
        raise RuntimeError('Unsupported compiler -- at least C++11 support '
                           'is needed!')


class BuildExt(build_ext):
    """A custom build extension for adding compiler-specific options."""
    c_opts = {
        'msvc': ['/EHsc'],
        'unix': [],
    }

    if sys.platform == 'darwin':
        c_opts['unix'] += ['-stdlib=libc++', '-mmacosx-version-min=10.7']

    def build_extensions(self):
        ct = self.compiler.compiler_type
        opts = self.c_opts.get(ct, [])
        link_args = []

        if ct == 'unix':
            opts.append('-DVERSION_INFO="%s"' % self.distribution.get_version())
            opts.append(cpp_flag(self.compiler))
            if has_flag(self.compiler, '-fvisibility=hidden'):
                opts.append('-fvisibility=hidden')
        elif ct == 'msvc':
            opts.append('/DVERSION_INFO=\\"%s\\"' % self.distribution.get_version())

        if has_flag(self.compiler, '-fopenmp'):
            opts.append('-fopenmp')
            link_args.append('-fopenmp')
        elif has_flag(self.compiler, '-openmp'):
            opts.append('-openmp')
            link_args.append('-openmp')

        for ext in self.extensions:
            ext.extra_compile_args = opts
            ext.extra_link_args = link_args

        build_ext.build_extensions(self)


setup(
        name='pyamgcl',
        version=git_version(),
        description='Solution of large sparse linear systems with Algebraic Multigrid Method',
        author='Denis Demidov',
        author_email='dennis.demidov@gmail.com',
        license='MIT',
        url='https://github.com/ddemidov/amgcl',
        packages=['pyamgcl'],
        include_package_data=True,
        exclude_package_data={'': ['CMakeLists.txt', 'pybind11']},
        zip_safe=False,
        ext_modules=[
            Extension('pyamgcl.pyamgcl_ext', ['pyamgcl/pyamgcl.cpp'],
                include_dirs=[
                    '.',
                    get_pybind_include(),
                    get_pybind_include(user=True)
                    ],
                language='c++'
                )
            ],
        install_requires=['pybind11>=1.7'],
        cmdclass={'build_ext': BuildExt},
)
