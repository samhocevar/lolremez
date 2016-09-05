# roflmao

[![Build Status (Travis-CI)](https://travis-ci.org/lolengine/lol-roflmao.svg?branch=master)](https://travis-ci.org/lolengine/lol-roflmao)
[![Build Status (Semaphore-CI)](https://semaphoreci.com/api/v1/samhocevar/lol-roflmao/branches/master/badge.svg)](https://semaphoreci.com/samhocevar/lol-roflmao)

`roflmao` is a simple project using Lol Engine. If you want to get
started with Lol Engine, you may either:

 - fork this project
 - duplicate this project (see [“duplicating a repository”](https://help.github.com/articles/duplicating-a-repository/))

## Setup

Make sure Lol Engine and its submodules are properly initialised:

    git submodules update --init --recursive

On Linux, make sure the following packages are installed:

    automake autoconf libtool pkg-config
    libglew-dev
    libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev

## Configure

The default application is called `roflmao` and lies in its own subdirectory.
You should rename it to whatever your application will be called. Make sure
to modify the following files:

    configure.ac
    Makefile.am
    roflmao/Makefile.am
    roflmao/roflmao.cpp

You can of course have several projects in the same repository.

## Build

Then bootstrap the project and configure it:

    ./bootstrap
    ./configure

Finally, build the project:

    make

