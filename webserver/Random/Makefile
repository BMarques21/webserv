##############################################################################
#                       webserv by brunmart & viceda-s                       #
##############################################################################

#─────────────────────────────Compilation Settings───────────────────────────#

NAME	= webserv
CC		= c++
FLAGS	= -Werror -Wextra -Wall -std=c++98
O_DIR	= obj/
RM		= rm -rf
HEADER	= $(O_DIR)/.header

all: $(HEADER) $(NAME)

#──────────────────────────────Animation Variables───────────────────────────#

YELLOW			= \033[38;2;255;248;147m# FFF893
PINK			= \033[38;2;231;133;190m# E785BE
GREEN			= \033[38;2;129;223;164m# 81DFA4
RESET			= \033[0m

BAR_SIZE		= 43
COMPILED_COUNT	= $(O_DIR)/.compiled_count

#───────────────────────────Mandatory Compilation────────────────────────────#

# Server directory files (src/server/)
SERVER_SRC	= main.cpp Server.cpp Client.cpp Request.cpp Response.cpp Config.cpp
SERVER_SRC	:= $(addprefix src/server/, $(SERVER_SRC))

# Root src directory files (src/)
ROOT_SRC	= CgiHandler.cpp HttpStatus.cpp Utils.cpp
ROOT_SRC	:= $(addprefix src/, $(ROOT_SRC))

# Combine all sources
SRC			= $(SERVER_SRC) $(ROOT_SRC)
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
	@touch $@
	@printf "$(GREEN)"
	@printf "╔═══════════════════════════════════════════════════════════════════════════════════════════════════════╗\n"
	@printf "║  ░▒▓█▓▒░░▒▓█▓▒░░▒▓█▓▒░▒▓████████▓▒░▒▓███████▓▒░ ░▒▓███████▓▒░▒▓████████▓▒░▒▓███████▓▒░░▒▓█▓▒░░▒▓█▓▒░  ║\n"
	@printf "║  ░▒▓█▓▒░░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░      ░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░      ░▒▓█▓▒░      ░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░  ║\n"
	@printf "║  ░▒▓█▓▒░░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░      ░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░      ░▒▓█▓▒░      ░▒▓█▓▒░░▒▓█▓▒░░▒▓█▓▒▒▓█▓▒░   ║\n"
	@printf "║  ░▒▓█▓▒░░▒▓█▓▒░░▒▓█▓▒░▒▓██████▓▒░ ░▒▓███████▓▒░ ░▒▓██████▓▒░░▒▓██████▓▒░ ░▒▓███████▓▒░ ░▒▓█▓▒▒▓█▓▒░   ║\n"
	@printf "║  ░▒▓█▓▒░░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░      ░▒▓█▓▒░░▒▓█▓▒░      ░▒▓█▓▒░▒▓█▓▒░      ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▓█▓▒░    ║\n"
	@printf "║  ░▒▓█▓▒░░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░      ░▒▓█▓▒░░▒▓█▓▒░      ░▒▓█▓▒░▒▓█▓▒░      ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▓█▓▒░    ║\n"
	@printf "║   ░▒▓█████████████▓▒░░▒▓████████▓▒░▒▓███████▓▒░░▒▓███████▓▒░░▒▓████████▓▒░▒▓█▓▒░░▒▓█▓▒░  ░▒▓██▓▒░     ║\n"
	@printf "║                                                                                                       ║\n"
	@printf "║                                        𝗯𝘆 𝗯𝗿𝘂𝗻𝗺𝗮𝗿𝘁 & 𝘃𝗶𝗰𝗲𝗱𝗮-𝘀                                         ║\n"
	@printf "╚═══════════════════════════════════════════════════════════════════════════════════════════════════════╝\n"
	@printf "$(RESET)\n"
	@touch $(HEADER)


#─────────────────────────────────Cleanup────────────────────────────────────#

clean:
	@$(RM) $(O_DIR)
	@echo "$(PINK)✓ Object files removed$(RESET)"

fclean: clean
	@$(RM) $(NAME)
	@echo "$(PINK)✓ $(NAME) removed$(RESET)"

re: fclean all

.PHONY: all clean fclean re
