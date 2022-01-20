# webserv

This is when you finally understand why a URL starts with HTTP.

## Intro

Fonction principale serveur Web : stocker, traiter et livrer des pages Web aux clients. Communication a l'aide protocole HTTP.
Le client (ex : navigateur web) demande une ressource(requete); Le serveur repond par le contenu (souvent fichier reel, contenu sur le stockage du server), ou par un message d'erreur.

## Fonctions autorisees

Tout en C++ 98. malloc, free, write, htons, htonl, ntohs, ntohl, select, poll, epoll (epoll_create, epoll_ctl, epoll_wait), kqueue (kqueue, kevent), socket, accept, listen, send, recv, bind, connect, inet_addr, setsockopt, getsockname, fcntl.

## Roadmap

- [ ] Web server
	- [ ] Listen for client
	- [ ] Non blocking
	- [ ] Use poll or equivalent to control IO between client and server
	- [ ] Use only 1 poll or equivalent
	- [ ] Request must never hang forever
	- [ ] Default error pages
	- [ ] Serve fully static website
	- [ ] Let client upload files
	- [ ] Accurate response status
	- [ ] Method GET
	- [ ] Method POST
	- [ ] Method DELETE
	- [ ] Ability to listen on multiple ports
	- [ ] Take config file as arg
	- [ ] Set default path for config file
- [ ] Config file
	- [ ] provide file to demonstrate every feature
	- [ ] choose port and host for each "server"
	- [ ] ability to setup server_name or not
	- [ ] The first server for a host:port will be the default for this host:port (meaning it will answer to all request that doesn’t belong to an other server)
	- [ ] Default error pages
	- [ ] limit client body size
	- [ ] accept uploaded files
	- [ ] configure where uploaded files should be saved
	- [ ] Setup routes one or multiple config
		- [ ] list of accepted HTTP methods
		- [ ] HTTP redirection
		- [ ] 'root' like in nginx conf
		- [ ] directory listing ('autoindex' in nginx conf)
		- [ ] 'index' like in nginx conf
		- [ ] Execute CGI
			- [ ] use full path as PATH_INFO
			- [ ] unchunk request
			- [ ] expect EOF as end of body
			- [ ] call the cgi with the file requested as first arg
			- [ ] run in correct directory for relative path



- [ ] Leaks
