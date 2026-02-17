##############################################################################
#                       webserv - HTTP/1.1 Server                           #
##############################################################################

NAME = webserv
CXX = g++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -Iincludes
RM = rm -rf

# Directories
SRC_DIR = srcs
INC_DIR = includes
OBJ_DIR = objs

# Source files - Core server
SERVER_SRCS = $(SRC_DIR)/main.cpp \
              $(SRC_DIR)/Server.cpp \
              $(SRC_DIR)/Client.cpp \
              $(SRC_DIR)/Config.cpp

# Source files - HTTP components
HTTP_SRCS = $(SRC_DIR)/HttpRequest.cpp \
            $(SRC_DIR)/HttpResponse.cpp \
            $(SRC_DIR)/StaticFileHandler.cpp \
            $(SRC_DIR)/UploadHandler.cpp

# Combined sources
SRCS = $(SERVER_SRCS) $(HTTP_SRCS)

# Object files
OBJS = $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

# Colors
GREEN = \033[0;32m
CYAN = \033[0;36m
RESET = \033[0m

# Main target
all: $(OBJ_DIR) $(NAME)

# Create object directory
$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

# Link executable
$(NAME): $(OBJS)
	@$(CXX) $(CXXFLAGS) -o $@ $^
	@echo "$(GREEN)✓ $(NAME) compiled successfully!$(RESET)"

# Compile objects
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) -c $< -o $@
	@echo "$(GREEN)Compiled:$(RESET) $<"

clean:
	@$(RM) $(OBJ_DIR)
	@echo "$(CYAN)✓ Object files removed$(RESET)"

fclean: clean
	@$(RM) $(NAME)
	@echo "$(CYAN)✓ $(NAME) removed$(RESET)"
	@echo "$(CYAN)✓ $(NAME) removed$(RESET)"

re: fclean all

run: $(NAME)
	@./$(NAME) config/webserv.conf

.PHONY: all clean fclean re run
