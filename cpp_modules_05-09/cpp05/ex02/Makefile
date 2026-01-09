NAME		= bureaucrat

CXX			= c++
CXXFLAGS	= -Wall -Wextra -Werror -std=c++98

SRCS		= main.cpp \
			  Bureaucrat.cpp \
			  AForm.cpp \
			  ShrubberyCreationForm.cpp \
			  RobotomyRequestForm.cpp \
			  PresidentialPardonForm.cpp

OBJ			= $(SRCS:.cpp=.o)

GREEN = \033[1;32m
BLUE = \033[1;34m
BLUELIGHT = \033[0;34m
RED = \033[1;31m
REDLIGHT = \033[0;31m
RESET = \033[0m

COMPILED_ONCE = $(shell test -f .compiled && echo "true" || echo "false")

all: $(NAME)

$(NAME): $(OBJ)
	@echo "$(GREEN)[C++] $(NAME) compiled$(RESET)"
	@$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME)

%.o: %.cpp
	@if [ ! -f .compiled ]; then \
	 echo "$(BLUE)[C++] Compiling C++ files...$(RESET)"; \
	 touch .compiled; \
	fi
	@echo "$(BLUELIGHT)Compiling $<...$(RESET)"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@rm -f $(OBJ) .compiled
	@echo "$(REDLIGHT)[C++] Object files removed$(RESET)"

fclean: clean
	@rm -f $(NAME)
	@echo "$(RED)[C++] $(NAME) removed$(RESET)"

re: fclean all

.PHONY: all clean fclean re