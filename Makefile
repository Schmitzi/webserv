NAME	=	webserv

CXX		=	c++
CXXFLAGS	=	-Wall -Werror -Wextra -std=c++98
DEPFLAGS =	-MMD -MP
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

DEP_DIR	= 	dep/
DEP		= 	$(addprefix $(DEP_DIR), $(addsuffix .d, $(FILES)))

all: $(NAME)

$(OBJ_DIR) $(DEP_DIR):
	@mkdir -p $@

$(OBJ_DIR)%.o: $(SRC_DIR)%.cpp | $(OBJ_DIR) $(DEP_DIR)
	@echo "$(YELLOW)Compiling $<$(RESET)"
	@$(CXX) $(CXXFLAGS) $(DEPFLAGS) -MF $(DEP_DIR)$*.d -c -o $@ $<

$(NAME): $(OBJ)
	@echo "$(YELLOW)Linking $(NAME)$(RESET)"
	@$(CXX) $(OBJ) -o $(NAME)
	@echo "$(GREEN)$(NAME) built successfully$(RESET)"

clean:
	@if [ -d "$(OBJ_DIR)" ]; then \
		echo "$(RED)Removing $(OBJ_DIR)$(RESET)"; \
		$(RM) -r $(OBJ_DIR); \
	fi
	@if [ -d "$(DEP_DIR)" ]; then \
		echo "$(RED)Removing $(DEP_DIR)$(RESET)"; \
		$(RM) -r $(DEP_DIR); \
	fi

fclean: clean
	@if [ -f "$(NAME)" ]; then \
		echo "$(RED)Removing $(NAME)$(RESET)"; \
		$(RM) $(NAME); \
	fi

re: fclean all

start: all
	@echo "$(GREEN)Starting $(NAME) with test config$(RESET)"
	@./$(NAME) config/default.conf

val: all
	@echo "$(GREEN)Running $(NAME) with valgrind$(RESET)"
	@valgrind --track-origins=yes --leak-check=full --show-leak-kinds=all --track-fds=all ./$(NAME) config/default.conf

.PHONY: all clean fclean re start val

-include $(DEP)
