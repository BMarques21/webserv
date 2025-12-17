###############################################################################
#                             webserv by brunmart &viceda-s                             #
#                                                                             #
###############################################################################

#──────────────────────────────Compilation Settings───────────────────────────#

NAME	= webserv
CC		= c++
FLAGS	= -Werror -Wextra -Wall -std=c++98
O_DIR	= obj/
RM		= rm -rf

all: $(HEADER) $(NAME)

#───────────────────────────────Animation Variables───────────────────────────#

YELLOW			= \033[38;2;255;248;147m# FFF893
PINK			= \033[38;2;231;133;190m# E785BE
GREEN			= \033[38;2;129;223;164m# 81DFA4
RESET			= \033[0m

HEADER			= $(O_DIR)/.header
BAR_SIZE		= 43
COMPILED_COUNT	= $(O_DIR)/.compiled_count

#────────────────────────────Mandatory Compilation────────────────────────────#

# Core server files
CORE_SRC	= main.cpp Server.cpp
CORE_SRC	:= $(addprefix server/, $(CORE_SRC))

# HTTP protocol files
HTTP_SRC	= Client.cpp Request.cpp Response.cpp
HTTP_SRC	:= $(addprefix server/, $(HTTP_SRC))

# Configuration files
CONFIG_SRC	= Config.cpp
CONFIG_SRC	:= $(addprefix server/, $(CONFIG_SRC))

# Utility files
UTIL_SRC	= Utils.cpp HttpStatus.cpp

# CGI handler
CGI_SRC		= CgiHandler.cpp

# Combine all sources
SRC			= $(CORE_SRC) $(HTTP_SRC) $(CONFIG_SRC) $(UTIL_SRC) $(CGI_SRC)
SRC			:= $(addprefix src/, $(SRC))
OBJ			= $(SRC:%.cpp=$(O_DIR)/%.o)
INC			= -I inc/

OBJ_COUNT 	= $(words $(OBJ))

# Compile .cpp to .o
$(O_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@$(CC) $(FLAGS) -c $< -o $@ $(INC);
	@current=$$(cat $(COMPILED_COUNT) 2>/dev/null || echo 0); \
	bar_size=$(BAR_SIZE); \
	next=$$(( $$current + 1 )); \
	echo $$next > $(COMPILED_COUNT); \
	if [ $$current -eq 0 ]; then \
		printf "%-12.12s %-10.10s $(GREEN)" "Compiling" $(NAME);  \
	fi; \
	filled=$$(( $$next * $$bar_size / $(OBJ_COUNT) )); \
	empty=$$(( $$bar_size - $$filled )); \
	printf "\r%-12.12s %-10.10s $(GREEN)[" "Compiling" $(NAME); \
	printf "%$${filled}s" | tr ' ' '='; \
	printf "%$${empty}s" | tr ' ' ' '; \
	printf "]$(RESET) %3d%%" $$(( $$next * 100 / $(OBJ_COUNT) )); \
	if [ $$next -eq $(OBJ_COUNT) ]; then \
		printf "\n"; \
	fi

# Link .o to executable
$(NAME): $(OBJ)
	@$(CC) $(FLAGS) $(OBJ) -o $(NAME)
	@echo "$(PINK)✓ $(NAME) compiled successfully!$(RESET)"

# Header
$(HEADER):
	@mkdir -p $(O_DIR)
	@echo "$(YELLOW)"
	@echo "╔══════════════════════════════════════════════════════════════════╗"
	@echo "║                           WEBSERV                                ║"
	@echo "║                      HTTP/1.1 Server in C++98                    ║"
	@echo "╚══════════════════════════════════════════════════════════════════╝"
	@echo "$(RESET)"
	@touch $(HEADER)
	@echo "0" > $(COMPILED_COUNT)

#──────────────────────────────────Cleanup────────────────────────────────────#

clean:
	@$(RM) $(O_DIR)
	@echo "$(PINK)✓ Object files removed$(RESET)"

fclean: clean
	@$(RM) $(NAME)
	@echo "$(PINK)✓ $(NAME) removed$(RESET)"

re: fclean all

.PHONY: all clean fclean re

