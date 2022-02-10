# webserv

This is when you finally understand why a URL starts with HTTP.

## Intro

Fonction principale serveur Web : stocker, traiter et livrer des pages Web aux clients. Communication a l'aide protocole HTTP.
Le client (ex : navigateur web) demande une ressource(requete); Le serveur repond par le contenu (souvent fichier reel, contenu sur le stockage du server), ou par un message d'erreur.

## Fonctions autorisees

Tout en C++ 98. malloc, free, write, htons, htonl, ntohs, ntohl, select, poll, epoll (epoll_create, epoll_ctl, epoll_wait), kqueue (kqueue, kevent), socket, accept, listen, send, recv, bind, connect, inet_addr, setsockopt, getsockname, fcntl.

## Roadmap

- [x] Creer la structure du projet
- [ ] server en c++
	- [x] envoyer message
	- [x] recevoir message
	- [x] passer en mode non bloquant
	- [x] utiliser poll ou equivalent
	- [x] close les clients qui se deconnecte
	- [x] alloue notre tableau
	- [x] realloue notre tableau si le nombre de client max est atteind
	- [x] repondre aux messages des clients
	- [x] utiliser les fichiers de conf
	- [x] parse content-length
	- [ ] revoir la maniere dont le client se deconnecte (deconnexion entre 2 requetes a partir du navigateur)
	- [ ] deconnecter le client si read/recv return 0 OR -1
- [ ] Parsing des fichiers de conf
	- [x] Prendre chemin en argument
	- [x] Chemin par defaut
	- [x] Recuperer le port
	- [x] Serveur utilise le port du fichier de conf
	- [x] Recuperer le host
	- [x] Serveur utilise le host du fichier de conf
	- [x] Decouper le fichier de configuration dans des objets Conf
	- [x] Checker mauvaises infos du fichier de config
	- [x] client max body size
	- [x] location block
	- [ ] Gerer clientbodysize taille (m, g, k)
	- [ ] possiblity de changer les pages d'error
	- [ ] faire des HTTP redirection 301 + header Location: URL
	- [ ] autoindex on/off
	- [ ] allow_method
	- [ ] devrait pas fonctionner si plusieurs fois le meme port utilise
- [ ] parsing des requetes clients
	- [x] analyse messages requetes
	- [x] create d'objet Request
	- [x] tronquer le body a max_client_body_size
	- [ ] gerer les header field-values multiligne
- [ ] creation des reponses du serveur
	- [x] header
	- [x] body => aller chercher le fichier correspondant
	- [x] envoyer au client
	- [x] DELETE
	- [x] adapter les status de reponse
	- [x] adapter les reponses en fonction de la Conf
	- [ ] default error page
	- [ ] POST avec requete = POST /index.html HTTP/1.1
	- [ ] send image
	- [ ] utiliser autoindex
	- [ ] upload fichier
	- [ ] download fichier
- [ ] CGI
	- [x] C'est quoi ?
	- [x] Faire requete GET dans le terminal
	- [X] Faire requete POST dans le terminal
	- [X] Faire fonctionner GET dans webserv
	- [x] Faire fonctionner POST dans webserv
	- [x] Faire fonctionner CGI dynamiquement dans webserv
		- [x] POST body
		- [x] POST Header (Content-type est duplique, a parser dans le body retourne par php-cgi ?)
		- [x] GET body
		- [x] GET Header
	- [x] gestion d'erreur dans launchCGI
	- [ ] unchunck request
- [ ] clean
	- [ ] deplacer dans utils

- [ ] proteger les fichiers elementaires

## Fix

- [ ] Notre server execute la requete des la premiere ligne envoyee a nc
- [ ] impossible de supprimer un dossier
- [ ] 409 Conflict when rm a folder that contains a file without perms
- [ ] how to test DELETE on browser ?

## Doc

- [socket](https://www.youtube.com/watch?v=s3o5tixMFho)
- [select/poll/epoll](https://www.youtube.com/watch?v=dEHZb9JsmOU)
- [webserv](https://webserv42.notion.site/webserv42/Webserv-cbb6ab4136ba4b4c8cb4f98109d5fc1f)

### CGI

[https://www.tutorialspoint.com/cplusplus/cpp_web_programming.htm](https://www.tutorialspoint.com/cplusplus/cpp_web_programming.htm)
[http://www.wijata.com/cgi/cgispec.html](http://www.wijata.com/cgi/cgispec.html#4.0)
[https://www.ibm.com/docs/ko/netcoolomnibus/8.1?topic=scripts-environment-variables-in-cgi-script](https://www.ibm.com/docs/ko/netcoolomnibus/8.1?topic=scripts-environment-variables-in-cgi-script)
[http://www.cgi101.com/book/ch3/text.html](http://www.cgi101.com/book/ch3/text.html)
https://fr.wikipedia.org/wiki/Variables_d%27environnement_CGI

## Subject

- [ ] Web server
	- [x] Listen for client
	- [x] Listen for clients
	- [x] Non blocking
	- [x] Use poll or equivalent to control IO between client and server
	- [x] Use only 1 poll or equivalent
	- [x] Request must never hang forever
	- [ ] Default error pages
	- [ ] Serve fully static website
	- [ ] Let client upload files
	- [ ] Accurate response status [doc](https://developer.mozilla.org/fr/docs/Web/HTTP/Status)
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

- [ ] Error messages
- [ ] add const and references
- [ ] Leaks
