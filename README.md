## roflmao

`roflmao` is a simple project using Lol Engine. If you want to get
started with Lol Engine, you should fork this project.

# Setup

Make sure Lol Engine and its submodules are properly initialised:

    git submodules update --init --recursive

# Configure

The default application is called `roflmao` and lies in its own subdirectory.
You should rename it to whatever your application will be called. Make sure
to modify the following files:

  - `configure.ac`
  - `Makefile.am`
  - `roflmao/Makefile.am`
  - `roflmao/roflmao.cpp`

# Build

Then bootstrap the project and configure it:

    ./bootstrap
    ./configure

Finally, build the project:

    make

# Develop

The sample application is in `roflmao`
