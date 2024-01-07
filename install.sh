git clone http://git.zh.com.ru/alexey.zholtikov/zh_gateway.git
cd zh_gateway
mkdir components
cd components
git clone http://git.zh.com.ru/alexey.zholtikov/zh_config.git
git clone http://git.zh.com.ru/alexey.zholtikov/zh_vector.git
git clone -b esp32 --recursive http://git.zh.com.ru/alexey.zholtikov/zh_network.git
git clone http://git.zh.com.ru/alexey.zholtikov/zh_json.git
cd ..