/*
** JNetLib
** Copyright (C) Joshua Teitelbaum, sergent first class, 1014 army.
** Author: Joshua Teitelbaum
** File: httppost.cpp
** License: see jnetlib.h
*/

#ifdef _WIN32
#include <windows.h>
#include <malloc.h>
#endif
#include "netinc.h"
#include "util.h"
#include "httppost.h"

#include <malloc/malloc.h>
#include <sys/stat.h>

static char *PREFIX = "--";
static char *NEWLINE = "\r\n";

/*
**  Joshua Teitelbaum 1/15/2006
**  1014 Army, Sergent First Class
**  HTTP Post implementation employing HTTPGet as base class.
*/
/*
**  _endstack
**  when building the POST stack for a FORM, need a postamble
*/
void JNL_HTTPPost::_endstack()
{
	std::string cd;
	cd = PREFIX;
	cd += m_boundary;
	cd += PREFIX;
	cd += NEWLINE;

	FILEStackObject fso;
	fso.m_otype = 2;
	fso.m_str = cd;
	m_filestack.push_back(fso);

}
/*
**  prepares one field and puts it on the stack
*/
void JNL_HTTPPost::_preparefield(std::string &name)
{
	std::string cd;

	cd = PREFIX;
	cd += m_boundary;
	cd +=  NEWLINE;
    cd += "content-disposition: form-data; name=\"";
	cd += name;
	cd += "\"";
	cd += NEWLINE;
	cd += NEWLINE;
	cd += m_fields[name];
	cd += "\r\n";

	FILEStackObject fso;
	
	fso.m_otype = 3;
	fso.m_str = cd;
	m_filestack.push_back(fso);
	
}
/*
**  prepare all the fields, and push them on the stack
*/
void JNL_HTTPPost::_preparefieldstack()
{
	std::map<std::string, std::string>::iterator it;

	for(it = m_fields.begin(); it != m_fields.end(); it++)
	{
		std::string str = (*it).first;
		_preparefield(str);
	}
}
/*
**  prepare files and put them on the stack
*/
void JNL_HTTPPost::_preparefilestack()
{
	std::map<std::string, std::string>::iterator it;

	for(it = m_paths.begin(); it != m_paths.end(); it++)
	{
		/*
		--boundary\r\n
		Content-Disposition: form-data; name="<fieldName>"; filename="<filename>"\r\n
		Content-Type: <mime-type>\r\n
		\r\n
		<file-data>\r\n
		*/
			
		std::string cd ;

		cd = PREFIX;
		cd += m_boundary;
		cd +=  NEWLINE;
		cd += "content-disposition: form-data; name=\"";
		cd += (*it).first;
		cd += "\"; filename=\"";
		cd += m_filenames[(*it).first];
		cd += "\"";
		cd += NEWLINE;
		cd += "content-type: ";
		cd += m_content_type[(*it).first];
		cd += NEWLINE;
		cd += NEWLINE;


		FILEStackObject fso;

		fso.m_otype = 0;
		fso.m_str = cd;
		m_filestack.push_back(fso);

		fso.m_otype = 1;
		fso.m_str = (*it).first;
		m_filestack.push_back(fso);

		cd = NEWLINE;

		fso.m_otype = 2;
		fso.m_str = cd;
		m_filestack.push_back(fso);
	}
	
}
/*
**  payload function, run.
**  the tactic is this:
**  if we can't write, do normal run.
**  
**  while there is stuff on the POST stack
**		pop and item
**		write to the buffers as much as we can
**      if we can't write more, stop
**  end while
**  do normal run
*/
int JNL_HTTPPost::run()
{
	bool stop = 0;
	std::map<std::string, std::string>::iterator it;
	int ntowrite;
	int retval;
	do
	{
		if(m_con->send_bytes_available() <= 0)
		{
			/*
			**  rut roh, no buffa
			*/
			break;
		}

		if(m_filestack.size() <= 0)
		{
			/*
			**  Nothing to do
			*/
			break;
		}
		/*
		**  Get the current object off the stack
		*/
		switch(m_filestack.front().m_otype)
		{
			case 0: /*preamble*/
			case 2: /*postamble*/
			case 3: /*plain jane field*/
			{
				ntowrite = m_filestack.front().m_str.length() - m_filestack.front().nwritten;
				
				if(m_con->send_bytes_available() < ntowrite)
				{
					ntowrite = m_con->send_bytes_available();
				}
				if(ntowrite>0)
				{
					retval = m_con->send(m_filestack.front().m_str.c_str() + m_filestack.front().nwritten,ntowrite);
					
					if(retval < 0)
					{
						break;
					}

					m_filestack.front().nwritten += ntowrite;
					m_nwritten += ntowrite;

					if( m_filestack.front().nwritten == m_filestack.front().m_str.length())
					{
						/*
						**  all done with this object
						*/
						m_filestack.pop_front();
					}
				}
				else
				{
					retval = 0;
					stop = true;
				}
			}
			break;
			case 1: /*MEAT*/
			{
				FILE *fp;
				int sresult;
				fp = fopen(m_paths[m_filestack.front().m_str].c_str(),"rb");

				if(fp == NULL)
				{
					/*
					**  someone gave this process a hot foot?
					*/
					return -1;
				}
				sresult = fseek(fp,m_filestack.front().nwritten,SEEK_SET);
				
				if(sresult < 0)
				{
					/*
					**  someone gave this process a hot foot?
					*/
					fclose(fp);
					return -1;
				}
				
				if(((m_content_length[m_filestack.front().m_str] - m_filestack.front().nwritten)) > (unsigned long)m_con->send_bytes_available())
				{
					ntowrite = m_con->send_bytes_available();
				}
				else
				{
					ntowrite = (m_content_length[m_filestack.front().m_str] - m_filestack.front().nwritten);
				}

				char *b=NULL;

				b = (char *)malloc(ntowrite);

				if(b == NULL)
				{
					return -1;
					/*
					**  whiskey tango foxtrot
					*/
				}
				/*
				** read from where we left off
				*/
				sresult = fread(b,1,ntowrite,fp);

				if(sresult != ntowrite)
				{
					/*
					**  hot foot with matches again
					*/
					if(b)
					{
						free(b);
					}
					fclose(fp);
					return -1;
				}

				fclose(fp);

				retval = m_con->send(b,ntowrite);

				free(b);

				if(retval < 0)
				{
					return retval;
				}

				m_filestack.front().nwritten += ntowrite;

				m_nwritten += ntowrite;

				if(m_filestack.front().nwritten == m_content_length[m_filestack.front().m_str])
				{
					m_filestack.pop_front();
				}
			}
			break;
		}
	} while(!stop);

	return JNL_HTTPGet::run();
}
/*
**  addfield, adds field
*/
int JNL_HTTPPost::addfield(char *name, char *val)
{
	if(name == NULL || val == NULL)
	{
		return -1;
	}
	m_fields[name]=val;
	
	return 0;
}
/*
**  addfile, adds file
*/
int JNL_HTTPPost::addfile(char *name, char *mimetype, char *filename, char *filepath)
{
	struct stat buffer;

	FILE *fp=NULL;
	if(name==NULL || filepath == NULL || mimetype == NULL || filename == NULL || (!(fp=fopen(filepath,"rb"))))
	{
		return -1;
	}

	m_content_type[name] = mimetype;
	m_paths[name] = filepath;
	m_filenames[name] = filename;
	
	fstat(fileno(fp),&buffer);
	fclose(fp);
	m_content_length[name] = buffer.st_size;

	return 0;
}
/*
**  After adding fields and files, get the content length
*/
int JNL_HTTPPost::contentlength()
{
	std::map<std::string,std::string>::iterator it;
	std::string tmp;
	int length = 0;
// fields
	{
		for (it = m_fields.begin(); it != m_fields.end(); it++)
		{
			std::string name = (*it).first;
			std::string value = (*it).second;

			tmp = "--" + m_boundary + "\r\n"
				"content-disposition: form-data; name=\"" + name + "\"\r\n"
				"\r\n";
			tmp += value + "\r\n";
			
			length += tmp.size();
		}
	}

	// files
	{
		for (it = m_filenames.begin(); it != m_filenames.end(); it++)
		{
			std::string name = (*it).first;
			std::string filename = (*it).second;
			long content_length = m_content_length[name];
			std::string content_type = m_content_type[name];
			tmp = "--" + m_boundary + "\r\n"
				"content-disposition: form-data; name=\"" + name + "\"; filename=\"" + filename + "\"\r\n"
				"content-type: " + content_type + "\r\n"
				"\r\n";
			length += tmp.size();
			length += content_length;
			length += 2; // crlf after file
		}
	}

	// end
	tmp = "--" + m_boundary + "--\r\n";
	length += tmp.size();

	return length;

}
void JNL_HTTPPost::_addformheader()
{
	char *h;
#ifdef _WIN32
	srand(GetTickCount());
#else
	srand(time(NULL));
#endif

	int x;
	char *preamble = "content-type:multipart/form-data; boundary=";
	char *dashes = "----";

	char *post_boundary = (char*)alloca(10+strlen(dashes));
	memcpy(post_boundary,dashes,strlen(dashes));
	
	for(x=strlen(dashes); x < (int)strlen(dashes)+9; x++)
	{
		post_boundary[x] = (char)(rand() % 9) + '0';
	}
	post_boundary[x] = 0;

	h = (char *)alloca(strlen(preamble) + strlen(post_boundary) + 1);
	sprintf(h,"%s%s",preamble,post_boundary);
	addheader(h);
	m_boundary = post_boundary;
}
