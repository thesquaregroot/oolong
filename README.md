# oolong
A programming language combining the usability of newer languages with the power
of old ones.

The long-term plan for this project is to turn it into my ideal programming
language.  This will undoubtedly take time and until I remove this statement,
please consider this highly experimental, highly subject to change, and not
production ready.

Example code (idealized, not necessarily implemented) can be found in the
*samples* directory.  These files will be how I figure out what I like and don't
like.

## Primary Philosophy
    -   Explicit is better than implicit.
    -   Implicit is better than redundant.
    -   Simple is better than complex.
    -   Complex is better than complicated.
    -   Learn from old languages where the coincide.
    -   Learn from new languages where they innovate.
    -   Disregard new languages where they don't.
    -   Compile-time is your first code review.
    -   Abbreviations should only occur where we would speak them.
    -   "Danger" **isn't** a good reason to not include something.
    -   "Magic" **is** a good reason to not include something.
    -   Breaking changes stink but holding yourself back is worse.
    -   Words are better than special characters.
    -   Special characters are better than unclear words.

## Proposed Language Features
    -   First-class functions
    -   Four access modifiers
            -   private (class only)
            -   protected (class-hierarchy only)
            -   shared (namespace and class hierarchy)
            -   public (all)
    -   Constant (invariant) variables and methods (C++)
    -   Nullability decided at declaration time (Swift)
    -   Question-mark operator short-circuits at null values (Swift)
            -   e.g. object?getValue()?toString() returns null if object or
                object.getValue() or object.getValue().toString() are null.
    -   Scope-based memory management using reference counting
            -   i.e. when the variable with the last reference goes out of scope
                the memory is released (destructors called)
    -   Templates (not generics)

