#include "Include/server.hpp"


void server::start_accept()
{

	session* new_session = new session(io_service_);

	acceptor_.async_accept(new_session->socket(),
				boost::bind(&server::handle_accept, this, new_session,
				boost::asio::placeholders::error));
	
	
}

void server::handle_accept(session* new_session,
	const boost::system::error_code& error)
{

	if (!error)
	{
		new_session->start();
		acceptor_.cancel();
	}
	else
	{
		delete new_session;
	}

	start_accept();
}

void server::stop() {
	running_ = false;
}