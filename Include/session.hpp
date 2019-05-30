#ifndef _SESSION_HPP
#define _SESSION_HPP

#include <iostream>
#include <memory>

#include <boost/asio.hpp>
#include <boost/bind.hpp>



class Toplist;

using boost::asio::ip::tcp;

class session
{
public:
	session(boost::asio::io_service& io_service)
		: socket_(io_service){
	}

	tcp::socket& socket(){

		return socket_;
	}

	void start();

private:
	void handle_read(const boost::system::error_code& error,
		size_t bytes_transferred);

	tcp::socket socket_;
	enum { max_length = 1024 };
	char data_[max_length];
};

 
#endif // _SESSION_HPP