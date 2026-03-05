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
OBJ_DIR = build/obj
BIN_DIR = build/bin

# Source files - Core server
SERVER_SRCS = $(SRC_DIR)/core/main.cpp \
              $(SRC_DIR)/core/Server.cpp \
              $(SRC_DIR)/core/Client.cpp \
              $(SRC_DIR)/core/Config.cpp

# Source files - HTTP components
HTTP_SRCS = $(SRC_DIR)/core/HttpRequest.cpp \
            $(SRC_DIR)/core/HttpResponse.cpp \
            $(SRC_DIR)/handlers/StaticFileHandler.cpp \
            $(SRC_DIR)/handlers/UploadHandler.cpp

# Combined sources
SRCS = $(SERVER_SRCS) $(HTTP_SRCS)

# Object files
OBJS = $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

# Colors
GREEN = \033[0;32m
CYAN = \033[0;36m
RESET = \033[0m

# Main target
all: $(OBJ_DIR) $(BIN_DIR) $(BIN_DIR)/$(NAME) create_symlink

# Create object directory
$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

# Create bin directory
$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

# Link executable
$(BIN_DIR)/$(NAME): $(OBJS) | $(BIN_DIR)
	@$(CXX) $(CXXFLAGS) -o $@ $^
	@echo "$(GREEN)✓ $(NAME) compiled successfully!$(RESET)"

# Create root symlink for compatibility
create_symlink: $(BIN_DIR)/$(NAME)
	@ln -sf $(BIN_DIR)/$(NAME) $(NAME)

# Compile objects
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) -c $< -o $@
	@echo "$(GREEN)Compiled:$(RESET) $<"

clean:
	@$(RM) $(OBJ_DIR)
	@echo "$(CYAN)✓ Object files removed$(RESET)"

fclean: clean
	@$(RM) $(BIN_DIR)
	@$(RM) $(NAME)
	@echo "$(CYAN)✓ $(NAME) removed$(RESET)"
	@echo "$(CYAN)✓ $(NAME) removed$(RESET)"

re: fclean all

run: all
	@./$(NAME) config/webserv.conf

.PHONY: all clean fclean re run create_symlink
