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
	- [x] faire des HTTP redirection 301 + header Location: URL
	- [x] possiblity de changer les pages d'error
	- [x] autoindex on/off
	- [x] allowed_method
	- [ ] devrait pas fonctionner si plusieurs fois le meme port utilise
	- [ ] Setup the server_names or not.
- [ ] parsing des requetes clients
	- [x] analyse messages requetes
	- [x] create d'objet Request
	- [x] tronquer le body a max_client_body_size
- [ ] creation des reponses du serveur
	- [x] header
	- [x] body => aller chercher le fichier correspondant
	- [x] envoyer au client
	- [x] DELETE
	- [x] adapter les status de reponse
	- [x] adapter les reponses en fonction de la Conf
	- [x] default error page
	- [x] send image
	- [x] utiliser autoindex
	- [x] 301 request if file is folder to folder/
	- [ ] upload fichier
		- [x] handle client_max_body_size
		- [x] handle content-length
		- [ ] upload.php script
	- [x] download fichier
- [x] CGI
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
	- [x] unchunck request
- [x] clean
	- [x] deplacer dans utils
	- [x] decouper les fonctions
- [x] proteger les fichiers elementaires
- [ ] default site to demonstrate
- [ ] default conf to demonstrate
- [x] Une requête à votre serveur ne devrait jamais se bloquer indéfiniment.
- [x] Exécuter CGI en fonction de certaines extensions de fichier (par exemple .php).
- [x] Rendre la route capable d’accepter les fichiers téléchargés et configurer où cela doit être enregistré.
- [x] Error messages
- [x] add const and references
- [x] assignation operators
- [x] Leaks
- [x] error when reloading page after uploaded file

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
