CC = gcc
CFLAGS = -Wall -Wextra -g -Og $(INC_DIR) -D__DEBUG__
LDFLAGS = -L/opt/homebrew/lib -lcurl -lpthread -lcrypto -lssl
SATORINOW_SRC_DIR = src/satorinow
SATORICLI_SRC_DIR = src/satoricli
INC_DIR = -Isrc/include -I/opt/homebrew/include
BUILD_DIR = build
BIN_DIR = bin
INSTALL_DIR = /usr/local/satori-now/bin

# Source files
SATORINOW_SRC = $(SATORINOW_SRC_DIR)/main.c \
	$(SATORINOW_SRC_DIR)/cli.c \
	$(SATORINOW_SRC_DIR)/cli/cli_satori.c \
	$(SATORINOW_SRC_DIR)/encrypt.c \
	$(SATORINOW_SRC_DIR)/repository.c \

SATORICLI_SRC = $(SATORICLI_SRC_DIR)/main.c

# Binaries
SATORINOW_BIN = $(BUILD_DIR)/satorinow
SATORICLI_BIN = $(BUILD_DIR)/satoricli

# Targets
.PHONY: all clean install uninstall

all: $(SATORINOW_BIN) $(SATORICLI_BIN)

# Build daemon
$(SATORINOW_BIN): $(SATORINOW_SRC)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(SATORICLI_BIN): $(SATORICLI_SRC)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $<

clean:
	@echo "Cleaning build directory..."
	rm -rf $(BUILD_DIR)

install: all
	@echo "Installing binaries to $(INSTALL_DIR)..."
	@mkdir -p $(INSTALL_DIR)
	cp $(SATORINOW_BIN) $(SATORICLI_BIN) $(INSTALL_DIR)

uninstall:
	@echo "Uninstalling binaries from $(INSTALL_DIR)..."
	rm -f $(INSTALL_DIR)/satori_now_daemon
	rm -f $(INSTALL_DIR)/satori_now_cli
