# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: schmitzi <schmitzi@student.42.fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2023/11/22 14:29:42 by mgeiger-          #+#    #+#              #
#    Updated: 2024/11/11 14:29:35 by mgeiger-         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME	=	webserv

CXX		=	c++
CFLAGS	=	-Wall -Werror -Wextra -std=c++98
RM		=	rm -f

RED		=	\e[0;91m
GREEN	=	\e[0;92m
YELLOW	=	\e[0;33m
RESET	=	\e[0m

FILES	=	main \
			Webserv \
			Config

SRC_DIR = 	src/
SRC 	= 	$(addprefix $(SRC_DIR), $(addsuffix .cpp, $(FILES)))

OBJ_DIR	= 	obj/
OBJ		= 	$(addprefix $(OBJ_DIR), $(addsuffix .o, $(FILES)))

$(OBJ_DIR)%.o: $(SRC_DIR)%.cpp
	@mkdir -p $(OBJ_DIR)
	@$(CXX) $(CFLAGS) -c -o $@ $<

all:	$(NAME)

$(NAME): $(OBJ)
	@echo "$(YELLOW)Compiling exercise$(RESET)"
	@$(CXX) $(OBJ) $(CFLAGS) -o $(NAME)
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

.PHONY:		all clean fclean re
