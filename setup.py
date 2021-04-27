#!usr/bin/env python
"""Orange: Machine learning and interactive data mining toolbox.

Orange is a scriptable environment for fast prototyping of new
algorithms and testing schemes. It is a collection of Python packages
that sit over the core library and implement some functionality for
which execution time is not crucial and which is easier done in Python
than in C++. This includes a variety of tasks such as attribute subset,
bagging and boosting, and alike.

Orange also includes a set of graphical widgets that use methods from
core library and Orange modules. Through visual programming, widgets
can be assembled together into an application by a visual programming
tool called Orange Canvas.
"""

DOCLINES = __doc__.split("\n")

from distutils.core import setup, Extension
from distutils.sysconfig import get_python_inc
import glob, os, sys, subprocess
try:
    import numpy
except:
    numpy = None

CLASSIFIERS = """\
Development Status :: 4 - Beta
Programming Language :: Python
License :: OSI Approved :: GNU General Public License (GPL)
Operating System :: POSIX
Operating System :: Microsoft :: Windows
Topic :: Scientific/Engineering :: Artificial Intelligence
Topic :: Scientific/Engineering :: Visualization
Intended Audience :: Education
Intended Audience :: Science/Research
"""

KEYWORDS = """\
data mining
machine learning
artificial intelligence
"""

NAME                = 'Orange'
DESCRIPTION         = DOCLINES[0]
LONG_DESCRIPTION    = "\n".join(DOCLINES[2:])
URL                 = "http://orange.biolab.si"
DOWNLOAD_URL        = "https://bitbucket.org/biolab/orange/downloads"
LICENSE             = 'GNU General Public License (GPL)'
CLASSIFIERS         = filter(None, CLASSIFIERS.split('\n'))
AUTHOR              = "Bioinformatics Laboratory, FRI UL"
AUTHOR_EMAIL        = "orange@fri.uni-lj.si"
KEYWORDS            = filter(None, KEYWORDS.split('\n'))
MAJOR               = 3
MINOR               = 0
MICRO               = 0
ISRELEASED          = False
VERSION             = '%d.%d.%d' % (MAJOR, MINOR, MICRO)

def run_pyxtract():
    os.chdir("source")
    if not os.path.exists("ppp"):
        os.mkdir("ppp")
    if not os.path.exists("px"):
        os.mkdir("px")
    subprocess.check_call([sys.executable, "pyxtract/pyprops.py", "-n", "orange"])
    os.chdir("..")

def get_source_files(path, ext="cpp", exclude=()):
    source_files = []
    import xml.dom.minidom
    for project in ("source/Orange.vcxproj",):
        projdir = project[:project.rfind("/")+1]
        vcproj = xml.dom.minidom.parse(open(project, encoding="utf8"))
        files = vcproj.getElementsByTagName("ClCompile")
        for fle in files:
            rpath = fle.getAttribute("Include")
            rpath = rpath.replace("\\", "/")
            cppfile = rpath # os.path.split(rpath)[1]
            if cppfile and not cppfile.endswith(".hpp"):
                source_files.append(projdir + cppfile)
    return source_files


python_include_dir = get_python_inc(plat_specific=1)

include_dirs = [python_include_dir, "source", "source/ppp", "source/px"]

if numpy:
    include_dirs.append(numpy.get_include())
    nonumpy = ""
else:
    numpy_include_dir = ""
    nonumpy = "-DNO_NUMPY"

run_pyxtract()

def setup_package():
    setup(name =NAME,
        description = DESCRIPTION,
        version = VERSION,
        author = AUTHOR,
        author_email = AUTHOR_EMAIL,
        url = URL,
        download_url = DOWNLOAD_URL,
        classifiers = CLASSIFIERS,
        long_description=LONG_DESCRIPTION,
        license = LICENSE,
        keywords = KEYWORDS,

        packages = (),
        package_data = (),
        ext_modules = [
            Extension(
                "orange",
                get_source_files("source/orange/"),
                include_dirs=include_dirs,
                extra_compile_args = ["-Wno-invalid-offsetof", "-Wno-write-strings", nonumpy],
                define_macros=[("_ASSERT", "")])])

if __name__ == '__main__':
    setup_package()
