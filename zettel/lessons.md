Lessons
=======

  - Syncing Visual Studio environments is surprisingly difficult
  - Makefiles aren't as cross platform as you'd like
    - In windows, backslashes are escaped ... sometimes
	- Also, `*` wildcards don't seem to work
  - `cl.exe` isn't that friendly to command line users
    - How does `vcvarsall.bat` select what SDK to target?
	- And is there a way to specify that from the command line?
  - Refactoring will make you feel like you're spinning your wheels
  - Elegance has a debugging cost
    - It may be less expensive to use inelegant, verbose ways of doing
      something if that means you don't have to pause progress to test
      a more refined solution.
