This is fork of fishman-ctags with better php support. This means php
parser in this fork is better for some developers. What it can:

- better matching for class definitions - you will jump on User class
  definition earlier than for variable named $User.

- better jumping - for each nested var/definition this parser
  generates two tags:

  `tagname`

  and

  `tagname<Parent>`

It dramatically helps when you using find-tag in emacs or so.

For example you have class member called "name", and lots of other
classes in your project has same member. So to find tag you type:

`name<Class`

And jump to needed class. Try it, you'll love it. Especially on
monster projects on monster Symfony.

Of course such strategy leads to bigger TAGS files, but comfort worth
it.

There how it looks for end-user:

![alt tag](https://raw.githubusercontent.com/zargener/ctags-better-php/master/demo.gif)

=====================================================================

fishman-ctags is a project forked from exuberant-ctags. The project is
hosted at github.

The purpose of the project is preparing and maintaining common/unified
working space where people interested in making ctags better can work
together.

pull-requests are welcome!
