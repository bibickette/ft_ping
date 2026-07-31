CC = cc
INCLUDE = include
CFLAGS = -Wall -Wextra -Werror -g3 -I$(INCLUDE) 

NAME = ft_ping

SRC_DIR = src
OBJ_DIR = obj

BLUE = \033[0;34m
RESET = \033[0m

SRC_FILES = \
	parse_opts.c 

		
SRC = $(addprefix $(SRC_DIR)/, $(SRC_FILES))

ALL_FILES = $(SRC) \
			main.c \

PMANDATORY =  $(addprefix , $(ALL_FILES))
OBJS = $(PMANDATORY:$(SRC_DIR)%.c=$(OBJ_DIR)%.o)

all : $(NAME)

$(OBJ_DIR)/%.o :  $(SRC_DIR)/%.c 
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(NAME): $(OBJS)
	@echo "$(BLUE)Compiling ft_ping...$(RESET)"
	$(CC) $(CFLAGS) $(OBJS) -o $(NAME)

clean :
	rm -rf $(OBJ_DIR)

fclean : clean
	rm -rf $(NAME)

re : fclean all

.PHONY: all re clean fclean