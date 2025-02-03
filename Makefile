CC = gcc
CFLAGS = -Wall -Wextra -g -Og $(INC_DIR) -D__DEBUG__ -fPIC
LDFLAGS = -L/opt/homebrew/lib -lcurl -lpthread -lcrypto -lssl -lcjson -ldl
SATORINOW_SRC_DIR = src/satorinow
SATORICLI_SRC_DIR = src/satoricli
MODULES_DIR = src/modules
INC_DIR = -Isrc/include -I/opt/homebrew/include
BUILD_DIR = build
BIN_DIR = bin
INSTALL_DIR = /usr/local/satorinow/bin
INSTALL_MODULE_DIR = $(INSTALL_DIR)/modules

# Source files
SATORINOW_SRC = $(SATORINOW_SRC_DIR)/main.c \
	$(SATORINOW_SRC_DIR)/http/http_neuron.c \
	$(SATORINOW_SRC_DIR)/cli.c \
	$(SATORINOW_SRC_DIR)/cli/cli_satori.c \
	$(SATORINOW_SRC_DIR)/encrypt.c \
	$(SATORINOW_SRC_DIR)/json.c \
	$(SATORINOW_SRC_DIR)/repository.c \

SATORICLI_SRC = $(SATORICLI_SRC_DIR)/main.c

MODULES_SRC = $(wildcard $(MODULES_DIR)/*.c)
MODULES_SO = $(MODULES_SRC:.c=.so)

# Binaries
SATORINOW_BIN = $(BUILD_DIR)/satorinow
SATORICLI_BIN = $(BUILD_DIR)/satoricli

# Targets
.PHONY: all clean install uninstall

all: $(SATORINOW_BIN) $(SATORICLI_BIN) $(MODULES_SO)

# Build daemon
$(SATORINOW_BIN): $(SATORINOW_SRC)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(SATORICLI_BIN): $(SATORICLI_SRC)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $<

# Build modules
$(MODULES_DIR)/%.so: $(MODULES_DIR)/%.c
	$(CC) $(CFLAGS) -shared -o $@ $< $(LDFLAGS)

clean:
	@echo "Cleaning build directory..."
	rm -rf $(BUILD_DIR) $(MODULES_DIR)/*.so

install: all
	@echo "Installing binaries to $(INSTALL_DIR)..."
	@mkdir -p $(INSTALL_DIR)
	@mkdir -p $(INSTALL_MODULE_DIR)
	cp $(SATORINOW_BIN) $(SATORICLI_BIN) $(INSTALL_DIR)
	cp $

uninstall:
	@echo "Uninstalling binaries from $(INSTALL_DIR)..."
	rm -f $(INSTALL_DIR)/satorinow
	rm -f $(INSTALL_DIR)/satoricli
