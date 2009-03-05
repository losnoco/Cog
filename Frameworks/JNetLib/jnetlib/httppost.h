
/*
** JNetLib
** Copyright (C) Joshua Teitelbaum, sergent first class, 1014 army.
** Author: Joshua Teitelbaum
** File: httppost.cpp/h
** License: see jnetlib.h
** Usage:
**   1. Create a JNL_HTTPPost object, optionally specifying a JNL_AsyncDNS
**      object to use (or NULL for none, or JNL_CONNECTION_AUTODNS for auto),
**      and the receive buffer size, and a string specifying proxy (or NULL 
**      for none). See note on proxy string below.
**   2. call addheader() to add whatever headers you want. It is recommended to
**      add at least the following two:
**        addheader("User-Agent:MyApp (Mozilla)");
*///      addheader("Accept:*/*");
/*  Joshua Teitelbaum 1/15/2006
**   2.5 call addfield to add field items to your POST
**   2.9 call addfile to add file items to your POST
*/
/*         ( the comment weirdness is there so I Can do the star-slash :)
**   3. Call connect() with the URL you wish to Post (see URL string note below)
**   4. Call run() once in a while, checking to see if it returns -1 
**      (if it does return -1, call geterrorstr() to see what the error is).
**      (if it returns 1, no big deal, the connection has closed).
**   5. While you're at it, you can call bytes_available() to see if any data
**      from the http stream is available, or getheader() to see if any headers
**      are available, or getreply() to see the HTTP reply, or getallheaders() 
**      to get a double null terminated, null delimited list of headers returned.
**   6. If you want to read from the stream, call get_bytes (which returns how much
**      was actually read).
**   7. content_length() is a helper function that uses getheader() to check the
**      content-length header.
**   8. Delete ye' ol' object when done.
**
** Proxy String:
**   should be in the format of host:port, or user@host:port, or 
**   user:password@host:port. if port is not specified, 80 is assumed.
** URL String:
**   should be in the format of http://user:pass@host:port/requestwhatever
**   note that user, pass, port, and /requestwhatever are all optional :)
**   note that also, http:// is really not important. if you do poo://
**   or even leave out the http:// altogether, it will still work.
*/

#ifndef _HTTPPOST_H_
#define _HTTPPOST_H_

#include "connection.h"
#include "httpget.h"
/*
**  VC6 SPEW
*/
#pragma warning(disable:4786)
/*
**  END VC6 SPEW
*/
#include <string>
#include <list>
#include <map>

class FILEStackObject
{
public:
	int m_otype; /* 0 = PREAMBLE, 1 = DO THE FILE , 2 = POSTAMBLE, 3 = FIELD */
	std::string m_str;
	unsigned long nwritten;
public:
	FILEStackObject()
	{
		m_otype = 0;
		m_str = "";
		nwritten = 0;
	}

};
class JNL_HTTPPost : public JNL_HTTPGet
{
  public:
	int run(void);
    void connect(const char *url, int ver=1, char *requestmethod="POST")
	{
		m_nwritten = 0;

		_addformheader();

		_preparefieldstack();
		_preparefilestack();

		_addcontentlength();

		_endstack();

		JNL_HTTPGet::connect(url,ver,requestmethod);
	}

	int addfield(char *name, char *val);
	int addfile(char *name, char *mimetype, char *filename, char *filepath);
	int contentlength();
	unsigned long written(){return m_nwritten;};
	
protected:
	
	void _preparefield(std::string &str);
	void _preparefieldstack();
	void _preparefilestack();
	void _endstack();
	void _addformheader();
	void _addcontentlength()
	{
		char sz[1024];
		sprintf(sz,"Content-length:%d",contentlength());
		addheader(sz);
	}

	unsigned long m_nwritten;
	std::string m_boundary;
	std::string m_strcurrentfileformname;

	std::map<std::string /*name*/ ,std::string /*value*/> m_fields;
	std::map<std::string /*name*/ ,std::string /*content type*/> m_content_type;
	std::map<std::string /*name*/ ,std::string /*path*/> m_paths;
	std::map<std::string /*name*/ ,std::string /*filename*/> m_filenames;
	std::map<std::string /*name */,unsigned long /*size*/> m_content_length;

	std::list<FILEStackObject> m_filestack;
};

#endif // _HTTPPOST_H_
