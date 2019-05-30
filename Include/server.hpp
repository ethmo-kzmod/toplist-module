#ifndef _SERVER_HPP
#define _SERVER_HPP

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "Include/session.hpp"


class server : public boost::enable_shared_from_this<server>
{
public:
	server(boost::asio::io_service &io_service, std::string ip, unsigned short port)
		: io_service_(io_service),
		running_(true),
		acceptor_(io_service_, boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(ip), port))
	{

		start_accept();
	}

	void stop();



private:
	void start_accept();

	void handle_accept(session* new_session, const boost::system::error_code& error);

	bool running_;
	boost::asio::io_service &io_service_;
	boost::asio::ip::tcp::acceptor acceptor_;
};

#endif // _SERVER_HPP