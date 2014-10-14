================================================================================
Simple DirectMedia Layer for Native Client
================================================================================

Requirements: 
    
    * Native Client SDK (https://developer.chrome.com/native-client), 
      (tested with Pepper version 33 or higher).

The SDL backend for Chrome's Native Client has been tested only with the PNaCl
toolchain, which generates binaries designed to run on ARM and x86_32/64 
platforms. This does not mean it won't work with the other toolchains!

================================================================================
Building SDL for NaCl
================================================================================

Set up the right environment variables (see naclbuild.sh), then configure SDL with:

    configure --host=pnacl --prefix some/install/destination
    
Then "make". 

As an example of how to create a deployable app a Makefile project is provided 
in test/nacl/Makefile, which includes some monkey patching of the common.mk file 
provided by NaCl, without which linking properly to SDL won't work (the search 
path can't be modified externally, so the linker won't find SDL's binaries unless 
you dump them into the SDK path, which is inconvenient).
Also provided in test/nacl is the required support file, such as index.html, 
manifest.json, etc.
SDL apps for NaCl run on a worker thread using the ppapi_simple infrastructure.
This allows for blocking calls on all the relevant systems (OpenGL ES, filesystem),
hiding the asynchronous nature of the browser behind the scenes...which is not the
same as making it disappear!


================================================================================
Running tests
================================================================================

Due to the nature of NaCl programs, building and running SDL tests is not as
straightforward as one would hope. The script naclbuild.sh in build-scripts 
automates the process and should serve as a guide for users of SDL trying to build 
their own applications.

Basic usage:
    
    ./naclbuild.sh path/to/pepper/toolchain (i.e. ~/naclsdk/pepper_35)
    
This will build testgles2.c by default.

If you want to build a different test, for example testrendercopyex.c:
    
    SOURCES=~/sdl/SDL/test/testrendercopyex.c ./naclbuild.sh ~/naclsdk/pepper_35
    
Once the build finishes, you have to serve the contents with a web server (the
script will give you instructions on how to do that with Python).

================================================================================
RWops and nacl_io
================================================================================

SDL_RWops work transparently with nacl_io. Two functions are provided to control
mount points:
    
    int SDL_NaClMount(const char* source, const char* target, 
                      const char* filesystemtype, 
                      unsigned long mountflags, const void *data);
    int SDL_NaClUmount(const char *target);
    
    For convenience, SDL will by default mount an httpfs tree at / before calling 
the app's main function. Such setting can be overridden by calling:
    
    SDL_NaClUmount("/");

And then mounting a different filesystem at /

It's important to consider that the asynchronous nature of file operations on a
browser is hidden from the application, effectively providing the developer with
a set of blocking file operations just like you get in a regular desktop 
environment, which eases the job of porting to Native Client, but also introduces 
a set of challenges of its own, in particular when big file sizes and slow 
connections are involved.

For more information on how nacl_io and mount points work, see:
    
    https://developer.chrome.com/native-client/devguide/coding/nacl_io

================================================================================
TODO - Known Issues
================================================================================
* Testing of all systems with a real application (something other than SDL's tests)

