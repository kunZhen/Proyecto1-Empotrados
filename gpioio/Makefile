# ========= Variables de configuración =========
PREFIX ?= /usr/local
LIBNAME := gpioio
MAJOR := 1
MINOR := 1
PATCH := 0

# Nombres de la biblioteca
LIB_FULL := lib$(LIBNAME).so.$(MAJOR).$(MINOR).$(PATCH)
LIB_SONAME := lib$(LIBNAME).so.$(MAJOR)
LIB_LINK := lib$(LIBNAME).so

# Compilador y flags
CC ?= gcc
CPPFLAGS += -Iinclude
CFLAGS += -fPIC -Wall -Wextra
LDFLAGS_SHARED := -shared -Wl,-soname,$(LIB_SONAME)

# Estructura de directorios
BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
LIB_DIR := $(BUILD_DIR)/lib
BIN_DIR := $(BUILD_DIR)/bin

# Configuración SSH para Raspberry Pi
RPI_USER ?= root
RPI_HOST ?= 192.168.100.234
RPI_PATH ?= /root/gpioio

# Nombres de aplicaciones
APPNAME := test_app
MOTOR_TEST := motor_test
VEHICLE_TEST := vehicle_test

# ========= Reglas principales =========

.PHONY: all clean dirs install deploy-ssh test-remote test-motors-remote

# Regla por defecto
all: dirs $(LIB_DIR)/$(LIB_FULL) $(BIN_DIR)/$(APPNAME) $(BIN_DIR)/$(MOTOR_TEST) $(BIN_DIR)/$(VEHICLE_TEST)

# Crear directorios
dirs:
	@mkdir -p $(OBJ_DIR) $(LIB_DIR) $(BIN_DIR)

# Compilar objeto
$(OBJ_DIR)/gpioio.o: src/gpioio.c include/gpioio.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# Crear biblioteca compartida
$(LIB_DIR)/$(LIB_FULL): $(OBJ_DIR)/gpioio.o
	$(CC) $(LDFLAGS_SHARED) -o $@ $^
	cd $(LIB_DIR) && ln -sf $(LIB_FULL) $(LIB_SONAME)
	cd $(LIB_DIR) && ln -sf $(LIB_SONAME) $(LIB_LINK)

# Compilar aplicación de prueba básica
$(BIN_DIR)/$(APPNAME): examples/test_app.c include/gpioio.h $(LIB_DIR)/$(LIB_FULL)
	$(CC) -Iinclude -o $@ $< -L$(LIB_DIR) -l$(LIBNAME) -Wl,-rpath,'$$ORIGIN/../lib'

# Compilar aplicación de prueba de motores
$(BIN_DIR)/$(MOTOR_TEST): examples/motor_test.c include/gpioio.h $(LIB_DIR)/$(LIB_FULL)
	$(CC) -Iinclude -o $@ $< -L$(LIB_DIR) -l$(LIBNAME) -Wl,-rpath,'$$ORIGIN/../lib'

# Compilar aplicación de prueba de motores con luces
$(BIN_DIR)/$(VEHICLE_TEST): examples/vehicle_test.c include/gpioio.h $(LIB_DIR)/$(LIB_FULL)
	$(CC) -Iinclude -o $@ $< -L$(LIB_DIR) -l$(LIBNAME) -Wl,-rpath,'$$ORIGIN/../lib'

# Limpiar archivos generados
clean:
	rm -rf $(BUILD_DIR)

# Instalación local
install: all
	install -d $(DESTDIR)$(PREFIX)/lib $(DESTDIR)$(PREFIX)/include $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(LIB_DIR)/$(LIB_FULL) $(DESTDIR)$(PREFIX)/lib/
	cd $(DESTDIR)$(PREFIX)/lib && ln -sf $(LIB_FULL) $(LIB_SONAME)
	cd $(DESTDIR)$(PREFIX)/lib && ln -sf $(LIB_SONAME) $(LIB_LINK)
	install -m 644 include/gpioio.h $(DESTDIR)$(PREFIX)/include/
	install -m 755 $(BIN_DIR)/$(APPNAME) $(DESTDIR)$(PREFIX)/bin/
	install -m 755 $(BIN_DIR)/$(MOTOR_TEST) $(DESTDIR)$(PREFIX)/bin/
	install -m 755 $(BIN_DIR)/$(VEHICLE_TEST) $(DESTDIR)$(PREFIX)/bin/

# Desplegar al Raspberry Pi via SSH
deploy-ssh: all
	@echo "Desplegando al Raspberry Pi..."
	ssh $(RPI_USER)@$(RPI_HOST) "mkdir -p $(RPI_PATH)/bin $(RPI_PATH)/lib"
	scp $(BIN_DIR)/$(APPNAME) $(RPI_USER)@$(RPI_HOST):$(RPI_PATH)/bin/
	scp $(BIN_DIR)/$(MOTOR_TEST) $(RPI_USER)@$(RPI_HOST):$(RPI_PATH)/bin/
	scp $(BIN_DIR)/$(VEHICLE_TEST) $(RPI_USER)@$(RPI_HOST):$(RPI_PATH)/bin/
	scp $(LIB_DIR)/$(LIB_FULL) $(RPI_USER)@$(RPI_HOST):$(RPI_PATH)/lib/
	ssh $(RPI_USER)@$(RPI_HOST) "cd $(RPI_PATH)/lib && ln -sf $(LIB_FULL) $(LIB_SONAME)"
	ssh $(RPI_USER)@$(RPI_HOST) "cd $(RPI_PATH)/lib && ln -sf $(LIB_SONAME) $(LIB_LINK)"
	@echo "Despliegue completado en $(RPI_HOST):$(RPI_PATH)"

# Ejecutar prueba básica remota
test-remote: deploy-ssh
	@echo "Ejecutando prueba básica en Raspberry Pi..."
	ssh $(RPI_USER)@$(RPI_HOST) "cd $(RPI_PATH) && LD_LIBRARY_PATH=./lib ./bin/$(APPNAME)"

# Ejecutar prueba de motores remota
test-motors-remote: deploy-ssh
	@echo "Ejecutando prueba de motores en Raspberry Pi..."
	ssh $(RPI_USER)@$(RPI_HOST) "cd $(RPI_PATH) && LD_LIBRARY_PATH=./lib ./bin/$(MOTOR_TEST)"

test-vehicle-remote: deploy-ssh
	@echo "Ejecutando prueba de vehículo en Raspberry Pi..."
	ssh $(RPI_USER)@$(RPI_HOST) "cd $(RPI_PATH) && LD_LIBRARY_PATH=./lib ./bin/$(VEHICLE_TEST)"