#! /usr/bin/env python

from setuptools import setup, Extension
import commands

def pkg_config_cflags(pkgs):
    '''List all include paths that output by `pkg-config --cflags pkgs`'''
    return map(lambda path: path[2::], commands.getoutput('pkg-config --cflags-only-I %s' % (' '.join(pkgs))).split())

pulseaudio_mod = Extension('pulseaudio',
                include_dirs = pkg_config_cflags(['glib-2.0']),
                libraries = ['pulse'],
                sources = ['mainloop.c'],
                extra_compile_args= ['-Wall'])

deepin_pulseaudio_small_mod = Extension('deepin_pulseaudio_small',
                include_dirs = pkg_config_cflags(['glib-2.0']),
                libraries = ['pulse', 'pulse-mainloop-glib'],
                sources = ['deepin_pulseaudio_small.c'],
                extra_compile_args= ['-Wall'])

setup(name='pypa',
      version='0.1',
      ext_modules = [pulseaudio_mod],
      description='PulseAudio Python binding.',
      long_description ="""PulseAudio Python binding.""",
      author='',
      author_email='',
      license='GPL-3',
      url="",
      download_url="",
      platforms = ['Linux'],
      )
