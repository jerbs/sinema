TOP_DIR := $(realpath $(CURDIR)/..)
PREFIX ?= $(TOP_DIR)/root
FFMPEG_BUILD_DIR := $(CURDIR)/ffmpeg

.PHONY: all
all: sinema_make

.PHONY: install
install: sinema_install

# --------------------------------------------------------------------
# FFmpeg

define ffmpeg_configure_recipe
    cd $(CURDIR)/ffmpeg; \
    $(TOP_DIR)/ffmpeg/configure \
        --prefix=$(PREFIX) \
        --enable-gpl \
        --enable-version3 \
        --enable-nonfree \
        --enable-shared
endef

define ffmpeg_make_recipe
    cd $(CURDIR)/ffmpeg; \
    make
endef

define ffmpeg_install_recipe
    cd $(CURDIR)/ffmpeg; \
    make install
endef

$(FFMPEG_BUILD_DIR):
	mkdir $@

.PHONY: ffmpeg_configure
ffmpeg_configure $(FFMPEG_BUILD_DIR)/config.mak: | $(FFMPEG_BUILD_DIR)
	$(ffmpeg_configure_recipe)

.PHONY: ffmpeg_make
ffmpeg_make $(FFMPEG_BUILD_DIR)/ffmpeg: $(FFMPEG_BUILD_DIR)/config.mak
	$(ffmpeg_make_recipe)

.PHONY: ffmpeg_install
ffmpeg_install $(PREFIX)/bin/ffmpeg: $(FFMPEG_BUILD_DIR)/ffmpeg
	$(ffmpeg_install_recipe)

# --------------------------------------------------------------------
# sinema

define sinema_env
    export CFLAGS="-g -O0"; \
    export CXXFLAGS="-g -O0"; \
    export CPPFLAGS="-Wall -Wextra -I$(PREFIX)/include"; \
    export FIRST_LDFLAGS="-L$(PREFIX)/lib"; \
    export PKG_CONFIG_PATH=$(PREFIX)/lib/pkgconfig
endef

define sinema_bootstrap_recipe
    cd $(TOP_DIR); \
    ./bootstrap
endef

define sinema_configure_recipe
    cd $(TOP_DIR); \
    $(sinema_env); \
    ./configure --prefix=$(PREFIX) \
                --with-boost-libdir=/usr/lib/x86_64-linux-gnu
endef

define sinema_make_recipe
    cd $(TOP_DIR); \
    $(sinema_env); \
    make
endef

define sinema_install_recipe
    cd $(TOP_DIR); \
    $(sinema_env); \
    make install
endef

.PHONY: sinema_bootstrap
sinema_bootstrap $(TOP_DIR)/configure: $(PREFIX)/bin/ffmpeg
	$(sinema_bootstrap_recipe)

.PHONY: sinema_configure
sinema_configure $(TOP_DIR)/Makefile: $(TOP_DIR)/configure
	$(sinema_configure_recipe)

.PHONY: sinema_make
sinema_make $(TOP_DIR)/gui/sinema: $(TOP_DIR)/Makefile
	$(sinema_make_recipe)

.PHONY: sinema_install
sinema_install: $(TOP_DIR)/gui/sinema
	$(sinema_install_recipe)

# --------------------------------------------------------------------
