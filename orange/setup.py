from distutils.core import setup, Extension
from distutils.command.build_ext import build_ext
from distutils.command.install_lib import install_lib

import os, sys, re
import glob

from subprocess import check_call

from types import *

from distutils.dep_util import newer_group
from distutils.file_util import copy_file
from distutils import log

from distutils.sysconfig import get_python_inc, get_config_var
import numpy
numpy_include_dir = numpy.get_include();
python_include_dir = get_python_inc(plat_specific=1)

include_dirs = [python_include_dir, numpy_include_dir, "source/include"]

if sys.platform == "darwin":
    extra_compile_args = "-fPIC -fpermissive -fno-common -w -DDARWIN ".split()
    extra_link_args = "-headerpad_max_install_names -undefined dynamic_lookup -lstdc++ -lorange_include".split()
elif sys.platform == "win32":
    extra_compile_args = []
    extra_link_args = []
elif sys.platform == "linux2":
    extra_compile_args = "-fPIC -fpermissive -w -DLINUX".split()
    extra_link_args = ["-Wl,-R$ORIGIN"]    
#    extra_link_args = ["-Wl,-R'$ORIGIN'"] #use this if distutils runs commands though the shell
else:
    extra_compile_args = []
    extra_link_args = []
    
define_macros=[('NDEBUG', '1'),
               ('HAVE_STRFTIME', None)]
        
class LibStatic(Extension):
    pass

class PyXtractExtension(Extension):
    def __init__(self, *args, **kwargs):
        for name, default in [("extra_pyxtract_cmds", []), ("lib_type", "dynamic")]:
            setattr(self, name, kwargs.get(name, default))
            if name in kwargs:    
                del kwargs[name]
            
        Extension.__init__(self, *args, **kwargs)
        
class PyXtractSharedExtension(PyXtractExtension):
    pass
        
class pyxtract_build_ext(build_ext):
    
    def run_pyxtract(self, ext, dir):
        original_dir = os.path.realpath(os.path.curdir)
        log.info("running pyxtract for %s" % ext.name)
        try:
            os.chdir(dir)
            ## we use the commands which are used for building under windows
            pyxtract_cmds = [cmd.split() for cmd in getattr(ext, "extra_pyxtract_cmds", [])]
            if os.path.exists("_pyxtract.bat"): 
                pyxtract_cmds.extend([cmd.split()[1:] for cmd in open("_pyxtract.bat").read().strip().splitlines()])
            for cmd in pyxtract_cmds:
                log.info(" ".join([sys.executable] + cmd))
                check_call([sys.executable] + cmd)
            if pyxtract_cmds:
                ext.include_dirs.append(os.path.join(dir, "ppp"))
                ext.include_dirs.append(os.path.join(dir, "px"))

        finally:
            os.chdir(original_dir)
        
    def finalize_options(self):
        build_ext.finalize_options(self)
        self.library_dirs.append(self.build_lib) # add the build lib dir (for liborange_include)
        
    def build_extension(self, ext):        
        if isinstance(ext, LibStatic):
            self.build_static(ext)
        elif isinstance(ext, PyXtractExtension):
            self.build_pyxtract(ext)
        else:
            build_ext.build_extension(self, ext)
            
        if isinstance(ext, PyXtractSharedExtension):
            # Make lib{name}.so link to {name}.so
            ext_path = self.get_ext_fullpath(ext.name)
            ext_path, ext_filename = os.path.split(ext_path)
            realpath = os.path.realpath(os.curdir)
#            print realpath, ext_path
            try:
                os.chdir(ext_path)
#                copy_file(os.path.join(ext_path, ext_filename), os.path.join(ext_path, "lib"+ext_filename), link="sym")
                lib_filename = self.compiler.library_filename(ext.name, lib_type="shared")
                ext.install_shared_link = (lib_filename, self.get_ext_filename(ext.name)) # store (lib_name, path) tuple to install shared library link to /usr/lib in the install_lib command
#                print realpath, ext_path, lib_filename
                copy_file(ext_filename, lib_filename, link="sym")
#                copy_file(ext_filename, lib_filename, link="sym")
            except OSError as ex:
                log.info("failed to create shared library for %s: %s" % (ext.name, str(ex)))
            finally:
                os.chdir(realpath)
            
    def build_pyxtract(self, ext):
        ## mostly copied from build_extension, changed
        sources = ext.sources
        if sources is None or type(sources) not in (ListType, TupleType):
            raise DistutilsSetupError(("in 'ext_modules' option (extension '%s'), " +
                   "'sources' must be present and must be " +
                   "a list of source filenames") % ext.name)
        sources = list(sources)
        
        ext_path = self.get_ext_fullpath(ext.name)
#        output_dir, _ = os.path.split(ext_path)
#        lib_filename = self.compiler.library_filename(ext.name, lib_type='static', output_dir=output_dir)
        
        depends = sources + ext.depends
        if not (self.force or newer_group(depends, ext_path, 'newer')):
            log.debug("skipping '%s' extension (up-to-date)", ext.name)
            return
        else:
            log.info("building '%s' extension", ext.name)

        # First, scan the sources for SWIG definition files (.i), run
        # SWIG on 'em to create .c files, and modify the sources list
        # accordingly.
        sources = self.swig_sources(sources, ext)
        
        # Run pyxtract in dir this adds ppp and px dirs to include_dirs
        dir = os.path.commonprefix([os.path.split(s)[0] for s in ext.sources])
        self.run_pyxtract(ext, dir)

        # Next, compile the source code to object files.

        # XXX not honouring 'define_macros' or 'undef_macros' -- the
        # CCompiler API needs to change to accommodate this, and I
        # want to do one thing at a time!

        # Two possible sources for extra compiler arguments:
        #   - 'extra_compile_args' in Extension object
        #   - CFLAGS environment variable (not particularly
        #     elegant, but people seem to expect it and I
        #     guess it's useful)
        # The environment variable should take precedence, and
        # any sensible compiler will give precedence to later
        # command line args.  Hence we combine them in order:
        extra_args = ext.extra_compile_args or []

        macros = ext.define_macros[:]
        for undef in ext.undef_macros:
            macros.append((undef,))

        objects = self.compiler.compile(sources,
                                         output_dir=self.build_temp,
                                         macros=macros,
                                         include_dirs=ext.include_dirs,
                                         debug=self.debug,
                                         extra_postargs=extra_args,
                                         depends=ext.depends)

        # XXX -- this is a Vile HACK!
        #
        # The setup.py script for Python on Unix needs to be able to
        # get this list so it can perform all the clean up needed to
        # avoid keeping object files around when cleaning out a failed
        # build of an extension module.  Since Distutils does not
        # track dependencies, we have to get rid of intermediates to
        # ensure all the intermediates will be properly re-built.
        #
        self._built_objects = objects[:]

        # Now link the object files together into a "shared object" --
        # of course, first we have to figure out all the other things
        # that go into the mix.
        if ext.extra_objects:
            objects.extend(ext.extra_objects)
        extra_args = ext.extra_link_args or []

        # Detect target language, if not provided
        language = ext.language or self.compiler.detect_language(sources)

        self.compiler.link_shared_object(
            objects, ext_path,
            libraries=self.get_libraries(ext),
            library_dirs=ext.library_dirs,
            runtime_library_dirs=ext.runtime_library_dirs,
            extra_postargs=extra_args,
            export_symbols=self.get_export_symbols(ext),
            debug=self.debug,
            build_temp=self.build_temp,
            target_lang=language)
        
        
    def build_static(self, ext):
        ## mostly copied from build_extension, changed
        sources = ext.sources
        if sources is None or type(sources) not in (ListType, TupleType):
            raise DistutilsSetupError(("in 'ext_modules' option (extension '%s'), " +
                   "'sources' must be present and must be " +
                   "a list of source filenames") % ext.name)
        sources = list(sources)
        
        ext_path = self.get_ext_fullpath(ext.name)
        output_dir, _ = os.path.split(ext_path)
        lib_filename = self.compiler.library_filename(ext.name, lib_type='static', output_dir=output_dir)
        
        depends = sources + ext.depends
        if not (self.force or newer_group(depends, lib_filename, 'newer')):
            log.debug("skipping '%s' extension (up-to-date)", ext.name)
            return
        else:
            log.info("building '%s' extension", ext.name)

        # First, scan the sources for SWIG definition files (.i), run
        # SWIG on 'em to create .c files, and modify the sources list
        # accordingly.
        sources = self.swig_sources(sources, ext)

        # Next, compile the source code to object files.

        # XXX not honouring 'define_macros' or 'undef_macros' -- the
        # CCompiler API needs to change to accommodate this, and I
        # want to do one thing at a time!

        # Two possible sources for extra compiler arguments:
        #   - 'extra_compile_args' in Extension object
        #   - CFLAGS environment variable (not particularly
        #     elegant, but people seem to expect it and I
        #     guess it's useful)
        # The environment variable should take precedence, and
        # any sensible compiler will give precedence to later
        # command line args.  Hence we combine them in order:
        extra_args = ext.extra_compile_args or []

        macros = ext.define_macros[:]
        for undef in ext.undef_macros:
            macros.append((undef,))

        objects = self.compiler.compile(sources,
                                         output_dir=self.build_temp,
                                         macros=macros,
                                         include_dirs=ext.include_dirs,
                                         debug=self.debug,
                                         extra_postargs=extra_args,
                                         depends=ext.depends)

        # XXX -- this is a Vile HACK!
        #
        # The setup.py script for Python on Unix needs to be able to
        # get this list so it can perform all the clean up needed to
        # avoid keeping object files around when cleaning out a failed
        # build of an extension module.  Since Distutils does not
        # track dependencies, we have to get rid of intermediates to
        # ensure all the intermediates will be properly re-built.
        #
        self._built_objects = objects[:]

        # Now link the object files together into a "shared object" --
        # of course, first we have to figure out all the other things
        # that go into the mix.
        if ext.extra_objects:
            objects.extend(ext.extra_objects)
        extra_args = ext.extra_link_args or []

        # Detect target language, if not provided
        language = ext.language or self.compiler.detect_language(sources)
        
        #first remove old library (ar only appends the contents if archive already exists
        try:
            os.remove(lib_filename)
        except OSError as ex:
            log.debug("failed to remove obsolete static library %s: %s" %(ext.name, str(ex)))

        self.compiler.create_static_lib(
            objects, ext.name, output_dir,
            debug=self.debug,
            target_lang=language)
        
    if not hasattr(build_ext, "get_ext_fullpath"):
        #On mac OS X python 2.6.1 distutils does not have this method
        def get_ext_fullpath(self, ext_name):
            """Returns the path of the filename for a given extension.
            
            The file is located in `build_lib` or directly in the package
            (inplace option).
            """
            import string
            # makes sure the extension name is only using dots
            all_dots = string.maketrans('/' + os.sep, '..')
            ext_name = ext_name.translate(all_dots)
            fullname = self.get_ext_fullname(ext_name)
            modpath = fullname.split('.')
            filename = self.get_ext_filename(ext_name)
            filename = os.path.split(filename)[-1]
            if not self.inplace:
                # no further work needed
                # returning :
                #   build_dir/package/path/filename
                filename = os.path.join(*modpath[:-1] + [filename])
                return os.path.join(self.build_lib, filename)
            # the inplace option requires to find the package directory
            # using the build_py command for that
            package = '.'.join(modpath[0:-1])
            build_py = self.get_finalized_command('build_py')
            package_dir = os.path.abspath(build_py.get_package_dir(package))
            # returning
            #   package_dir/filename
            return os.path.join(package_dir, filename)
        
class install_shared(install_lib):
    def run(self):
        install_lib.run(self)
        
    def copy_tree(self, infile, outfile, preserve_mode=1, preserve_times=1, preserve_symlinks=1, level=1):
        """ Run copy_tree with preserve_symlinks=1 as default
        """
        install_lib.copy_tree(self, infile, outfile, preserve_mode, preserve_times, preserve_symlinks, level)
        
            
def get_source_files(path, ext="cpp"):
    return glob.glob(os.path.join(path, "*." + ext))

include_ext = LibStatic("orange_include", get_source_files("source/include/"), include_dirs=include_dirs)

libraries = ["stdc++", "orange_include"]

orange_ext = PyXtractSharedExtension("orange", get_source_files("source/orange/") + get_source_files("source/orange/blas/", "c"),
                                      include_dirs=include_dirs,
                                      extra_compile_args = extra_compile_args + ["-DORANGE_EXPORTS"],
                                      extra_link_args = extra_link_args,
                                      libraries=libraries,
                                      extra_pyxtract_cmds = ["../pyxtract/defvectors.py"],
#                                      depends=["orange/ppp/lists"]
                                      )

if sys.platform == "darwin":
    build_shared_cmd = get_config_var("BLDSHARED")
    if "-bundle" in build_shared_cmd.split(): #dont link liborange.so with orangeom and orangene - MacOS X treats loadable modules and shared libraries different
        shared_libs = libraries
    else:
        shared_libs = libraries + ["orange"]
else:
    shared_libs = libraries + ["orange"]
    
orangeom_ext = PyXtractExtension("orangeom", get_source_files("source/orangeom/") + get_source_files("source/orangeom/qhull/", "c"),
                                  include_dirs=include_dirs + ["source/orange/"],
                                  extra_compile_args = extra_compile_args + ["-DORANGEOM_EXPORTS"],
                                  extra_link_args = extra_link_args,
                                  libraries=shared_libs,
                                  )

orangene_ext = PyXtractExtension("orangene", get_source_files("source/orangene/"), 
                                  include_dirs=include_dirs + ["source/orange/"], 
                                  extra_compile_args = extra_compile_args + ["-DORANGENE_EXPORTS"],
                                  extra_link_args = extra_link_args,
                                  libraries=shared_libs,
                                  )

corn_ext = Extension("corn", get_source_files("source/corn/"), 
                     include_dirs=include_dirs + ["source/orange/"], 
                     extra_compile_args = extra_compile_args + ["-DCORN_EXPORTS"],
                     extra_link_args = extra_link_args,
                     libraries=libraries
                     )

statc_ext = Extension("statc", get_source_files("source/statc/"), 
                      include_dirs=include_dirs + ["source/orange/"], 
                      extra_compile_args = extra_compile_args + ["-DSTATC_EXPORTS"],
                      extra_link_args = extra_link_args,
                      libraries=libraries
                      )
 

pkg_re = re.compile("Orange/(.+?)/__init__.py")
packages = ["Orange"] + ["Orange." + pkg_re.findall(p)[0] for p in glob.glob("Orange/*/__init__.py")]
setup(cmdclass={"build_ext": pyxtract_build_ext, "install_lib": install_shared},
      name ="Orange",
      version = "2.0.0b",
      description = "Orange data mining library for python.",
      author = "Bioinformatics Laboratory, FRI UL",
      author_email = "orange@fri.uni-lj.si",
      maintainer = "Ales Erjavec",
      maintainer_email = "ales.erjavec@fri.uni-lj.si",
      url = "http://www.ailab.si/orange",
      download_url = "http://www.ailab.si/svn/orange/trunk",
      packages = packages + [".",
                             "OrangeCanvas", 
                             "OrangeWidgets", 
                             "OrangeWidgets.Associate",
                             "OrangeWidgets.Classify",
                             "OrangeWidgets.Data",
                             "OrangeWidgets.Evaluate",
                             "OrangeWidgets.Prototypes",
                             "OrangeWidgets.Regression",
                             "OrangeWidgets.Unsupervised",
                             "OrangeWidgets.Visualize",
                             "doc",
                             ],
      package_data = {"OrangeCanvas": ["icons/*.png", "orngCanvas.pyw", "WidgetTabs.txt"],
                      "OrangeWidgets": ["icons/*.png", "icons/backgrounds/*.png", "report/index.html"],
                      "OrangeWidgets.Associate": ["icons/*.png"],
                      "OrangeWidgets.Classify": ["icons/*.png"],
                      "OrangeWidgets.Data": ["icons/*.png"],
                      "OrangeWidgets.Evaluate": ["icons/*.png"],
                      "OrangeWidgets.Prototypes": ["icons/*.png"],
                      "OrangeWidgets.Regression": ["icons/*.png"],
                      "OrangeWidgets.Unsupervised": ["icons/*.png"],
                      "OrangeWidgets.Visualize": ["icons/*.png"],
                      "doc": ["datasets/*.tab", ]
                      },
      ext_modules = [include_ext, orange_ext, orangeom_ext, orangene_ext, corn_ext, statc_ext],
      extra_path=("orange", "orange"),
      scripts = ["orange-canvas"],
      license = "GNU General Public License (GPL)",
      keywords = ["data mining", "machine learning", "artificial intelligence"],
      classifiers = ["Development Status :: 4 - Beta",
                     "Programming Language :: Python",
                     "License :: OSI Approved :: GNU General Public License (GPL)",
                     "Operating System :: POSIX",
                     "Operating System :: Microsoft :: Windows",
                     "Topic :: Scientific/Engineering :: Artificial Intelligence",
                     "Topic :: Scientific/Engineering :: Visualization",
                     "Intended Audience :: Education",
                     "Intended Audience :: Science/Research"
                     ],
      long_description="""\
Orange data mining library
==========================

Orange is a scriptable environment for fast prototyping of new
algorithms and testing schemes. It is a collection of Python-based modules
that sit over the core library and implement some functionality for
which execution time is not crucial and which is easier done in Python
than in C++. This includes a variety of tasks such as pretty-print of
decision trees, attribute subset, bagging and boosting, and alike.

Orange also includes a set of graphical widgets that use methods from core
library and Orange modules. Through visual programming, widgets can be assembled
together into an application by a visual programming tool called Orange Canvas.
"""
      )
      

