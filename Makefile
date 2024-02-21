SRCS = main.cpp Server.cpp parse/DataConfige.cpp parse/ParseConfigeFile.cpp

OBJS = $(SRCS:.cpp=.o)

NAME = webserv

CPP = c++

FLAGS = -Wall -Wextra -Werror -std=c++98 -fsanitize=address -g

all : $(NAME)

$(NAME) : $(OBJS)
	$(CPP) $(FLAGS) $(OBJS) -o $(NAME)

%.o:%.cpp Server.hpp
	$(CPP) $(FLAGS) -c $< -o $@ 

clean:
	rm -rf $(OBJS)

fclean: clean
	rm -rf $(NAME)

re: fclean all

.PHONY: all clean fclean re