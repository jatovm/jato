# Jato VM

## About

Jato is an implementation of the Java virtual machine. It includes a VM and a
JIT compiler for the x86 machine architecture and supports the JNI API. Jato
uses Boehm GC as its garbage collector and relies on GNU Classpath to provide
essential Java APIs.

## Compilation and Installation

### Getting the Sources

Fetch the latest sources with:

    git clone git://git.kernel.org/pub/scm/java/jato/jato.git

### Build Requirements

Before installing Jato ensure you have the following software installed on your
system:

  - GNU Classpath 0.99 or later:

    Release tarball:

    ftp://ftp.gnu.org/pub/gnu/classpath/classpath-0.99.tar.gz

    Git repository:

    git://git.savannah.gnu.org/classpath.git

  - Eclipse Java Compiler

Fedora 15:

    sudo yum install binutils-devel bison glib2-devel libffi-devel

Ubuntu 10.10:

    sudo apt-get install ecj libffi-dev binutils-dev libglib2.0-dev bison

Archlinux:

    pacman -S eclipse-ecj classpath libffi

### Building GNU Classpath

GNU Classpath is not present in most Linux distributions so you need to
build and install it yourself to run Jato. First download the sources
from:

  ftp://ftp.gnu.org/pub/gnu/classpath/classpath-0.99.tar.gz

Then install the following dependencies that are required to build GNU
Classpath:

    sudo apt-get install openjdk-6-jdk antlr libgconf2-dev libgtk2.0-dev ecj fastjar pccts

You can then compile GNU Classpath:

    ./configure --disable-Werror --disable-plugin
    make

and install it to /usr/local:

    sudo make install

### Building the Software

Compile the VM:

    make

### Testing and Installation

You can run all Jato unit and regression tests with the command:

    make check

All tests should pass.

In addition, you can download and run bunch of real-world tests with the
command:

    make torture

Note! This step is optional and can take a long time.

You can now install Jato with:

    make install

## Using Jato

Jato uses the same command line options as 'java'. You can execute a single
class with:

    jato <class name>

To specify classpath use:

    jato -cp <jar files or directories> <class name>

You can also execute a Jar file with:

    jato -jar <jar file>

Jato also supports variety of command line options for debugging and tracing
purposes. See the file Documentation/options.txt for details.

## Copyright and License

Copyright (C) 2005-2012  Pekka Enberg and contributors

Jato is available under the GNU General Public License (GPL) version 2 with the
following clarification and special exception:

    Linking this library statically or dynamically with other modules is making a
    combined work based on this library. Thus, the terms and conditions of the
    GNU General Public License cover the whole combination.

    As a special exception, the copyright holders of this library give you
    permission to link this library with independent modules to produce an
    executable, regardless of the license terms of these independent modules, and
    to copy and distribute the resulting executable under terms of your choice,
    provided that you also meet, for each linked independent module, the terms
    and conditions of the license of that module. An independent module is a
    module which is not derived from or based on this library. If you modify this
    library, you may extend this exception to your version of the library, but
    you are not obligated to do so. If you do not wish to do so, delete this
    exception statement from your version.

## Thanks and Acknowledgements

I would like to thank the following people and organizations for supporting
Jato development:

- Google for including Jato in Summer of Code 2008, 2009, and 2011.

- Kernel.org for providing git hosting for Jato.

- Reaktor Innovations Oy for sponsoring initial Jato development back in 2005.

Thank you!
