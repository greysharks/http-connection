SRCS_DIR = src
INCLUDES_DIR = include

SRCS = $(SRCS_DIR)/main.c \
	$(SRCS_DIR)/client.c \
	$(SRCS_DIR)/server.c \
	$(SRCS_DIR)/tools.c \

INCLUDES = -I $(INCLUDES_DIR)/common/
