[![Build Status](https://travis-ci.org/riolet/nope.c.svg?branch=master)](https://travis-ci.org/riolet/nope.c)

#nope.c

nope.c is a C language-based ultra-light software platform for scalable server-side and networking applications. Think node.js for C programmers.

Just put your code inside the function `void server(Request request)` in `server.c` and you are good to go.

nope.c can operate in many different configurations, all selected at compile time. They include:
1. Single-threaded (Default) or multi-threaded (make with `THREADS=n` where n>0)
2. Select(Default) or epoll (make with `LOOP=epoll`) based event loop
3. C10K mode (make with `LOOP=epoll MAX_CON_CONS=n` where n>10,000)

Default port is 4242. Set environment variable 'PORT' to change it.

That's really it. The source comes with a simple example `factor.c` to get you started.

###Note to Contributors

Thank you for making this a wonderful project!

Here's our preferred formatting style:

  `find . \( -name '*.c' -o -name '*.h' \) -exec indent --no-tabs  --linux-style --line-length 90 --indent-level 4 -bli0 \{\} \;`
  

###Acknowledgements

1. [J. David Blackstone](http://sourceforge.net/u/jdavidb/profile/) and [Feng Shen](https://github.com/shenfeng), whose web servers have been repurposed to build this platform.

2. [Mark Karpeles](https://github.com/MagicalTux) for the incredible number of bug fixes!

3. [Fine folks at /r/programming](http://www.reddit.com/r/programming/) for the honest and constructive feedback.

