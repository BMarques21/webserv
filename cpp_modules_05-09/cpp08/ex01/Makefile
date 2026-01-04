NAME		= span

CXX			= c++
CXXFLAGS	= -Wall -Wextra -Werror -std=c++98

SRCS		= main.cpp Span.cpp

OBJ			= $(SRCS:.cpp=.o)

GREEN = \033[1;32m
YELLOW = \033[1;33m
YELLOWLIGHT = \033[0;33m
RED = \033[1;31m
BLUE = \033[1;34m
RESET = \033[0m

COMPILED_ONCE = $(shell test -f .compiled && echo "true" || echo "false")

all: $(NAME)

$(NAME): $(OBJ)
	@echo "$(GREEN)[C++] $(NAME) compiled$(RESET)"
	@$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME)

%.o: %.cpp
	@if [ ! -f .compiled ]; then \
	 echo "$(YELLOW)[C++] Compiling C++ files...$(RESET)"; \
	 touch .compiled; \
	fi
	@echo "$(YELLOWLIGHT)Compiling $<...$(RESET)"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@rm -f $(OBJ) .compiled
	@echo "$(BLUE)[C++] Object files removed$(RESET)"

fclean: clean
	@rm -f $(NAME)
	@echo "$(RED)[C++] $(NAME) removed$(RESET)"

re: fclean all

.PHONY: all clean fclean re
