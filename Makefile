NAME	=	webserv

CXX		=	c++
CXXFLAGS	=	-Wall -Werror -Wextra -std=c++98 -g
RM		=	rm -f

RED     =   $(shell tput setaf 1)
GREEN   =   $(shell tput setaf 2)
YELLOW  =   $(shell tput setaf 3)
RESET   =   $(shell tput sgr0)

FILES	=	main \
			Webserv \
			Config \
			ConfigHelper \
			ConfigParser \
			ConfigValidator \
			Server \
			Client \
			Request \
			CGIHandler \
			Multipart \
			Helper \
			Response

SRC_DIR = 	src/
SRC 	= 	$(addprefix $(SRC_DIR), $(addsuffix .cpp, $(FILES)))

OBJ_DIR	= 	obj/
OBJ		= 	$(addprefix $(OBJ_DIR), $(addsuffix .o, $(FILES)))

$(OBJ_DIR)%.o: $(SRC_DIR)%.cpp
	@mkdir -p $(OBJ_DIR)
	@$(CXX) $(CXXFLAGS) -c -o $@ $<

all:	$(NAME)

$(NAME): $(OBJ)
	@echo "$(YELLOW)Compiling exercise$(RESET)"
	@$(CXX) $(OBJ) $(CXXFLAGS) -o $(NAME)
	@echo "$(GREEN)Exercise built$(RESET)"

clean:
	@$(RM) $(OBJ)
	@if [ -d "obj" ]; then \
		rm -r obj/; \
	fi
	@echo "$(RED)Removed objects$(RESET)"

fclean:	clean
	@$(RM) $(NAME)
	@echo "$(RED)All files cleaned$(RESET)"

re:		fclean all
	@clear ;
	@echo "$(GREEN)Files cleaned and program re-compiled$(RESET)"

start: re
	@./webserv

val: re
	@valgrind --track-origins=yes --leak-check=full --track-fds=all ./webserv config/test.conf

.PHONY:		all clean fclean re
