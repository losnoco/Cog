JNetLib Readme.txt
/*
**
**  Justin Frankel
**  www.cockos.com
**
**  Joshua Teitelbaum
**  www.cryptomail.org
*/

Table of Contents:

I)    License
II)   Introduction
III)  Features
IV)   SSL Integration
V)    Postamble

I)  License

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
    claim that you wrote the original software. If you use this software
    in a product, an acknowledgment in the product documentation would be
    appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
    misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution
  
II)  Introduction

Welcome to the JNetLib library, a C++ asynchronous network abstraction layer.

III) Features

* Works under Linux, FreeBSD, Win32, and other operating systems.
* TCP connections support
* Listening sockets support
* Asynchronous DNS support
* HTTP serving and getting support (including basic authentication, GET/POST and HTTP/1.1 support)
* Basic HTTPS serving, getting support (including basic authentication, and HTTP/1.1 support)
* Completely asynchronous love for single threaded apps.
* Coming soon: support for UDP, as well as serial i/o. 

IV)  SSL Integration

JNetLib now employs the OpenSSL encryption library from www.openssl.org.
However, it is not compiled in as the default.
If you would like to have SSL support compiled in you have to:
1)  uncomment the #define _JNETLIB_SSL_ 1 in netinc.h
2)  Obtain the openssl requisite libraries libeay32.lib and ssleay32.lib
2a) You can obtain these libraries either by building them yourself via aquring the source yourself or...
2b) Download the SSL quickpack for windows, and ensure that you have the SSL quickpack in your build environment
You will need the proper include and library paths.
If you downloaded the quickpack to C:\, then the appropriate build environment include directory would be "c:\openssl\include", and so forth.

Disclaimer:
SSL functionality is new, and not entirely feature complete.
The current certification authentication tactic is to allow all certificates through, without checking any CA signatures.


V) Postamble
Questions/Comments
joshuat@cryptomail.org

 



