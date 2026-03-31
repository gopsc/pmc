/*
 * 目前是Message Queue 的发送端（不涉及创建消息队列）
 *
 * FIXME: 集成发送、接收和创建
 */

#pragma once
#include <iostream>
#include <cstring>
#include <memory>
#include <boost/interprocess/ipc/message_queue.hpp>

namespace qing {
namespace bip = boost::interprocess;
using sf_t = std::function<void(const char*)>; /* success */
using ef_t = std::function<void(void)>;        /* error */
class Mq {
public:
	enum {CREATOR=0, USER=1};
	Mq(const std::string& name, int msglen, size_t msgcnt, int mode = USER) {
		this->name = std::string("/") + name; 
		this->msglen = msglen;
		this->msgcnt = msgcnt;
		this->mode = mode;
		switch (mode) {
		case USER:
			mqp = std::make_unique<bip::message_queue>(bip::open_only, this->name.c_str());
			break;
		case CREATOR: /* 由创建者创建消息队列 */
			bip::message_queue::remove(this->name.c_str());
			mqp = std::make_unique<bip::message_queue>(
				bip::create_only, this->name.c_str(),
				msgcnt, msglen * sizeof(char)
			);
			break;
		}
	}

	/* 创建者负责清理 */
	~Mq() {
		if (mode == CREATOR)
			bip::message_queue::remove(name.c_str());
	}

	/*  */
	Mq(const Mq&) = delete;
	Mq& operator=(const Mq&) = delete;


	/* 发送消息 */
	void send(std::string &msg)
	{
		if (msg.size() + 1 > msglen) /* over the len */
			throw std::length_error("Message queue Send Length over");

		mqp->send(msg.c_str(), msg.size() + 1, 0); /* include \0 */
	}

	/* 尝试接收消息 */
	void recv(sf_t success, ef_t notyet) {

		auto data = std::make_unique<char[]>(msglen); /* 接收缓冲区 */
		unsigned int priority; /* 优先级 */
		bip::message_queue::size_type recvd_size;

		try {	/* try_receive()返回bool类型 */
			auto r = mqp->try_receive(
				data.get(), msglen*sizeof(char),
				recvd_size, priority
			);

			if (r) /* 成功接收到数据 */
				success(data.get());

			else /* 没有收到数据 */
				notyet();
		}

		catch (std::exception &exp) {
    			std::cerr << "exception type: " << typeid(exp).name() << std::endl;
    			std::cerr << "exception message: " << exp.what() << std::endl;
		}
	}

private:

	/*  */
	std::unique_ptr<bip::message_queue> mqp;
	std::string name;
	size_t msglen=0, msgcnt=0;
	int mode;
};
}
