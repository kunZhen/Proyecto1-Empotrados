# ========= Config =========
PREFIX ?= /usr/local
LIBNAME := gpioio
MAJOR := 1
MINOR := 0
PATCH := 0

LIB_FULL  := lib$(LIBNAME).so.$(MAJOR).$(MINOR).$(PATCH)
LIB_SONAME:= lib$(LIBNAME).so.$(MAJOR)
LIB_LINK  := lib$(LIBNAME).so

CC ?= gcc
CPPFLAGS += -Iinclude
CFLAGS   += -fPIC -Wall -Wextra
LDFLAGS_SHARED := -shared -Wl,-soname,$(LIB_SONAME)

BUILD_DIR := build
OBJ_DIR   := $(BUILD_DIR)/obj
LIB_DIR   := $(BUILD_DIR)/lib
BIN_DIR   := $(BUILD_DIR)/bin

#SDROOT ?= /media/emilio/root1      # <-- ajusta si cambia el punto de montaje de tu uSD
APPNAME := test_app

# ========= Reglas =========
all: dirs $(LIB_DIR)/$(LIB_FULL) $(BIN_DIR)/$(APPNAME)

dirs:
	@mkdir -p $(OBJ_DIR) $(LIB_DIR) $(BIN_DIR)

$(OBJ_DIR)/gpioio.o: src/gpioio.c include/gpioio.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(LIB_DIR)/$(LIB_FULL): $(OBJ_DIR)/gpioio.o
	$(CC) $(LDFLAGS_SHARED) -o $@ $^
	ln -sf $(LIB_FULL) $(LIB_DIR)/$(LIB_SONAME)
	ln -sf $(LIB_SONAME) $(LIB_DIR)/$(LIB_LINK)

$(BIN_DIR)/$(APPNAME): examples/test_app.c include/gpioio.h $(LIB_DIR)/$(LIB_FULL)
	$(CC) -Iinclude -o $@ $< -L$(LIB_DIR) -l$(LIBNAME) -Wl,-rpath,'$$ORIGIN/../lib'

clean:
	rm -rf $(BUILD_DIR)

install:
	install -d $(DESTDIR)$(PREFIX)/lib $(DESTDIR)$(PREFIX)/include $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(LIB_DIR)/$(LIB_FULL) $(DESTDIR)$(PREFIX)/lib/
	ln -sf $(LIB_FULL) $(DESTDIR)$(PREFIX)/lib/$(LIB_SONAME)
	ln -sf $(LIB_SONAME) $(DESTDIR)$(PREFIX)/lib/$(LIB_LINK)
	install -m 644 include/gpioio.h $(DESTDIR)$(PREFIX)/include/
	install -m 755 $(BIN_DIR)/$(APPNAME) $(DESTDIR)$(PREFIX)/bin/

# Copia a la uSD bajo /opt/gpioio/{bin,lib} (recomendado)
deploy-sd: all
	install -d $(SDROOT)/opt/gpioio/bin $(SDROOT)/opt/gpioio/lib
	install -m 755 $(BIN_DIR)/$(APPNAME) $(SDROOT)/opt/gpioio/bin/
	# copia la lib + symlinks
	install -m 755 $(LIB_DIR)/$(LIB_FULL) $(SDROOT)/opt/gpioio/lib/
	ln -sf $(LIB_FULL) $(SDROOT)/opt/gpioio/lib/$(LIB_SONAME)
	ln -sf $(LIB_SONAME) $(SDROOT)/opt/gpioio/lib/$(LIB_LINK)

.PHONY: all dirs clean install deploy-sd

