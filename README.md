[![Build Status](https://travis-ci.org/riolet/wafer.c.svg?branch=master)](https://travis-ci.org/riolet/wafer.c)

#WAFer

WAFer is a C language-based ultra-light scalable server-side web applications framework. Think node.js for C programmers. Because it's written in C for the C eco system, WAFer is wafer-thins with a memory footprint that is only a fraction of that of node.js and other bulky frameworks.

Just copy server.c (say, as myserver.c), put your code inside the function `void server(Request request)` in `myserver.c` and, make with `make SERVER=myserver`, and you are good to go.

WAFer can operate in many different configurations, all selected at compile time. They include:


1. Single-threaded (Default) or multi-threaded (make with `THREADS=n` where n>0)

2. Select(Default) or epoll (make with `LOOP=epoll`) based event loop

3. C10K mode (make with `LOOP=epoll MAX_CON_CONS=n` where n>10,000)


Default port is 4242. Set environment variable 'PORT' to change it.

That's really it. The source comes with a simple example `example.c` to get you started.

###Note to Contributors

Thank you for making this a wonderful project!

Here's our preferred formatting style:

  `find . \( -name '*.c' -o -name '*.h' \) -exec indent --no-tabs  --linux-style --line-length 90 --indent-level 4 -bli0 \{\} \;`
  

###Acknowledgements

1. [J. David Blackstone](http://sourceforge.net/u/jdavidb/profile/) and [Feng Shen](https://github.com/shenfeng), whose web servers have been repurposed to build this platform.

2. [Mark Karpeles](https://github.com/MagicalTux) for the incredible number of bug fixes!

3. [Fine folks at /r/programming](http://www.reddit.com/r/programming/) for the honest and constructive feedback.

