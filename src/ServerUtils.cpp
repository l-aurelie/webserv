#include "Parser.hpp"
#include "Request.hpp"
#include "ServerUtils.hpp"
#include "Utils.hpp"
#include "webserv.hpp"

#include <iostream>
#include <sstream>

namespace ServerUtils {

static void writeBodyInTMPFile(Request & req, char *buf, int read)
{
	if (req.countClientMaxBodySize > 0 && read > req.countClientMaxBodySize && req.countClientMaxBodySize <= req.countContentLength)//depasse le client_max_body_size
	{
		req.statusCode = TOO_LARGE;
		req.countContentLength = 0;
	}
	else if (read > req.countContentLength)//depasse contentlength
	{
		req.tmpFile.write(buf, req.countContentLength);
		req.tmpFile.flush();
		req.countContentLength = 0;
	}
	else
	{
		req.tmpFile.write(buf, read);
		req.tmpFile.flush();
		req.countContentLength -= read;
		req.countClientMaxBodySize -= read;
	}
}

static void unchunk(char *& body_buf, int & read, std::size_t & pos, Request & req)
{
	while (pos != static_cast<std::size_t>(read))	// tant que pas fin du buf
	{
		//-- Parse le hexa content length
		if (req.countContentLength == 0)
		{
			std::stringstream ss;
			std::string hex(body_buf + pos);
			hex = hex.substr(0, hex.find("\r\n"));
			pos += hex.length(); //avance pos apres le hexa\r\n
			ss << std::hex << hex;
			ss >> req.countContentLength;
			if (ss.fail())
			{
				std::cerr << "error : unchunk request: cannot convert content length from hexa" << std::endl;
				req.statusCode = BAD_REQUEST;
				return ;
			}
			//-- Ajoute la taille du chunk au content length
			req.contentLength += req.countContentLength;
			
			if (req.countContentLength == 0)// Fin de request
				req.countContentLength = -1;
		}
		
		//-- Saute \r\n  
		if (static_cast<int>(pos) < read && req.countContentLength)//Apres hexa
		{
			if (std::string(body_buf + pos).find("\r") == 0)
				pos += 1;
			if (static_cast<int>(pos) < read && std::string(body_buf + pos).find("\n") == 0)
				pos += 1;
		}
		if (static_cast<int>(pos) < read && req.countContentLength == -1)//a la fi n de la requete
		{
			if (std::string(body_buf + pos).find("\r") == 0)
				pos += 1;
			if (static_cast<int>(pos) < read && std::string(body_buf + pos).find("\n") == 0)
				break ;
		}
		if (req.countClientMaxBodySize > 0 && req.countContentLength >= req.countClientMaxBodySize) // si somme de countContentLength a atteint ClientMaxBodySize
		{
			req.statusCode = TOO_LARGE;
			break ;
		}
		else if (req.countContentLength > 0 && req.countContentLength < read - static_cast<int>(pos))//read trop grand on ecrit juste content length on a fini ce chunk content length = 0
		{
			req.tmpFile.write(body_buf + pos, req.countContentLength);
			req.tmpFile.flush();
			pos += req.countContentLength;
			if (static_cast<int>(pos) + 2 < read) // skip \r\n of end of text line
				pos += 2;
			if (req.countClientMaxBodySize > 0)
				req.countClientMaxBodySize -= req.countContentLength; // ContentLength always > ClientMaxBodySize, because checked at the above if condition
			req.countContentLength = 0;
		}
		else // Notre message est separe car notre buffer est trop petit
		{
			req.tmpFile.write(body_buf + pos, read - pos);
			req.tmpFile.flush();
			req.countContentLength -= read - pos;
			if (req.countClientMaxBodySize > 0)
				req.countClientMaxBodySize -= read - pos;
			pos = read; // stopper la boucle while
		}
	}
}

void parseRecv(int bytes_read, char * buf, Request & req, std::vector< Conf > & confs) 
{
	//-- Cherche la fin du header
	std::string tmp = buf;
	std::size_t pos = std::string::npos;
	if (!req.headerFilled)
		pos = tmp.find("\n\r\n") + 1;
	if (pos == std::string::npos && !req.getChunked()) // cas non chunked
	{
		//- Soit ajoute buf a la string headerBuf si header non complet
		if (!req.headerFilled)
			req.headerBuf += tmp;
		//- Soit ajoute buf au body tmpFile
		else
			ServerUtils::writeBodyInTMPFile(req, buf, bytes_read);
	}
	else //-- curseur sur la ligne de fin du header OU au debut de chunked
	{
		if (pos != std::string::npos) //- Curseur a la ligne de fin du header
		{
			//- Rempli la fin du header
			req.headerFilled = true;
			req.headerBuf += tmp.substr(0, pos);
			//- Le header est complet, on parse le requete
			req = Parser::parseRequest(req);
			req.countClientMaxBodySize = Utils::selectConf(confs, req.getServerName(), req.getPath()).clientMaxBodySize;//trouver max_body_size
			pos += 2; //- sauter la ligne vide separatrice entre les header et le body
			bytes_read -= pos; //- bytes_read: le nb de characters du body dans le buf
		}
		else //- debut de nouveau msg Chunked
			pos = 0;
		char * body_buf = &(buf[pos]); //debut du body jusqu a fin de buf
		pos = 0; // debut du body_buf
		if (!req.getChunked()) //- Non chunked et fin header : ecrit la partie body dans fichier
		{
			req.countContentLength = req.getContentLength();
			ServerUtils::writeBodyInTMPFile(req, body_buf, bytes_read);
		}
		else if (req.getChunked())
			ServerUtils::unchunk(body_buf, bytes_read, pos, req);
	}
}

}
