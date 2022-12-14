S			=	src/
I			=	header/
O			=	obj/
D			=	dep/

NAME		=	webserv
SRC			=	main.cpp		\
				Conf.cpp		\
				CGI.cpp			\
				Parser.cpp		\
				Request.cpp		\
				Response.cpp	\
				Server.cpp		\
				ServerUtils.cpp	\
				Utils.cpp

OBJ			=	$(SRC:%.cpp=$O%.o)
DEP			=	$(SRC:%.cpp=$D%.d)

CXX			=	c++

CXXFLAGS	+=	-I$I
CXXFLAGS	+=	-Wall -Wextra -Werror -std=c++98 -pedantic-errors
CXXFLAGS	+=	-g3 -fsanitize=address

LDFLAGS		+=	-Wall -Wextra -Werror -std=c++98 -pedantic-errors
LDFLAGS		+=	-g3 -fsanitize=address

RM			=	/bin/rm -f
RMDIR		=	/bin/rm -Rf

.PHONY: all clean fclean re

all: $(NAME)

$O:
	@mkdir -p $@

$(OBJ): | $O

$(OBJ): $O%.o: $S%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$D:
	@mkdir -p $@

$(DEP): | $D

$(DEP): $D%.d: $S%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -MM -MF $@ -MT "$O$*.o $@" $<

$(NAME): $(OBJ)
	$(CXX) $(LDFLAGS) $(OBJ) -o $@

clean:
	$(RMDIR) $(wildcard $(NAME).dSYM)
	$(RMDIR) $O
	$(RM) $(wildcard $(DEP))
	$(RMDIR) $D

fclean: clean
	$(RM) $(NAME)

re: fclean
	$(MAKE)

-include $(DEP)
