VERSION := 0.0.4

APP_NAME := pmcd

INSTL_DIRS := /usr/local/bin

CONF_NAME := conf.json

CONF_DIR := /etc/qsont

SRV_NAME := pmcd.service

SRV_DIR := /etc/systemd/system

CPP:= g++

INCS:= -I include/

LIBS := -lpthread -lssl -lcrypto -lcpp-httplib -ljpeg -lboost_program_options

CPPFLAGS := --std=c++20 -O3 -pipe #-Wall -Werror

BUILD_DIR = build
$(shell mkdir -p $(BUILD_DIR))

#-----------
$(APP_NAME): $(BUILD_DIR)/main.o $(BUILD_DIR)/ansi.o $(BUILD_DIR)/utf8.o $(BUILD_DIR)/fsm.o $(BUILD_DIR)/Color.o $(BUILD_DIR)/ku500.o $(BUILD_DIR)/CommonThread.o $(BUILD_DIR)/Drawable.o $(BUILD_DIR)/Area.o $(BUILD_DIR)/CmdArea.o $(BUILD_DIR)/InputBox.o $(BUILD_DIR)/KeyboardEvent.o $(BUILD_DIR)/FrameBuf.o $(BUILD_DIR)/I2C.o $(BUILD_DIR)/PCA9685.o $(BUILD_DIR)/MPU6050.o $(BUILD_DIR)/Camera.o $(BUILD_DIR)/Matrix.o $(BUILD_DIR)/NeuralNetwork.o $(BUILD_DIR)/NNBuilder.o $(BUILD_DIR)/ILayer.o $(BUILD_DIR)/Thread.o $(BUILD_DIR)/HttpTask.o $(BUILD_DIR)/LciTask.o $(BUILD_DIR)/PmcTask.o $(BUILD_DIR)/ParserTask.o
	$(CPP) $^ $(LIBS) -o $@

$(BUILD_DIR)/main.o:  src/main.cpp
	$(CPP) $(CPPFLAGS) -c $^ $(INCS) -o $@

$(BUILD_DIR)/ansi.o:  src/lci/ansi.cpp
	$(CPP) $(CPPFLAGS) -c $^ $(INCS) -o $@

$(BUILD_DIR)/utf8.o:  src/lci/utf8.cpp
	$(CPP) $(CPPFLAGS) -c $^ $(INCS) -o $@

$(BUILD_DIR)/fsm.o:  src/lci/fsm.cpp
	$(CPP) $(CPPFLAGS) -c $^ $(INCS) -o $@

$(BUILD_DIR)/Color.o:  src/lci/Color.cpp
	$(CPP) $(CPPFLAGS) -c $^ $(INCS) -o $@

$(BUILD_DIR)/ku500.o:  src/lci/ku500.cpp
	$(CPP) $(CPPFLAGS) -c $^ $(INCS) -o $@

$(BUILD_DIR)/CommonThread.o:  src/lci/CommonThread.cpp
	$(CPP) $(CPPFLAGS) -c $^ $(INCS) -o $@

$(BUILD_DIR)/Drawable.o:  src/lci/Drawable.cpp
	$(CPP) $(CPPFLAGS) -c $^ $(INCS) -o $@

$(BUILD_DIR)/Area.o:  src/lci/Area.cpp
	$(CPP) $(CPPFLAGS) -c $^ $(INCS) -o $@

$(BUILD_DIR)/CmdArea.o:  src/lci/CmdArea.cpp
	$(CPP) $(CPPFLAGS) -c $^ $(INCS) -o $@

$(BUILD_DIR)/InputBox.o:  src/lci/InputBox.cpp
	$(CPP) $(CPPFLAGS) -c $^ $(INCS) -o $@

$(BUILD_DIR)/KeyboardEvent.o:  src/lci/KeyboardEvent.cpp
	$(CPP) $(CPPFLAGS) -c $^ $(INCS) -o $@

$(BUILD_DIR)/FrameBuf.o:  src/lci/FrameBuf.cpp
	$(CPP) $(CPPFLAGS) -c $^ $(INCS) -o $@

$(BUILD_DIR)/I2C.o:  src/hd/I2C.cpp
	$(CPP) $(CPPFLAGS) -c $^ $(INCS) -o $@

$(BUILD_DIR)/PCA9685.o:  src/hd/PCA9685.cpp
	$(CPP) $(CPPFLAGS) -c $^ $(INCS) -o $@

$(BUILD_DIR)/MPU6050.o:  src/hd/MPU6050.cpp
	$(CPP) $(CPPFLAGS) -c $^ $(INCS) -o $@

$(BUILD_DIR)/Camera.o:  src/hd/Camera.cpp
	$(CPP) $(CPPFLAGS) -c $^ $(INCS) -o $@

$(BUILD_DIR)/Matrix.o:  src/nn/Matrix.cpp
	$(CPP) $(CPPFLAGS) -c $^ $(INCS) -o $@

$(BUILD_DIR)/NeuralNetwork.o: src/nn/NeuralNetwork.cpp
	$(CPP) $(CPPFLAGS) -c $^ $(INCS) -o $@

$(BUILD_DIR)/NNBuilder.o: src/nn/NNBuilder.cpp
	$(CPP) $(CPPFLAGS) -c $^ $(INCS) -o $@

$(BUILD_DIR)/ILayer.o: src/nn/ILayer.cpp
	$(CPP) $(CPPFLAGS) -c $^ $(INCS) -o $@

$(BUILD_DIR)/Thread.o: src/th/Thread.cpp
	$(CPP) $(CPPFLAGS) -c $^ $(INCS) -o $@

$(BUILD_DIR)/HttpTask.o: src/pmc/HttpTask.cpp
	$(CPP) $(CPPFLAGS) -c $^ $(INCS) -o $@

$(BUILD_DIR)/LciTask.o: src/pmc/LciTask.cpp
	$(CPP) $(CPPFLAGS) -c $^ $(INCS) -o $@

$(BUILD_DIR)/PmcTask.o: src/pmc/PmcTask.cpp
	$(CPP) $(CPPFLAGS) -c $^ $(INCS) -o $@

$(BUILD_DIR)/ParserTask.o: src/pmc/ParserTask.cpp
	$(CPP) $(CPPFLAGS) -c $^ $(INCS) -o $@


install: $(APP_NAME)
	@echo "正在安装 $(APP_NAME) 到$(INSTL_DIRS)"
	@
	@mkdir -p $(INSTL_DIRS)
	@cp -f pmcd $(INSTL_DIRS)
	#@chmod 755 $(INSTL_DIRS)/pmcd
	@
	@echo "正在配置 $(APP_NAME)"
	@mkdir -p $(CONF_DIR)
	#@cp $(CONF_NAME) $(CONF_DIR)/$(CONF_NAME)
	@cp $(SRV_NAME) $(SRV_DIR)/$(SRV_NAME)
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
	@rm -rvf $(BUILD_DIR)
	@rm -vf $(APP_NAME)
