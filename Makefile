CXX = g++
CXXFLAGS = -std=c++17 -Wall -O2
TARGET = ding

all: $(TARGET)

$(TARGET): main.cpp
	$(CXX) $(CXXFLAGS) main.cpp -o $(TARGET)

install: $(TARGET)
	sudo cp $(TARGET) /usr/local/bin/
	@echo "Ding! 已安装到系统。"

uninstall:
	sudo rm -f /usr/local/bin/$(TARGET)
	@echo "已卸载 Ding。"

clean:
	rm -f $(TARGET)
