# Oolong
A programming language combining the usability of newer languages with the power
of old ones.

The long-term plan for this project is to turn it into my ideal programming
language.  This will undoubtedly take time and until I remove this statement,
please consider this highly experimental, highly subject to change, and not
production ready.

Example code (idealized, not necessarily implemented) can be found in the
*samples* directory.  These files will be how I figure out what I like and don't
like.

## Building and Running
Ensure the following dependencies are installed (rough version requirements):

 - cmake (>2.6)
 - llvm (>5.0.0)
 - bison (>3.0)
 - flex (>2.5)
 - g++/gcc

Build the project using:

    ./make.sh

Compile Oolong files using:

    ./oolong <path-to-file>

(e.g. `./oolong samples/hello-world.ool`).

Then run the output program via:

    ./a.out

## Building on OSX

If you're using homebrew, you should be able to install all the dependencies you
need using:

    brew install cmake llvm bison flex

For bison, flex, and llvm you will need to update your PATH to point to the
newer versions that will be made available to you.  For me this meant running:

    # add new PATH entries
    echo 'export PATH="/usr/local/opt/bison/bin:$PATH"' >> ~/.bash_profile
    echo 'export PATH="/usr/local/opt/flex/bin:$PATH"' >> ~/.bash_profile
    echo 'export PATH="/usr/local/opt/llvm/bin:$PATH"' >> ~/.bash_profile
    # source changes for current session
    . ~/.bash_profile

Once you have completed those steps you should be able to proceed as normal.

## Primary Philosophy
 - Explicit is better than implicit.
 - Implicit is better than redundant.
 - Simple is better than complex.
 - Complex is better than complicated.
 - Learn from old languages where they coincide.
 - Learn from new languages where they innovate.
 - Compile-time is your first code review.
 - Abbreviations should only occur where we would speak them.
 - "Danger" **isn't** a good reason to not include something.
 - "Magic" **is** a good reason to not include something.
 - Breaking changes stink but holding yourself back is worse.

## Proposed Language Features
 - First-class functions
 - Four access modifiers
   - private (class only)
   - protected (class-hierarchy only)
   - shared (namespace and class hierarchy)
   - public (all)
 - Constant (invariant) variables and methods (C++)
 - Nullability decided at declaration time (Swift)
 - Question-mark operator short-circuits at null values (Swift)
   - e.g. object?getValue()?toString() returns null if object or
       object.getValue() or object.getValue().toString() are null.
 - Scope-based memory management using reference counting
   - i.e. when the variable with the last reference goes out of scope
       the memory is released (destructors called)
 - Templates (not generics)

### Questions
 - Virtual by default?
   - The C++ approach is more efficient
   - The Java approach is more usable (leaning this way)
 - Distinguish between interfaces, abstract classes, and classes?
   - The C++ approach is more usable (leaning this way)
   - The Java approach is more explicit

