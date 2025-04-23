# REQUIREMENTS #
NAME = ircserv
CXX = c++
CXXFLAGS = -std=c++17 -Wall -Wextra -Werror

# DIRECTORYZ #
SRC_DIR = source/src
INC_DIR = source/inc
OBJ_DIR = obj

# ADDITIONAL DIRECTORYZ # wille be added soon .
#  CMDS
#  PARSING
#  SOCKET INIT
#  . . .

# FILEZ #
SRC_FILES := $(wildcard $(SRC_DIR)/*.cpp)
OBJ_FILES := $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRC_FILES))

# RULEZ #
.SILENT :

all : $(NAME)

$(NAME) : $(OBJ_FILES)
	$(CXX) $(CXXFLAGS) $^ -o $@
	echo "$(NAME) got compiled"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean :
	rm -rf $(OBJ_DIR)
	echo "deleted object files"

fclean : clean
	rm -rf $(NAME)
	echo "deleted all files"

re : fclean all
	echo "re-creating . . ."

.PHONY : all clean fclean re