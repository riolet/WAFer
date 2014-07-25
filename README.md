nope.c
=======

nope.c is a C language-based ultra-light software platform for scalable server-side and networking applications. Think node.js for C programmers.

Just put your code inside the function void server(int client, const char * queryString, const char * method) in server.c and you are good to go.

Default port is 4242. Set environment variable 'PORT' to change it.

That's really it. The source comes with a simple example that prints "Hello World" to get your started.

Acknowledgements:

1. [J. David Blackstone](http://sourceforge.net/u/jdavidb/profile/) and [Feng Shen](https://github.com/shenfeng), whose web servers have been repurposed to build this platform.

2. [Mark Karpeles](https://github.com/MagicalTux) for the incredible number of bug fixes!

