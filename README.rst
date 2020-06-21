tsup - Tiny supervisor
======================
`tsup` is a tiny process supervisor that allows users to control processes
based on their user defined labels instead of OS assigned PIDs.


Requeirements
-------------
`tsup` is written in c and depends on ``libevent`` library for event handling.

Installation
------------

Verify that compilation flags and installation paths defined in ``config.mk``
match your expectations. Then compile `tsup` code with

::

    make

and install it with

::

    make install

Usage
-----

`tsup` package comes with 2 binaries: tsupd and tsupq.
tsupd is a supervisor daemon itself. It creates and kills processes on demand.
tsupq is a client application used to send requests to the daemon.

Therefore, to use `tsup` first you need to start the daemon:

::

    $ tsupd

By default it will run in the foreground, so you will need additional setup if
you want to run it in the background. You can ask tsupd to create a new process
for you by using tsupq:

::

    $ tsupq spawn LABEL sleep 100

this will make tsupd to create a new process ``sleep 100`` and assign it label
*LABEL*. A list of currently supervised processes by tsupd can be obtained with

::

    $ tsupq list
    ID: "LABEL", PID: 14917, CMD: "'sleep' '100'"

Finally, to kill a process with label *LABEL* you can run

::

    $ tsupq kill LABEL

Please refer to the manpages of tsupq and tsupd for the further information.

