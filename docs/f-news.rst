Changes in fishman-ctags
======================================================================

Many changes have been introduced in fishman-ctags. Here I (Masatake
YAMATO) enumerate the changes mainly in common part. About changes
not enumerated here especially in language parsers, inquire git-log.

Importing the most of all changes from exuberant-ctags
---------------------------------------------------------------------
See "exuberant-ctags" in "Tracking other projects" about the status of
importing. Some changes in Fedora and Debian are also imported.

New parsers
---------------------------------------------------------------------
Following parsers are added.

* ada
* css
* ctags option library *optlib*
* falcon
* go
* mib *optlib*
* rust *optlib*
* windres

See "Option library" about  *optlib*.

Heavily improved language parsers
---------------------------------------------------------------------
* php


New test facility named *Units*
---------------------------------------------------------------------
The existing test facility *Test* checks differences between the
output of older ctags and newer ctags. If a difference is found, it is
recognized as failed. *Test* expects older ctags works fine.

This expectation is not always met. Consider you add a new parser for
a language. You may want to add a sample source code for the language
to *Test*. Older ctags cannot make good tags file for the sample
code. Newer ctags can make it. In this point a difference is found;
and *Test* reports failure.

In other hand *Units* compares the expected output which a test
developer prepares and the output of ./ctags just built.  The expected
output doesn't depend on ctags. Many part of changes I made
is covered by *Units*. Fore more detail see "Using *Units*".

Running tests under valgrind
---------------------------------------------------------------------
With the following command line tests can be run under valgrind memory
checker.

::

	$ make -f testing.mak VG=1

The most of all memory leaks are fixed already.


Semi-fuzz testing
---------------------------------------------------------------------
One of the frustrating thing is that ctags enters infinite loop
against unexpected input. I wanted detect this kind of bug.

::

	$ make -f testing.mak fuzz FUZZ_LANGUAGE=LANG

With this command line, you can given all test inputs
including *Test/\** and *Units/\*/input.\** to a parser specified
with ``FUZZ_LANGUAGE1`` macro variable. You can get the list of things
which can be used as ``LANG1`` with following command line

::

	$ ./ctags --list-languages

You can run the target fuzz under ``VG=1``.

Automatic parser selection based on corpora
---------------------------------------------------------------------
For suffix *.m* ctags has two built-in parsers: ``ObjectiveC`` and
``Matlab``.  Which parser ctags should use? Here this situation is
called "parser confliction".

Like ``--language-force`` option ctags provides some ways to choose a
parser manually. However, it is nice if ctags choose a proper parser
without manual instruction.

With ``--<LANG>-corpus=spec:corpusFile`` option you can prepare corpus a
file to make ctags learn lexical tendency of a language. Ctags
learns it as typical input of ``LANG``. Based on the learning ctags
tries to solve the parser confliction. See *Data/optlib/mib.ctags*
and *Data/corpora/RFC1213-MIB.txt* as an example of the usage of
``--<LANG>-corpus``.

About ``ObjectiveC`` and ``Matlab`` parser, corpus files are embedded
in the parser implementations. See *objc.c* and *matlab.c*.

More documentation is needed.


Modeline based parser selection
---------------------------------------------------------------------
exuberant-ctags has ability to choose a proper parser based on shebang
line(e.g. *#!/bin/sh*). This feature is extended in fishman-ctags.

Editors like vim and emacs can recognize special patterns in editing
text file. Here it is called modeline. The pattern in inserted by a
user of text editor and represents a programming language of the text
file. With the recognition the editor chooses a mode (in emacs) or a
syntax (in vim). The editor can change its behavior specializing to a
programming language based on the mode (or syntax).

fishman-ctags can also recognize the modeline; and choose a language
parser based on the modeline.


ctags recognizes following patterns used in emacs:

  * at the head of input file or at the line next of shebang line::

      -*- mode: MODE; -*-

    or ::

      -*- MODE -*-

  * at the end of input file::

      Local Variables:
      ...
      mode: MODE
      ...
      End:


ctags recognizes following patterns used in vim:

  * at the end of input file::

      vim:set filetype=SYNTAX
    
    or ::

      ex:se ft=SYNTAX


Better parser selection for template files
---------------------------------------------------------------------
Consider an input file name *foo.c.in*.  Suffix *.in* is popular as a
name for template files.  Well-known one is *config.h.in* used in GNU
autotools.

ctags used suffix here *\*.in* for choosing a parser. *.in* shows
nothing about the language used in the input file. When fishman-ctags
finds *.in* as suffix, fishman-ctags checks the next suffix, here *.c*.

Option library
---------------------------------------------------------------------

exuberant-ctags provides the way to customize ctags with options like
``--langdef=<LANG>`` and ``--regex-<LANG>``. An option file where options are
written can be loaded with ``--options=OPTION_FILE``.

fishman-ctags extends this feature. fishman-ctags treats option files
as libraries. Developers of fishman-ctags maintain some option files
as part of fishman-ctags. They are shipped as part of fishman-ctags
release. With ``make install`` they are also installed as ctags command
is.

fishman-ctags prepares directories where the option files are installed.

Consider you use a GNU/Linux distribution.
Following directories are searched when loading an option file.

#. *~/.ctags.d/optlib*
#. */etc/ctags/optlib*
#. */usr/share/ctags/optlib*

The name of an option file must have .conf or .ctags as suffix.

If ctags is invoked with following command line::

	$ ctags --options=mib ...

Following files are searched with following order for finding ``mib``:

#.  *~/.ctags.d/optlib/mib.conf*
#.  *~/.ctags.d/optlib/mib.ctags*
#.  */etc/ctags/optlib/mib.conf*
#.  */etc/ctags/optlib/mib.ctags*
#.  */usr/share/ctags/optlib/mib.conf*
#.  */usr/share/ctags/optlib/mib.ctags*

These are called built-in search path.

If you don't want ctags not to refer above search path, instead you
want to specify directly an option file with ``--options``, start the
parameter of the option with */* (absolute path) or *./* (relative path)
like::

	$ ctags --option=/home/user/test/mib.cf
	$ ctags --option=./test/mib.cf

Here the restriction about suffix doesn't exist.

On GNU/Linux you can add more directories with environment variable
named ``CTAGS_DATA_PATH``.

::

	$ CTAGS_DATA_PATH=A:B ctags --options=mib ...

Following files are searched with following order for finding *mib*:

#. *A/optlib/mib.conf*
#. *A/optlib/mib.ctags*
#. *B/optlib/mib.conf*
#. *B/optlib/mib.ctags*
#. *~/.ctags.d/optlib/mib.conf*
#.  ...

Further more you can use ``--data-path=[+]PATH`` for adding more directories
with environment variable::

	$ CTAGS_DATA_PATH=A:B ctags --data-path=+C --options=mib ...

In this case following files are searched with following order for
finding *mib*:

#. *C/optlib/mib.conf*
#. *C/optlib/mib.ctags*
#. *A/optlib/mib.conf*
#. *A/optlib/mib.ctags*
#. *B/optlib/mib.conf*
#. *B/optlib/mib.ctags*
#. *~/.ctags.d/optlib/mib.conf*
#. ...

If you omit *+*, instead of adding you can set a directory::

	$ CTAGS_DATA_PATH=A:B ctags --data-path=C --options=mib ...

In this case following files are searched with following order for
finding mib:

#. *C/config/mib.conf*
#. *C/config/mib.ctags*

With reserved file name ``NONE``, you can make the directory list empty::

	$ CTAGS_DATA_PATH=A:B ctags --data-path=NONE --options=mib ...

In this case ctags tries to load *./mib*.

See also "Loading option recursively".

How a directory is set/added to the search path can be watched with
``--verbose`` option. This is useful for debugging this feature.

fishman-ctags developers wait your pull request of well written
option files.

NOTE: Though ``--data-path`` is highest priority, ``--data-path`` doesn't
affect a stage of automatic option file loading. Following files are
automatically loaded when ctags starts:

#. */ctags.cnf* (on MSDOS, MSWindows only)
#. */etc/ctags.conf*
#. */usr/local/etc/ctags.conf*
#. *$HOME/.ctags*
#. *$HOME /ctags.cnf* (on MSDOS, MSWindows only)
#. *.ctags*
#. *ctags.cnf* (on MSDOS, MSWindows only)

NOTE: This feature is still in experimental. The name of directories,
suffix rules, and other convention will change.

TODO

* Write about MSWindows more(*.cnf*).
* ``accept_only_dot_ctags()`` doesn't  check *.cnf*.

See "Contributing an optlib" if you have a good optlib.

Loading option recursively
---------------------------------------------------------------------

The option file loading rules explained in "Option library" is more
complex.  If you specify a directory as parameter for ``--option`` instead
of a file, fishman-ctags loads option files under the directory
recursively.

Consider following command line on GNU/Linux distribution::

	$ ctags --options=bundle ...

Following directories are searched first:

#. *~/.ctags.d/optlib/bundle.d*
#. */etc/ctags/optlib/bundle.d*
#. */usr/share/ctags/optlib/bundle.d*

If *bundle.d* is found and it is a directory, files (*\*.ctags*
and *\*.conf*), directories (\*.d) are loaded recursively.

NOTE, TODO: If *bundle.d* is not found above list, a file
*bundle.ctags* or *bundle.conf* are searched. This rule is a bit
ugly. Following search rules looks better.

#. *~/.ctags.d/optlib/bundle.d*
#. *~/.ctags.d/optlib/bundle.ctags*
#. *~/.ctags.d/optlib/bundle.conf*
#. */etc/ctags/optlib/bundle.d*
#. */etc/ctags/optlib/bundle.ctags*
#. */etc/ctags/optlib/bundle.conf*
#. */usr/share/ctags/optlib/bundle.d*
#. */usr/share/ctags/optlib/bundle.ctags*
#. */usr/share/ctags/optlib/bundle.conf*

NOTE: This feature requires ``scandir`` library function. This feature may
be disabled on which platform scandir is not available. Check ``option-directory``
in the supported features::

	$ ./ctags --list-features
	wildcards
	regex
	option-directory


Directories for preloading
---------------------------------------------------------------------

As written in "Option library", option libraries can be loaded with
``--options`` option. However, you may want to load them without
specifying it explicitly.

Following files can be used for the purpose. 

* ~/.ctags
* /ctags.cnf (on MSDOS, MSWindows only)
* /etc/ctags.conf
* /usr/local/etc/ctags.conf

This preloading feature comes from exuberant-ctags. However, I
think two weaknesses in this implementation.

* You have to edit the file when you want to add an
  option library to be loaded.

  If one wants to add or remove an ``--options=`` in a *ctags.conf*,
  currently one may have to use sed or something tool for adding or
  removing the line for the entry in */usr/local/etc/ctags.conf* (or
  */etc/ctags.conf*).

  I made a discussion about the similar issue in
  *http://marc.info/?t=129794755000003&r=1&w=2* about */etc/exports*
  of NFS.

* You cannot override the configuration defined in
  system administrator.

  A user must accept all configuration including ``--options=``
  in */etc/ctags.conf* and */usr/local/etc/ctags.conf*.

I prepare another facility. Let's not use older facility like *.ctags*.
I introduced following directories for preloading purpose.

#. *~/.ctags.d/preload*
#. */etc/ctags/preload*
#. */usr/share/ctags/preload*

All files and directories under the directories are loaded recursively,
but two restrictions:

* file/directory name

  The same suffix rules written in "Option library" and
  "Loading option recursively" are applied in preloading, too.

* overriding

  The traversing and loading are done in the order listed above.
  Once a file is loaded, another file with the same name is not loaded.
  Once a directory is traversed, another directory with the same name is
  not traversed.

  fishman-ctags prepares */usr/share/ctags/preload/default.ctags*.
  If you want ctags not to load it, make an empty file at
  *~/.ctags/default.ctags*. If you want to customize
  */usr/share/ctags/preload/default.ctags*, copy the file to
  *~/.ctags.d/default.ctags* and edit it as you want.

  Assume */usr/share/ctags/preload/something.d* exits.
  Some *.ctags* files are in the directory. With making
  an empty directory at *~/.ctags.d/something.d*, you
  can make ctags not to traverse */usr/share/ctags/preload/something.d*.
  As the result *.ctags* files under */usr/share/ctags/preload/something.d*
  are not loaded.

  If you want to customize one of file under
  */usr/share/ctags/preload/something.d*, you have to copy
  */usr/share/ctags/preload/something.d* to *~/.ctags.d/somethind.d* recursively.
  You can also use symbolic links. After copying or symbolic linking, edit
  one of the copied file.

This feature is heavily inspired by systemd.


Long regex flag
---------------------------------------------------------------------

I am thinking about making regex parser more useful by adding
more kind of flags to ``--regex-<LANG>`` expression. As explained in
*ctags.1* man page, ``b``, ``e`` and ``i`` are defined as flags in
exuberant-ctags.

Even if I add more flags like ``x``, ``y``, ``z``,..., I guess users
including I myself may not utilize them well. It is difficult for them
to memorize. In addition If many "option libraries" are contributed,
we have to maintain them.

For both users and developers the variety short flags are just
nightmares.

So fishman-ctags prepares API for defining long flags, which can be
used as aliases for short flags. The long flags requires more typing
but more readable.

Here is the mapping between short flag names and long flag names.

=========== =========== 
short flag  long flag
=========== ===========
b           basic
e           extend
i           icase
=========== ===========

Long flags can be specified with surrounding ``{`` and ``}``.
So the following ``--regex-<LANG>`` expression ::

   --m4-regex=/^m4_define\(\[([^]$\(]+).+$/\1/d,definition/x

is the same as ::

   --m4-regex=/^m4_define\(\[([^]$\(]+).+$/\1/d,definition/{extend}

The characters ``{`` and ``}`` are not suitable in using command line.  My
intent is that using long flags in option libraries.


Exclusive flag in regex
---------------------------------------------------------------------

A line read from input files was matched with **all** regular expressions
defined with ``--regex-<LANG>`` (or ``--<LANG>-regex``). Each regular
expression matched successfully emits a tag.

In some cases another policy, exclusive-matching, is preferable to the
all-matching policy. Exclusive-matching means the rest of regular
expressions are not tried if one of regular expressions is matched
successfully,

For specifying exclusive-matching I introduced a flag ``exclusive``
(long) and ``x`` (short). It is used in *data/optlib/mib.ctags*::


	--mib-regex=/^([^ \t]+)[ \t]+DEFINITIONS ::= BEGIN/\1/d,definitions/{exclusive}
	--mib-regex=/^([a-zA-Z][^ \t]+)[ \t]+[A-Za-z]/\1/n,name/


passing parameter for long regex flag
---------------------------------------------------------------------

TBW


External tag engine
---------------------------------------------------------------------

TBW