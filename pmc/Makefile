VERSION := 0.0.4

APP_NAME := pmcd

INSTL_DIRS := /usr/local/bin

CONF_NAME := conf.json

CONF_DIR := /etc/qsont

SRV_NAME := pmcd.service

SRV_DIR := /etc/systemd/system

CPP:= g++

LIBS := -lpthread -lssl -lcrypto -lcpp-httplib

CPPFLAGS := --std=c++17 -O1 -pipe #-Wall -Werror

#-----------
$(APP_NAME): src/*.cpp
	$(CPP) $(CPPFLAGS) $^ $(LIBS) -o $@

install: $(APP_NAME)
	@echo "正在安装 $(APP_NAME) 到$(INSTL_DIRS)"
	@
	@mkdir -p $(INSTL_DIRS)
	@cp -f pmcd $(INSTL_DIRS)
	#@chmod 755 $(INSTL_DIRS)/pmcd
	@
	@echo "正在配置 $(APP_NAME)"
	@mkdir -p $(CONF_DIR)
	#@cp sets/$(CONF_NAME) $(CONF_DIR)/$(CONF_NAME)
	@cp sets/$(SRV_NAME) $(SRV_DIR)/$(SRV_NAME)
	@systemctl daemon-reload
	@systemctl enable $(SRV_NAME)
	@systemctl restart $(SRV_NAME)
	@
	@echo "安装完成！"

uninstall:
	@echo "正在从 $(INSTL_DIRS) 卸载 $(APP_NAME)"
	@
	@rm -f $(INSTL_DIRS)/pmcd
	#@rm -f $(CONF_DIRS)/$(CONF_NAME)
	@
	@systemctl stop $(SRV_NAME)
	@systemctl disable $(SRV_NAME)
	@rm -f $(SRV_DIRS)/$(SRV_NAME)
	@systemctl daemon-reload
	@
	@echo "卸载完成！"

clean:
	@rm -f $(APP_NAME)
