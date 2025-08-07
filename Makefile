NAME	=	webserv

CXX		=	c++
CXXFLAGS	=	-std=c++98 -pedantic -Wall -Wextra -Werror
RM		=	rm -f

RED     =   $(shell tput setaf 1)
GREEN   =   $(shell tput setaf 2)
YELLOW  =   $(shell tput setaf 3)
RESET   =   $(shell tput sgr0)

FILES	=	main \
			Webserv \
			ConfigHelper \
			ConfigParser \
			ConfigValidator \
			Server \
			Client \
			Request \
			CGIHandler \
			Multipart \
			Helper \
			Response \
			EpollHelper \
			ClientHelper

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
	@./webserv config/test.conf

val: re
	@valgrind --track-origins=yes --leak-check=full --show-leak-kinds=all --track-fds=all ./webserv config/test.conf

.PHONY:		all clean fclean re
